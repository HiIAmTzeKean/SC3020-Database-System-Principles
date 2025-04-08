import logging
import subprocess

# Configure the global logger
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler()
    ]
)

logger = logging.getLogger('DBPiper')

if __name__ == "__main__":
    # Run Streamlit server i.e. CLI command "streamlit run interface.py"
    subprocess.run(["streamlit", "run", "interface.py"])
