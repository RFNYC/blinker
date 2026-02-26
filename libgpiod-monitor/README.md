# libgpiod event-monitor example: push button

In this code I'm going to have a loop that watches for an edge-event in the main thread and a timer function in another thread.
The goal of the second thread is to monitor the number of seconds between startup and the first event detection, after that the thread stops.
I'll then show how to safely stop the background thread and clean up via CTRL+C without terminating the entire program first.

This code makes use of GPIO Pins 5, 6, 21, (BCM) but you can put your own values to test it with your circuit.
My circuit is setup such that my button idles high and on press should lower the voltage, (We'll be listening for a falling edge event).

Note: You can also do this the other way around and have your gpio monitoring logic in another thread and let your main logic be something else.

## Build
```
make
```

## Execute

```
./monitor-example
```

## Clean
```
make clean
```

## Demo

coming soon
