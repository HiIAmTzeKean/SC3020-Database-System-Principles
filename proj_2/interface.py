import json

import streamlit as st
from pipesyntax import PipeSyntaxParser
from preprocessing import Database
from streamlit_ace import st_ace
from streamlit_flow import streamlit_flow
from streamlit_flow.layouts import LayeredLayout
from streamlit_flow.state import StreamlitFlowState


st.set_page_config(page_title="QEP Visualizer", layout="wide")

# Initialize session states
default_session_states = {
    "page": "login",
    "db_connection": None,
    "db_schema": None,
    "db_location": "Cloud",
    "db_name": "",
    "selected_example_query": "",
    "pipe_syntax_result": "",
    "qep_results": "",
    "query_results": None,
    "pipe_syntax_parser": None,
    "streamlit_flow_state": None,
    "editor_rerun_key": 0,
}
for key, value in default_session_states.items():
    if key not in st.session_state:
        st.session_state[key] = value

# TODO: setup examples
example_queries = {
    "1 - Get the top 10 highest-rated movies with more than 100,000 votes":
    """
    SELECT tb.primarytitle, tr.averagerating, tr.numvotes
    FROM title_basics tb
    JOIN title_ratings tr ON tb.tconst = tr.tconst
    WHERE tb.titletype = 'movie'
    AND tr.numvotes > 100000
    ORDER BY tr.averagerating DESC
    LIMIT 10;
    """,
    "2 - Get number of titles per year from 2020-2022":
    """
    SELECT startyear, COUNT(*) AS num_titles
    FROM title_basics
    WHERE startyear BETWEEN 2020 AND 2022
    GROUP BY startyear
    ORDER BY startyear DESC;
    """,
    "3 - Get all actors in a specified movie":
    """
    SELECT nb.primaryname, tp.category, tp.characters
    FROM title_basics tb
    JOIN title_principals tp ON tb.tconst = tp.tconst
    JOIN name_basics nb ON tp.nconst = nb.nconst
    WHERE tb.primarytitle = 'Inception'
    AND tp.category = 'actor';
    """
}


def login():
    st.title(
        "QEP Visualizer [SC3020 Group 1]")
    st.subheader("❓ Can Query Execution Plans Help You Generate Better SQL?")
    st.markdown("""
    This project aims to enhance SQL query comprehension by transforming input SQL queries into an equivalent, easier-to-read pipe-syntax format using query execution plans (QEP).
            
    ##### ✨ Features:
    1. **Generate Pipe-Syntax Format**: Convert SQL queries into its pipe-syntax format, annotated with estimated execution costs.  
    2. **Visualize QEP**: Display an interactive visualization of the QEP for the SQL queries.
    3. **Query Interface**: Provide a user-friendly interface to write and execute SQL queries with syntax highlighting, error validation, and display of query results in a dataframe. The database schema can also be viewed.
    """)
    st.subheader("Database Login")

    db_location_options = ("Cloud", "Local")
    st.session_state.db_location = st.radio(
        label="**Database Location**",
        options=db_location_options,
        index=db_location_options.index(default_session_states["db_location"]),
    )

    with st.form("db_login_form"):
        if st.session_state.db_location == "Cloud":
            db_params = {
                "dbname": "imdb",
                "user": "group1",
                "password": "group1",
                "host": "18.140.58.158",
                "port": "5432",
                "sslmode": "require",
            }
        elif st.session_state.db_location == "Local":
            st.markdown("**Local Database Credentials**")
            dbname = st.text_input(
                label="Database Name",
                placeholder="Enter database name (e.g. imdb)",
            )
            username = st.text_input(
                label="Username",
                placeholder="Enter username (e.g. group1)",
            )
            password = st.text_input(
                label="Password",
                type="password",
                placeholder="Enter password (e.g. group1)",
            )
            host = st.text_input(
                label="Host",
                placeholder="Enter host IP address (e.g. localhost)",
            )
            port = st.text_input(
                label="Port",
                placeholder="Enter port (e.g. 5432)",
            )

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
                    print(db_params)
                    db = Database(db_params)
                    st.session_state.db_connection = db
                    st.session_state.pipe_syntax_parser = PipeSyntaxParser(db)
                    st.session_state.db_name = db_params["dbname"]
                    st.session_state.db_schema = st.session_state.db_connection.get_db_schema()

                    st.session_state.page = "main"
                    st.rerun()
                except Exception as e:
                    st.error(f"Failed to connect to the database. Error: {e}")


