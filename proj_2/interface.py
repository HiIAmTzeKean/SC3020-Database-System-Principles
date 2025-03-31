import streamlit as st

st.set_page_config(page_title="QEP Visualizer", layout="wide")

# Initialize session states
default_session_states = {
    "page": "login",
    "connection": None,
    "db_location": "Cloud",
    "db_schema": "IMDB"
}
for key, value in default_session_states.items():
    if key not in st.session_state:
        st.session_state[key] = value


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
            "dbname": "",
            "user": "",
            "password": "",
            "host": "",
            "port": "",
            "sslmode": "require"
        }
    elif st.session_state.db_location == "Local":
        st.markdown("**Local Database Credentials**")
        # TODO: empty input validation
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

    st.title("Main")


# Page navigation
page_names_to_funcs = {
    "login": login,
    "main": main,
}
page_names_to_funcs[st.session_state.page]()
