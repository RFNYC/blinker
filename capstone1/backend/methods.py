import subprocess

import requests, os
from dotenv import find_dotenv, load_dotenv

dotenv_path = find_dotenv()
load_dotenv(dotenv_path)
key = os.getenv("TOPIC")
address = os.getenv("ADDRESS")
url = f'https://ntfy.sh/{key}'

# For more information on title, priority, tags, ect. 
# https://fotos.esferas.org/docs/publish/ 

def send_notification():

    requests.post(url,
    data="CHALLENGE FAILED: MORSE-CODE - Tap to Cancel Alarm!",
    headers={
        "Title": "Unauthorized Access Detected!",
        "Priority": "urgent",
        "Click": f'{address}/alarm',

        # Some tags reference specific emojis which appear in messages (can be seen in documentation). 
        # The tag does not refer to an emoji your custom tag will appear as you've written.
        "Tags": "warning, custom tag"
    })

    return 0


def trigger_stop():
    # finds and kills the c++ process you create via ./capstone
    # also sends SIGINT, but by the time this method runs the main loop should be finished though so its okay.
    try:
        subprocess.run(["pkill", "-INT", "capstone"])
        print("Sent SIGINT to C++ process.")
    except Exception as e:
        print(f"Failed to signal C++: {e}")
    return 0
