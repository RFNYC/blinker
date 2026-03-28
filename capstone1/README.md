# Morse Code Challenge (GPIO + I²C LCD) - VIDEO BELOW

This is a Raspberry Pi C++ program that runs a 30-second Morse code challenge using a button, LEDs, a buzzer, and a 16x2 I²C LCD display.

Users enter Morse code via short and long button presses (dot/dash), which are translated into letters on the LCD. Entering "SOS" within 30 seconds succeeds; otherwise, an HTTP request is sent to my flask server which sends a notification to my phone.

## Hardware Used

This capstone makes use of the gpiopins: 21, 5, 6, 19, and 13.
In these slots are: 
- 1 Button
- 1 Active buzzer
- 3 LEDs
  
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

Unfortunately to make this fit on github I had to compress the video down alot and I removed the audio since I was picking up extra noise.

On startup you are told the name of the challenge, that you will have 30 seconds to complete said challenge, and a final message saying "BEGIN!". After that a timer runs down from 30 seconds on the top line. As the time goes there is a beeping and red light flashing. On the second line is where my input "SOS" was typed out, disabling the timer and "granting" me access.

https://github.com/user-attachments/assets/f436fda7-38fc-4fc8-9998-df701123bc91

