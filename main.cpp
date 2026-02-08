#include <iostream>
#include <unistd.h>
#include <gpiod.hpp> 	// library originally for C so you need to include .hpp header

int main() {

    try {

		gpiod::chip main_header("/dev/gpiochip0"); // 40pin header on the rpi4
		std::cout << "Successfully instantitated chip object." << '\n';

		// info about the 40 pin header
		std::cout << main_header.get_info() << '\n';

		// info about offset 18 (line 18 => gpiopin 18)
		std::cout << main_header.get_line_info(18) << '\n';

		/*
		To do anything with the gpio lines on the main header you need to build a line request.
		You can include line settings for multiple pins at once as part of one request. Just uncomment the extras I left to see that in action.
		A basic line request to blink an LED should have the following:
		---------------------------------------------------------------
		CONSUMER-NAME: the name of the file tampering with a pin at a given moment
		LINE-SETTINGS: the actual line and attributes you want to give to that line
			LINE-NUMBER: BCM # of the pin you'd like to access
			ELECTRICITY-DIRECTION: OUTPUT if you're pushing volts to a pin, INPUT if you're "listening" for volts from a pin.
		---------------------------------------------------------------
		*/

		// THE CODE BELOW DESCRIBES HOW TO BUILD A REQUEST TO MULTIPLE LINES 
		auto line_request = main_header.prepare_request();
		auto line_settings = gpiod::line_settings(); // Initializes the line_settings object with default values.
		unsigned int pin_bcm{18};
		// unsigned int second_pin_bcm{your_num};

		line_request.set_consumer("main.cpp");
		line_request.add_line_settings( pin_bcm, line_settings.set_direction(gpiod::line::direction::OUTPUT)
		// line_request.add_line_settings( second_pin_bcm, line_settings.set_direction(gpiod::line::direction::OUTPUT)

	);
		
		// After this code can make various changes to the requested GPIO pin using the library's methods
		// more info here: https://libgpiod.readthedocs.io/en/latest/cpp_line_request.html#_CPPv4N5gpiod12line_request9set_valueEN4line6offsetEN4line5valueE
		auto gpio_pin = line_request.do_request();
		std::cout << "Successfully requested PIN: " << pin_bcm << '\n';

		// Supply power to the LED and then remove it after 3 seconds twice.
		gpio_pin.set_value(pin_bcm, gpiod::line::value::ACTIVE);
		sleep(3);
		gpio_pin.set_value(pin_bcm, gpiod::line::value::INACTIVE);
		sleep(3);
		gpio_pin.set_value(pin_bcm, gpiod::line::value::ACTIVE);
		sleep(3);
		gpio_pin.set_value(pin_bcm, gpiod::line::value::INACTIVE);

	} catch (const std::system_error& e) {

		std::cout << "Hardware failed!" << '\n';
		std::cout << "Error: " << e.what() << '\n';

		return 1; // code for exiting program with error
	}

}
