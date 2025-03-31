import streamlit as st

st.set_page_config(page_title="QEP Visualizer", layout="wide")

# Initialize session states
default_session_states = {
    "page": "login",
    "connection": None,
    "db_location": "Cloud",
    "db_schema": "IMDB",
    "selected_example_query": "",
    "pipe_syntax_result": ""
}
for key, value in default_session_states.items():
    if key not in st.session_state:
        st.session_state[key] = value

# TODO: setup examples
example_queries = {
    "1 - Find XXX": "SELECT * FROM xxx;",
    "2 - Find YYY": "SELECT * FROM yyy;",
    "3 - Find ZZZ": "SELECT * FROM zzz;"
}


def login():
    st.title("QEP Visualizer")
    st.subheader("Database Login")

    db_location_options = ("Cloud", "Local")
    st.session_state.db_location = st.radio(
        label="**Database Location**",
        options=db_location_options,
        index=db_location_options.index(default_session_states["db_location"])
    )

    # TODO: confirm if we need to switch between the datasets?
    db_schema_options = ("TPC-H", "IMDB")
    st.session_state.db_schema = st.radio(
        label="**Database Schema**",
        options=db_schema_options,
        index=db_schema_options.index(default_session_states["db_schema"])
    )

    if st.session_state.db_location == "Cloud":
        # TODO: set up cloud db & store credentials in secrets
        db_params = {
            "dbname": "imdb",
            "user": "group1",
            "password": "group1",
            "host": "localhost",
            "port": "5432",
            "sslmode": "require"
        }
    elif st.session_state.db_location == "Local":
        st.markdown("**Local Database Credentials**")
        dbname = st.text_input(
            label="Database Name",
            placeholder="Enter database name"
        )
        username = st.text_input(
            label="Username",
            placeholder="Enter your username"
        )
        password = st.text_input(
            label="Password",
            type="password",
            placeholder="Enter your password"
        )
        host = st.text_input(
            label="Host",
            placeholder="e.g., localhost, or IP address"
        )
        port = st.text_input(
            label="Port",
            placeholder="e.g., 5432"
        )

        db_params = {
            "user": username,
            "password": password,
            "dbname": dbname,
            "host": host,
            "port": port,
            "sslmode": "prefer"
        }

    if st.button("Login"):
        # Empty input validation
        if any(value.strip() == "" for value in db_params.values()):
            st.error(f"One or more fields are empty.")
        else:
            try:
                db_manager = None  # TODO: connect using db_params
                # db_manager.connect()
                # with db_manager.conn.cursor() as cursor:
                #     cursor.execute("SET max_parallel_workers_per_gather = 0;")
                # db_manager.conn.commit()
                # st.session_state.connection = db_manager.conn

                st.session_state.page = "main"
                st.rerun()
            except Exception as e:
                st.error(f"Failed to connect to the database. Error: {e}")


def main():
    # TODO: fetch dynamically
    st.sidebar.header("Current Database: TPC-H")

    # TODO: accordions to expand/hide schema hierarchy
    st.sidebar.subheader("DB Schema")
    # TODO: fetch dynamically.
    db_schema = """
        - customer
        - lineitem
        - nation
        - orders
        - part
        - partsupp
        - region
        - supplier
    """
    st.sidebar.text(db_schema)

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
            value=st.session_state.selected_example_query.strip()
        )

        col1_1, col1_2 = st.columns([1, 3], gap="medium")
        with col1_1:
            if st.button("Run Query"):
                st.session_state.pipe_syntax_result = f"TODO pipe syntax result of '{sql_query}'"
        with col1_2:
            with st.expander(label="Select an Example Query", expanded=False):
                for ex in example_queries:
                    if st.button(ex):
                        st.session_state.selected_example_query = example_queries.get(
                            ex)

    with col2:
        pipe_syntax_result = st.text_area(
            label="Pipe Syntax Result",
            value=st.session_state.pipe_syntax_result,
            disabled=True
        )

    st.subheader("QEP Visualizer")


# Page navigation
page_names_to_funcs = {
    "login": login,
    "main": main,
}
page_names_to_funcs[st.session_state.page]()
