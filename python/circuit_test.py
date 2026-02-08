import RPi.GPIO as GPIO
import time

# .BCM refers to the GPIO function on legend
# .BOARD refers to the physical board position | will not be using this.
GPIO.setmode(GPIO.BCM)

# SETTING UP LEDS TO BE OUTPUT
GPIO.setup(18, GPIO.OUT)
GPIO.setup(24, GPIO.OUT)
GPIO.setup(25, GPIO.OUT)

for x in range(5):
    GPIO.output(18, GPIO.HIGH)  # Turn on the LED
    GPIO.output(24, GPIO.HIGH)  
    GPIO.output(25, GPIO.HIGH) 
    # WAIT 1 SECOND WHILE ON
    time.sleep(1)

    GPIO.output(18, GPIO.LOW)   # Turn off the LED
    GPIO.output(24, GPIO.LOW)  
    GPIO.output(25, GPIO.LOW) 
    # WAIT 1 SECOND WHILE OFF
    time.sleep(1)

# END WITH THIS
GPIO.cleanup()

