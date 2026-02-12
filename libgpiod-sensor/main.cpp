#include <iostream>

#include <unistd.h>
#include <gpiod.hpp> // library originally for C so you need to include .hpp version of it


/*
TODO:
THIS EXAMPLE IS GOING TO BE ABOUT HOW TO KEEP TRACK A GPIOLINES ELECTRICAL STATE
https://cdn.awsli.com.br/945/945993/arquivos/HCSR04.pdf <- include 

- Finish circuit
- Finish event reading logic
- Explain what the buffer is
*/

int main() {

    try {

		gpiod::chip main_header("/dev/gpiochip0"); // 40pin header on the rpi4
		std::cout << "Successfully instantitated chip object." << '\n';

		/*
		To do anything with the gpio lines on the main header you need to build a line request.
		You can include line settings for multiple pins at once as part of one request.
		A basic line request object to an INPUT component should have the following:
		----------------------------------------------------------------------------------------------------------------------
		CONSUMER-NAME: the name of the file tampering with a pin at a given moment.
		LINE-SETTINGS: the actual line and attributes you want to give to that line.
			LINE-NUMBER: BCM # of the pin you'd like to access.
			ELECTRICITY-DIRECTION: OUTPUT if you're pushing volts to a pin, INPUT if you're listening for a signal from a pin.
			EDGE-DETECTION: BOTH-WAYS | Rising & Falling edge events are how you track electrical state. Rising means voltage
									  |	is going up from zero and Falling means voltage is going down to zero.
		----------------------------------------------------------------------------------------------------------------------
		To use edge events you need to input request object to watch for any status changes for a duration of time.
		If any occur you want to be able to read them so you will create a buffer and then read from it.

        For the purposes of this example I will build two requests: a request my outputs, and a request for my inputs
		*/

        // ---- MY ACTIVE GPIO PINS ----
        // inputs (listening for a signal)
        unsigned int sensor_echo{24};

        // outputs (pushing voltage to them)
        unsigned int sensor_trigger{18}; 
		unsigned int active_buzzer{6};
        unsigned int red_led{16};


		// THE CODE BELOW DESCRIBES HOW TO BUILD A REQUEST FOR MULTIPLE OUTPUT LINES
		gpiod::request_builder outputs_request = main_header.prepare_request();
		gpiod::line_settings line_settings = gpiod::line_settings(); // Initializes a line_settings object with default values.
		outputs_request.set_consumer("main.cpp_outputs");
		outputs_request.add_line_settings(sensor_trigger, line_settings.set_direction(gpiod::line::direction::OUTPUT));
		outputs_request.add_line_settings(active_buzzer, line_settings.set_direction(gpiod::line::direction::OUTPUT));
		outputs_request.add_line_settings(red_led, line_settings.set_direction(gpiod::line::direction::OUTPUT));


        // THE CODE BELOW DESCRIBES HOW TO BUILD A REQUEST FOR AN INPUT LINE
        gpiod::request_builder inputs_request = main_header.prepare_request();
		inputs_request.set_consumer("main.cpp_inputs");
		inputs_request.add_line_settings(sensor_echo, line_settings.set_direction(gpiod::line::direction::INPUT));
		inputs_request.add_line_settings(sensor_echo, line_settings.set_edge_detection(gpiod::line::edge::BOTH));


		// After running do_request(), you can make various changes to the requested GPIO pins using the library's methods via this object.
		gpiod::line_request output_pins = outputs_request.do_request();
    	gpiod::line_request input_pins = inputs_request.do_request();

		std::cout << "Successfully requested pins." << '\n' << std::endl;
		std::cout << "Output pins: " << output_pins.offsets() << '\n';
		std::cout << "Input pins: " << input_pins.offsets() << '\n' << '\n';

		/*
		The main loop is going to work like this:

		My input component is an HC-SR04 proximity sensor. To start a distance measurement, I supply the trig pin with power for 10 microseconds.
		The component sends out a ultrasonic burst and sets the echo pin (my input) to high. I am going to measure how long this state lasts. 
		When the burst is reflected back to the component (bouncing off an object in close proximity) the component will set the echo pin 
		back to its original state (low). 

		I'm going to measure how long the pin is set to high by watching for two time stamps, one when the pin is set to high and another when set to low.
		Then I will subtract the first timestamp from the second to find out the total time the pin was in the high state. 

		To find the distance of the object I will use this formula:
		DISTANCE(cm) = DURATION(microseconds) * SPEED OF SOUND / 2
		This distance will then be printed into the console and used in the logic for whether or not the active buzzer and LED should turn on.
		*/

		// THE CODE BELOW DESCRIBES HOW TO READ ELECTRICAL STATE CHANGES FROM WATCHED GPIO PINS (YOUR INPUTS)
		// create a gpiod buffer
		gpiod::edge_event_buffer buffer(4); // buffer event capacity

		// send voltage for 10 microseconds
		output_pins.set_value(sensor_trigger, gpiod::line::value::ACTIVE);
		usleep(10);
		output_pins.set_value(sensor_trigger, gpiod::line::value::INACTIVE);

		// event reading logic
		if (input_pins.wait_edge_events(std::chrono::milliseconds(100))) {
			
			// ...

		} else {
			std::cout << "Timeout: No echo received. Check your wiring/resistors!" << std::endl;
		}

		// Once you're done working with the pins make sure to close up shop.
		output_pins.release();
		input_pins.release();
		main_header.close();

	} catch (const std::system_error& e) {

		std::cout << "Hardware failed!" << '\n';
		std::cout << "Error: " << e.what() << '\n';

		return 1;
	}

	return 0;
}
