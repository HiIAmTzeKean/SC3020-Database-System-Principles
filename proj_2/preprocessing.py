import psycopg2
from psycopg2 import sql
import json


class Database:
    def __init__(self, config) -> None:
        """Initializes the database connection."""
        self.config = config
        try:
            self.conn = psycopg2.connect(**self.config)
            self.conn.autocommit = True
            self.cursor = self.conn.cursor()
            print("Connected to database successfully.")
        except Exception as e:
            raise e

    def get_db_schema(self) -> dict:
        """Returns a nested dictionary of public tables with their columns and data types."""
        try:
            # Query to fetch all tables in the 'public' schema
            self.cursor.execute(
                """
                SELECT table_name
                FROM information_schema.tables
                WHERE table_schema = 'public'
                AND table_type = 'BASE TABLE'
                ORDER BY table_name;
            """
            )
            tables = [row[0] for row in self.cursor.fetchall()]

            # Create a nested dictionary for ALLOWED_COLUMNS dynamically
            schema_dict = {}
            for table in tables:
                self.cursor.execute(
                    f"""
                    SELECT column_name, data_type
                    FROM information_schema.columns
                    WHERE table_schema = 'public'
                    AND table_name = %s
                    ORDER BY ordinal_position;
                """,
                    (table,),
                )

                # Dict {column_name: data_type}
                columns = {row[0]: row[1] for row in self.cursor.fetchall()}
                # {table_name: {column_name: data_type}}
                schema_dict[table] = columns

            return schema_dict
        except Exception as e:
            print("Error retrieving tables and columns:", e)
            raise e

    def get_qep(self, query: str) -> str | None:
        """Retrieves the Query Execution Plan (QEP) for a given SQL query."""
        try:
            self.cursor.execute(
                sql.SQL("EXPLAIN (ANALYZE, FORMAT JSON) {}").format(sql.SQL(query))
            )
            qep_result = self.cursor.fetchone()[0]
            return json.dumps(qep_result)
        except Exception as e:
            print("Error retrieving QEP:", e)
            raise e

    def close_connection(self) -> None:
        """Closes the database connection."""
        if hasattr(self, "cursor") and self.cursor:
            self.cursor.close()
        if hasattr(self, "conn") and self.conn:
            self.conn.close()
            print("Database connection closed.")


if __name__ == "__main__":
    config = {
        "dbname": input("Enter database name: ") or "imdb",
        "user": input("Enter username: ") or "group1",
        "password": input("Enter password: ") or "group1",
        "host": input("Enter host: ") or "18.140.58.158",
        "port": int(input("Enter port: ") or 5432),
    }

    db = Database(config)

    # Ask for SQL query
    # Example query = "SELECT * FROM name_basics LIMIT 10;"
    query = input("Enter your SQL query: ")

    qep_json = db.get_qep(query)
    if qep_json:
        print("QEP Result:\n", json.dumps(json.loads(qep_json), indent=2))

    db.close_connection()
