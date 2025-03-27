
import os
import csv
import logging
import argparse
import psycopg2

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
)

# CLI arguments
args = argparse.ArgumentParser()
args.add_argument('--dir', '-d', type=str, default='imdb', help='Directory where SQL schema files are stored')
args.add_argument('--data', type=str, default='dataFile', help='Directory where local TSV files are stored')
args.add_argument('--pguser', '-U', type=str, default='postgres', help='Postgres user')
args.add_argument('--pghost', '-H', type=str, default='localhost', help='Postgres host')
args.add_argument('--pgdb', '-D', type=str, default='postgres', help='Postgres database')
args.add_argument('--pgport', '-p', type=int, default=5432, help='Postgres port')
args.add_argument('--pgpass', type=str, default='postgres', help='Postgres password')
args = args.parse_args()

# Configuration
SCHEMA_DIR = "imdb"
DATA_DIR = "dataFile"
PG_USER = args.pguser
PG_HOST = args.pghost
PG_DB = args.pgdb
PG_PORT = args.pgport
PG_PASS = args.pgpass

DATASETS = [
    'name.basics',
    'title.akas',
    'title.basics',
    'title.crew',
    'title.episode',
    'title.principals',
    'title.ratings'
]

def connect():
    return psycopg2.connect(
        dbname=PG_DB,
        user=PG_USER,
        password=PG_PASS,
        host=PG_HOST,
        port=PG_PORT
    )

def import_dataset(dataset):
    sql_file = os.path.join(SCHEMA_DIR, f"{dataset}.sql")
    tsv_file = os.path.join(DATA_DIR, f"{dataset}.tsv")

    if not os.path.exists(sql_file):
        logging.warning(f"Schema not found: {sql_file}")
        return
    if not os.path.exists(tsv_file):
        logging.warning(f"Data file not found: {tsv_file}")
        return

    with connect() as conn:
        with conn.cursor() as cur:
            logging.info(f"Importing schema: {dataset}")
            with open(sql_file, 'r', encoding='utf-8') as sf:
                cur.execute(sf.read())
            conn.commit()

            logging.info(f"Importing data: {dataset}")
            with open(tsv_file, 'rb') as f:
                cleaned_lines = []
                skipped = 0
                for line in f:
                    try:
                        decoded = line.decode('latin1').encode('utf-8').decode('utf-8')
                        cleaned_lines.append(decoded)
                    except Exception:
                        skipped += 1

                logging.info(f"✓ Cleaned {len(cleaned_lines)} lines | ✗ Skipped {skipped} lines")

                from io import StringIO
                sio = StringIO(''.join(cleaned_lines))
                cur.copy_expert(f"COPY {dataset.replace('.', '_')} FROM STDIN WITH (FORMAT text, DELIMITER E'\t', NULL '\\N', HEADER true)", sio)
            conn.commit()

# Main execution
for dataset in DATASETS:
    import_dataset(dataset)
