import logging
import sys

from streamlit.web import cli as stcli


# Configure the global logger
logging.basicConfig(
    level=logging.DEBUG,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    handlers=[logging.StreamHandler()],
)

logger = logging.getLogger("DBPiper")

if __name__ == "__main__":
    sys.argv = ["streamlit", "run", "proj_2/interface.py"]
    sys.exit(stcli.main())
