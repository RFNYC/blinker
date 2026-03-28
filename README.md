# C++ libgpiod examples

This repo is intended to show how to use [libgpiod](https://libgpiod.readthedocs.io/en/latest/) in C++. I found resources on how to use the library in C, but there aren't many on using it in C++, so I hope to build up this repo to help my own understanding and help out others.

For all examples, I used `gpiochip0` to control the main 40-pin header of the Raspberry Pi 4 and all notes are written with this context in mind. 
*Any files flagged `DEMO` have a video or gif showing hardware*

# Examples
1. **capstone1**: Morse Code Challenge + alarm system & push notifications. Features timing-based input (dot/dash), a custom I²C LCD1602 driver, and HTTP notifications. `(DEMO)`

2. **libgpiod-led**: Output to 3 LEDs. `(DEMO)`
3. **libgpiod-monitor-polling**: Watch for edge events from a GPIO pin using a polling method.
4. **libgpiod-monitor-thread**: Watch for edge events from a GPIO pin on a separate thread.
5. **libgpiod-sensor**: Output to an active buzzer and LED via sensor input. `(DEMO)`

# Need to know (libgpiod)

To interact with the GPIO pins on the main header, you need to build a **line request** (or batch line request). You can include line settings for multiple pins at once as part of one request. 

A basic line request object should have the following:

* **CONSUMER-NAME**: The name of the thread/file tampering with a pin at a given moment.  
* **LINE-SETTINGS**: The actual line and attributes you want to give to that line.
    * **LINE-NUMBER**: BCM number of the pin you'd like to access.
    * **DIRECTION**: `OUTPUT` if you're pushing volts to a pin, `INPUT` if you're listening for a signal.
    * **EDGE-DETECTION**: `BOTH-EDGES`, `RISING-EDGE`, or `FALLING-EDGE`. This is how you track electrical state changes.

### Working with Edge Events
To use edge events, you need a request object to watch for status changes over a duration of time. If a change occurs, you must create a buffer to read the events (shown best in the `monitor` examples).

* **Rising Edge**: Usually the moment a button is pressed (0V to 3.3V).
* **Falling Edge**: Usually the moment a button is released (3.3V to 0V).  
*(Note: This assumes a Pull-Down resistor setup. If using Pull-Up, the logic is inverted!)*

## Other
Inspired by [starnight's repo](https://github.com/starnight/libgpiod-example?tab=readme-ov-file) featuring libgpiod examples for C.
