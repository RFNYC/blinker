# libgpiod INPUT example: HC-SRO4 Ultrasonic Sensor

This file intends to explain how to build line requests, listen for **INPUT** from a component, and monitor when a gpio line changes electrical state. In this example I'm going to be using the HC-SR04 Ultrasonic Sensor as my input component. If our sensor detects an object it will send a signal back to the RPI which will activate an active buzzer and a red LED. This should compile on a Linux based system if there is GPIO and you have the required components. 

This code makes use of GPIO Pins 24, 18, 6, 16 (BCM) but you can put your own values to test it with your circuit.
Documentation for the sensor component im using can be found [here.](https://cdn.awsli.com.br/945/945993/arquivos/HCSR04.pdf)

## Build
```
make
```

## Execute

```
./sensor-example
```

## Clean
```
make clean
```

## Demo

Coming soon.

