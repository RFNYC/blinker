# C++ libgpiod examples

This repo is intended to show how to use [libgpiod](https://libgpiod.readthedocs.io/en/latest/) in C++. I found resources on how to use the library in C but there aren't many on using it in C++ so I hope to build up this repo to help my own understanding and help out others.

For all examples I used gpiochip0 to control the main 40pin header of the raspberrypi-4 and all notes are written in with this context in mind.

# Examples
1. libgpiod-led: Output to 3 LEDs.
2. libgpiod-monitor: Watch for edge-events from a gpiopin on a separate thread.
3. libgpiod-sensor: Output to active buzzer and LED via sensor input.

# Need to know (libgpiod):

To do anything to the gpio pins on the main header you need to build a line request (or batch line request).
You can include line settings for multiple pins at once as part of one request.
A basic line request object should have the following:

CONSUMER-NAME: the name of the thread/file tampering with a pin at a given moment.  
LINE-SETTINGS: the actual line and attributes you want to give to that line.
- LINE-NUMBER: BCM number of the pin you'd like to access.
- ELECTRICITY-DIRECTION: OUTPUT if you're pushing volts to a pin, INPUT if you're listening for a signal from a pin.
- EDGE-DETECTION: BOTH-WAYS | Rising & Falling edge events are how you track electrical state.

To use edge events you need to input request object to watch for any status changes for a duration of time.
If any occur, to read them you will need create a buffer and then read from it. (Shown best in monitor example).

More on edge-events:
- Rising Edge: Usually the moment the button is pressed (0V to 3.3V).
- Falling Edge: Usually the moment the button is released (3.3V to 0V).  
(Note: This assumes a Pull-Down resistor setup. If using Pull-Up, the logic is inverted!)

## Other
Inspired by [starnight's repo](https://github.com/starnight/libgpiod-example?tab=readme-ov-file) with libgpiod examples for C.


