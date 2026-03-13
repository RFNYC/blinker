from flask import Flask
from flask import request
from methods import trigger_stop, send_notification

app = Flask(__name__)

@app.route("/")
def Index():
    return "<pIndex</p>"

# We're only expecting an error code from the C++ code here, only 'POST' is necessary
@app.route("/notification", methods=['POST'])
def push_notification():
    if request.method == 'POST':
        response_code = request.get_data().decode()
        if response_code == '401':
            print("Data Recieved: ", response_code)
            print("Sending Warning Notification...")
            send_notification()
        elif response_code == '200':
            print("Data Recieved: ", response_code)
            print("Final key was accepted. No action required.")
        else:
            print("Route: notification - Unexpected HTTP-request recieved.")

    return "<p>Attempting to send notification...</p>"

# Since we expect the user to be clicking a link which redirects them to the endpoint it will be classified as a 'GET' request
# We'll use this to trigger the stopping function
@app.route("/alarm", methods=['GET'])
def stop_alarm():

    if request.method == 'GET':
        print("Stopping Alarm...")
        trigger_stop()
    else:
        print("Route: alarm - Unexpected HTTP-request recieved.")

    return "<p>Attempting to stop alarm.</p>"

if __name__ == "__main__":
        app.run(host='0.0.0.0', port=5000) # Listen on all interfaces, port 5000