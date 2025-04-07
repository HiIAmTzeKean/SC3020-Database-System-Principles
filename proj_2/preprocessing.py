import psycopg2
from psycopg2 import sql


class Database:
    config = {
        "dbname": "imdb",
        "user": "group1",
        "password": "group1",
        "host": "localhost",
        "port": 5432,
    }

    def __init__(self) -> None:
        """Initializes the database connection."""
        try:
            self.conn = psycopg2.connect(**self.config)
            self.conn.autocommit = True
            self.cursor = self.conn.cursor()
            print("Connected to database successfully.")
        except Exception as e:
            # database likely not created yet
            self.setup()

    def setup(self) -> None:
        """Ensures the database and user exist before proceeding."""
        try:
            admin_conn = psycopg2.connect(
                dbname="postgres",
                user="postgres",
                password="postgres",
                host="localhost",
                port=5432,
            )
            admin_conn.autocommit = True
            admin_cursor = admin_conn.cursor()

            admin_cursor.execute(
                "SELECT 1 FROM pg_roles WHERE rolname=%s;", ("group1",)
            )
            if admin_cursor.fetchone() is None:
                admin_cursor.execute("CREATE USER group1 WITH LOGIN PASSWORD 'group1';")
                print("User 'group1' created successfully.")

            admin_cursor.execute(
                "SELECT 1 FROM pg_database WHERE datname=%s;", ("imdb",)
            )
            if admin_cursor.fetchone() is None:
                admin_cursor.execute("CREATE DATABASE imdb WITH OWNER group1;")
                print("Database 'imdb' created successfully.")

            admin_cursor.close()
            admin_conn.close()
        except Exception as e:
            print("Error setting up PostgreSQL:", e)

    def get_qep(self, query: str) -> str | None:
        """Retrieves the Query Execution Plan (QEP) for a given SQL query."""
        try:
            self.cursor.execute(
                sql.SQL("EXPLAIN (ANALYZE, FORMAT JSON) {}").format(sql.SQL(query))
            )
            qep_result = self.cursor.fetchone()[0]
            return qep_result
        except Exception as e:
            print("Error retrieving QEP:", e)
            return None

    def close_connection(self) -> None:
        """Closes the database connection."""
        if hasattr(self, "cursor") and self.cursor:
            self.cursor.close()
        if hasattr(self, "conn") and self.conn:
            self.conn.close()
            print("Database connection closed.")


if __name__ == "__main__":
    db = Database()
    db.get_qep("SELECT * FROM title_basics LIMIT 10;")
    db.close_connection()
