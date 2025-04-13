import streamlit as st
from preprocessing import Database
from pipesyntax import PipeSyntaxParser
from streamlit_flow import streamlit_flow
from streamlit_flow.elements import StreamlitFlowNode, StreamlitFlowEdge
from streamlit_flow.state import StreamlitFlowState
from streamlit_flow.layouts import LayeredLayout
import json


st.set_page_config(page_title="QEP Visualizer", layout="wide")

# Initialize session states
default_session_states = {
    "page": "login",
    "db_connection": None,
    "db_location": "Cloud",
    "db_schema": "IMDB",
    "selected_example_query": "",
    "pipe_syntax_result": "",
    "qep_results": "",
    "pipe_syntax_parser": None,
    "streamlit_flow_state": None,
}
for key, value in default_session_states.items():
    if key not in st.session_state:
        st.session_state[key] = value

# TODO: setup examples
example_queries = {
    "1 - Get first 10 rows from title_basics table": """
        SELECT * FROM title_basics LIMIT 10;
    """,
    "2 - Get original titles released in non-English languages": """
        SELECT tb.primarytitle, ta.language
        FROM title_basics tb
        JOIN title_akas ta ON tb.tconst = ta.titleid
        WHERE ta.language != 'en' AND ta.isoriginaltitle = TRUE;
    """,
    "3 - Find ZZZ": "SELECT * FROM zzz;",
}


def login():
    st.title("QEP Visualizer")
    st.subheader("Database Login")

    db_location_options = ("Cloud", "Local")
    st.session_state.db_location = st.radio(
        label="**Database Location**",
        options=db_location_options,
        index=db_location_options.index(default_session_states["db_location"]),
    )

    with st.form("db_login_form"):
        if st.session_state.db_location == "Cloud":
            # TODO: set up cloud db & store credentials in secrets
            db_params = {
                "dbname": "imdb",
                "user": "group1",
                "password": "group1",
                "host": "localhost",
                "port": "5432",
                "sslmode": "require",
            }
        elif st.session_state.db_location == "Local":
            st.markdown("**Local Database Credentials**")
            dbname = st.text_input(
                label="Database Name", placeholder="Enter database name"
            )
            username = st.text_input(
                label="Username", placeholder="Enter your username"
            )
            password = st.text_input(
                label="Password", type="password", placeholder="Enter your password"
            )
            host = st.text_input(
                label="Host", placeholder="e.g., localhost, or IP address"
            )
            port = st.text_input(label="Port", placeholder="e.g., 5432")

            db_params = {
                "user": username,
                "password": password,
                "dbname": dbname,
                "host": host,
                "port": port,
                "sslmode": "prefer",
            }

        if st.form_submit_button("Login"):
            # Empty input validation
            if any(value.strip() == "" for value in db_params.values()):
                st.error(f"One or more fields are empty.")
            else:
                try:
                    db = Database()
                    st.session_state.db_connection = db  # TODO: connect using db_params
                    st.session_state.pipe_syntax_parser = PipeSyntaxParser(db)

                    st.session_state.page = "main"
                    st.rerun()
                except Exception as e:
                    st.error(f"Failed to connect to the database. Error: {e}")


def main():
    db_name = ""
    if st.session_state.db_connection:
        # TODO: dynamically fetch DB name
        # db_name = st.session_state.db_connection.get_db_name()
        db_name = "imdb"
    st.sidebar.header(f"Current Database: {db_name}")

    st.sidebar.subheader("DB Schema")
    if st.session_state.db_connection:
        # TODO: dynamically fetch DB schema
        # db_schema = st.session_state.db_connection.get_db_schema()
        db_schema = {
            "name_basics": {
                "nconst": "text",
                "primaryname": "text",
                "birthyear": "integer",
                "deathyear": "integer",
                "primaryprofession": "text",
                "knownfortitles": "text",
            },
            "title_akas": {
                "titleid": "text",
                "ordering": "integer",
                "title": "text",
                "region": "text",
                "language": "text",
                "types": "text",
                "attributes": "text",
                "isoriginaltitle": "boolean",
            },
            "title_basics": {
                "tconst": "text",
                "titletype": "text",
                "primarytitle": "text",
                "originaltitle": "text",
                "isadult": "boolean",
                "startyear": "integer",
                "endyear": "integer",
                "runtimeminutes": "integer",
                "genres": "text",
            },
            "title_crew": {"tconst": "text", "directors": "text", "writers": "text"},
            "title_episode": {
                "tconst": "text",
                "parenttconst": "text",
                "seasonnumber": "integer",
                "episodenumber": "integer",
            },
            "title_principals": {
                "tconst": "text",
                "ordering": "integer",
                "nconst": "text",
                "category": "text",
                "job": "text",
                "characters": "text",
            },
            "title_ratings": {
                "tconst": "text",
                "averagerating": "double precision",
                "numvotes": "integer",
            },
        }
        for table, schema in db_schema.items():
            with st.sidebar.expander(table):
                for attribute, data_type in schema.items():
                    st.markdown(f"- **{attribute}**: {data_type}")

    if st.sidebar.button("Logout"):
        # Reset session states
        for key, value in default_session_states.items():
            st.session_state[key] = value
        st.rerun()

    col1, col2 = st.columns(2, gap="large")
    with col1:
        # TODO (nice-to-have): query validation, syntax highlighting
        sql_query = st.text_area(
            label="SQL Query",
            value=st.session_state.selected_example_query.strip(),
            height=200,
        )

        col1_1, col1_2 = st.columns([1, 3], gap="medium")
        with col1_1:
            if st.button("Run Query"):
                # Reset
                st.session_state.qep_results = ""
                st.session_state.pipe_syntax_result = ""
                st.session_state.streamlit_flow_state = None

                if st.session_state.db_connection:
                    # TODO: handle QEP results
                    try:
                        st.session_state.qep_results = json.loads(
                            st.session_state.db_connection.get_qep(sql_query)
                        )
                        st.session_state.pipe_syntax_result = (
                            st.session_state.pipe_syntax_parser.get_pipe_syntax(
                                sql_query
                            )
                        )
                        qep_graph = st.session_state.pipe_syntax_parser.get_execution_plan_graph(
                            sql_query
                        )
                        nodes, edges = qep_graph.visualize()
                        if not st.session_state.streamlit_flow_state:
                            st.session_state.streamlit_flow_state = StreamlitFlowState(
                                nodes, edges
                            )
                    except Exception as e:
                        # TODO proper error handling
                        st.error(e)
        with col1_2:
            with st.expander(label="Select an Example Query", expanded=False):
                for query_name, query in example_queries.items():
                    if st.button(query_name):
                        st.session_state.selected_example_query = query
                        st.rerun()

    with col2:
        pipe_syntax_result = st.text_area(
            label="Pipe Syntax Result",
            value=st.session_state.pipe_syntax_result,
            disabled=True,
            height=200,
        )

    st.subheader("QEP Visualizer")

    if st.session_state.qep_results:
        # TODO fix bug where nodes are not set correctly to layout (reset to 0,0 pos) when same query is re-run
        streamlit_flow(
            "flow",
            st.session_state.streamlit_flow_state,
            layout=LayeredLayout(direction="right", node_layer_spacing=100),
            fit_view=True,
            hide_watermark=True,
        )

        st.write(st.session_state.qep_results)  # TODO: remove after testing


# Page navigation
page_names_to_funcs = {
    "login": login,
    "main": main,
}
page_names_to_funcs[st.session_state.page]()
