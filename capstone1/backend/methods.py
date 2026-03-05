import requests, os
from dotenv import find_dotenv, load_dotenv

dotenv_path = find_dotenv()
load_dotenv(dotenv_path)
key = os.getenv("TOPIC")
url = f'https://ntfy.sh/{key}'

# For more information on title, priority, tags, ect. 
# https://fotos.esferas.org/docs/publish/ 

def send_notification():

    requests.post(url,
    data="CHALLENGE FAILED: MORSE-CODE - Tap to Cancel Alarm!",
    headers={
        "Title": "Unauthorized Access Detected!",
        "Priority": "urgent",
        "Click": "http://127.0.0.1:5000/alarm",

        # Some tags reference specific emojis which appear in messages (can be seen in documentation). 
        # The tag does not refer to an emoji your custom tag will appear as you've written.
        "Tags": "warning, custom tag"
    })

    return 0

def trigger_stop():
    print("alarm stopped")
    return 0