def main():
    st.sidebar.header(f"Current Database: {st.session_state.db_name}")

    st.sidebar.subheader("DB Schema")
    if st.session_state.db_schema:
        for table, schema in st.session_state.db_schema.items():
            with st.sidebar.expander(table):
                for attribute, data_type in schema.items():
                    st.markdown(f"- **{attribute}**: {data_type}")

    if st.sidebar.button("Logout"):
        # Reset session states
        for key, value in default_session_states.items():
            st.session_state[key] = value
        st.rerun()

    col1, col2 = st.columns(2, gap="large")
    error_msg = None
    with col1:
        st.markdown("**SQL Query**")
        sql_query = st_ace(
            value=st.session_state.selected_example_query.strip(),
            language="sql",
            height=300,
            font_size=16,
            theme="sqlserver",
            auto_update=True,
            key=f"editor_{st.session_state.editor_rerun_key}",
            placeholder="Enter SQL query ..."
        )

        col1_1, col1_2 = st.columns([2, 4], gap="small")
        with col1_1:
            if st.button("Get Pipe Syntax & QEP", type="primary", use_container_width=True):
                # Reset
                st.session_state.qep_results = ""
                st.session_state.pipe_syntax_result = ""
                st.session_state.streamlit_flow_state = None
                st.session_state.query_results = None

                if sql_query.strip() == "":
                    error_msg = f"Failed to get QEP. Error:  \nEmpty query"
                elif st.session_state.db_connection:
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
                        if sql_query in str(e):
                            # hide SQL query error highlighting
                            error_msg = f"Failed to get QEP. Error:  \n{str(str(e).splitlines()[0])}"
                        else:
                            error_msg = f"Failed to get QEP. Error:  \n{str(e)}"

            if st.button("Get Query Output", use_container_width=True):
                # Reset
                st.session_state.qep_results = ""
                st.session_state.pipe_syntax_result = ""
                st.session_state.streamlit_flow_state = None
                st.session_state.query_results = None

                if st.session_state.db_connection:
                    try:
                        st.session_state.query_results = st.session_state.db_connection.execute_query(
                            sql_query)
                    except Exception as e:
                        if sql_query in str(e):
                            # hide SQL query error highlighting
                            error_msg = f"Failed to run query. Error:  \n{str(str(e).splitlines()[0])}"
                        else:
                            error_msg = f"Failed to run query. Error:  \n{str(e)}"
        with col1_2:
            with st.expander(label="Select an Example Query", expanded=False):
                for query_name, query in example_queries.items():
                    if st.button(query_name):
                        st.session_state.selected_example_query = query
                        st.session_state.editor_rerun_key += 1  # force code editor to rerun
                        st.rerun()

        if error_msg:
            st.error(error_msg)

    with col2:
        st.markdown("**Pipe Syntax Result**")
        pipe_syntax_result = st_ace(
            value=st.session_state.pipe_syntax_result,
            language="c_cpp",  # just for syntax highlighting style
            height=300,
            font_size=16,
            theme="terminal",
            auto_update=True,
            readonly=True,
            show_gutter=False,
        )

    if st.session_state.qep_results:
        st.subheader("QEP Visualizer")
        # TODO fix bug where nodes are not set correctly to layout (reset to 0,0 pos) when same query is re-run
        streamlit_flow(
            "flow",
            st.session_state.streamlit_flow_state,
            layout=LayeredLayout(direction="right", node_layer_spacing=100),
            fit_view=True,
            hide_watermark=True,
        )

        # st.write(st.session_state.qep_results)  # QEP output, for testing only
    if st.session_state.query_results is not None:
        st.subheader("Query Execution Results")
        st.dataframe(st.session_state.query_results)


# Page navigation
page_names_to_funcs = {
    "login": login,
    "main": main,
}
page_names_to_funcs[st.session_state.page]()
