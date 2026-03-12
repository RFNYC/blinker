# libgpiod event-monitor example: polling

In this code I'm going to have a loop that monitors for edge-events that tell us if the button has been pressed. The idea here is that for every iteration of the loop we'll check for a button press only on certain other iterations will some extra logic run. This will be achieved by using std::chrono for polling. This is what I'm going to use for my capstone example.

This code makes use of GPIO Pins 5, 6, 21, (BCM) but you can put your own values to test it with your circuit.
My circuit is setup such that my button idles high and on press should lower the voltage, (We'll be listening for a falling edge event).

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
