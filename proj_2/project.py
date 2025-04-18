import logging
import signal
import sys
import os

# Configure the global logger
logging.basicConfig(
    level=logging.DEBUG,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    handlers=[logging.StreamHandler()],
)

logger = logging.getLogger("DBPiper")


def signal_handler(sig, frame):
    sys.exit(0)


if __name__ == "__main__":
    # Register signal handler for SIGINT (Ctrl+C)
    signal.signal(signal.SIGINT, signal_handler)

    try:
        os.system('streamlit run interface.py')
    except KeyboardInterrupt:
        signal_handler(None, None)
