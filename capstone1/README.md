# Morse Code Challenge (GPIO + I²C LCD) - VIDEO BELOW

This is a Raspberry Pi C++ program that runs a 30-second Morse code challenge using a button, LEDs, a buzzer, and a 16x2 I²C LCD display.

Users enter Morse code via short and long button presses (dot/dash), which are translated into letters on the LCD. Entering "SOS" within 30 seconds succeeds; otherwise, an HTTP request is sent to my flask server which sends a notification to my phone.

## Hardware Used

GPIO Pins (BCM):

| Pin | Device |
|----|----|
| 21 | Button |
| 5  | Buzzer |
| 6  | Red LED |
| 19 | White LED |
| 13 | Green LED |

Additional hardware:

- LCD1602 with PCF8574 I²C backpack (address 0x27)

## Learning goals:
The project demonstrates:

- GPIO **edge event monitoring** using `libgpiod`
- **timing-based input interpretation** (dot/dash detection)
- a **custom I²C LCD1602 driver** implemented in userspace
- **hardware feedback** via LEDs and buzzer
- **HTTP notifications** using a simple request library
- safe shutdown using **SIGINT (CTRL+C)**

## Build

```
make
```

## Execute

```
./capstone
```

## Clean

```
make clean
```

## Demo

coming soon