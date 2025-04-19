## Project Setup

Project can be setup using either `pip` or `poetry` package managers.
### Installation with pip
1. Open a command prompt or terminal window in the root of the extracted source code directory.
2. Create a Python virtual environment (only need to be done once).
```bash
python -m venv .venv
```
3. Activate the virtual environment for the directory.

Mac:
```bash
source .venv/bin/activate
```
Windows:
```bash
.venv/Scripts/activate
```
4. Install the required Python dependencies from the `pyproject.toml` file.
```bash
python -m pip install .
```
5. Run the application.
```bash
python project.py
```

### Installation with Poetry
1. Open a command prompt or terminal window in the root of the extracted source code directory.
2. Install the required Python dependencies from the `pyproject.toml` file.
```bash
poetry install
```
3. Run the application.
```bash
python project.py
```


## PostgreSQL Database Setup

Set up PostgreSQL either manually or using Docker. 

### Option 1: Using Docker

```bash
docker-compose up -d --build
```

### Option 2: Manual Setup

1. Install PostgreSQL on system.
2. Create a database named `imdb`.
3. Create a user named `group1` with the password `group1`.
4. Ensure the PostgreSQL server is running and accessible.

## Initialising the Database

### Downloading the Data

1. Download from [IMDb Datasets](https://datasets.imdbws.com/) page. 
2. Extract the files and place them in the `init/data` directory. The extracted files should have the `.tsv` extension.

### Running the Initialisation Script

1. Switch to the `init` directory.
2. Run the initialisation script:
    ```bash
    python ingest_data.py
    ```
3. The script already contains the correct arguments to connect to the database. To change the arguments, pass them as arguments to the script:
    ```bash
    # Example with arguments
    python ingest_data.py -U group1 -H localhost -D imdb -p 5432 -d imdb --data data
    ```
4. The script takes about 30+ minutes to run.

### Checking the Tables and Data 

1. Connect to the database using `psql`:
    ```bash
    psql -U group1 -d imdb
    ```
2. If using Docker, connect to the database using:
    ```bash
    docker exec -it proj_2-db-1 psql -U group1 -d imdb
    ```
3. If the connection is successful, should see the following prompt:
    ```sql
    imdb=#
    ```
4. Check the tables:
    ```sql
    \dt
    ```
5. Check the data in a table:
    ```sql
    SELECT * FROM <table_name> LIMIT 30;
    ```
