from flask import Flask
from flask import request
from methods import trigger_stop, send_notification

# TODO: Fix server refusing to connect over mobile

app = Flask(__name__)

@app.route("/")
def Index():
    return "<pIndex</p>"

# We're only expecting an error code from the C++ code here, only 'POST' is necessary
@app.route("/notification", methods=['POST'])
def push_notification():
    if request.method == 'POST':
        error_code = request.get_data().decode()
        if error_code == '10':
            print("Sending Notification...")
            send_notification()
        else:
            print("Unexpected POST request data recieved. Please check your code.")
            print("Data Recieved: ", error_code)
    else:
        print("route: notification - Unexpected HTTP-request recieved.")

    return "<p>Attempting to send notification...</p>"

# Since we expect the user to be clicking a link which redirects them to the endpoint it will be classified as a 'GET' request
# We'll use this to trigger the stopping function
@app.route("/alarm", methods=['GET'])
def stop_alarm():

    if request.method == 'GET':
        print("Stopping Alarm...")
        trigger_stop()
    else:
        print("route: alarm - Unexpected HTTP-request recieved.")

    return "<p>Attempting to stop alarm.</p>"