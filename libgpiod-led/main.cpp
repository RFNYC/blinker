#include <iostream>
#include <unistd.h>
#include <gpiod.hpp> // library originally for C so you need to include .hpp version of it
#include "blink_modes.hpp"

int main() {
    try {
        gpiod::chip main_header("/dev/gpiochip0"); // 40pin header on the rpi4
        std::cout << "Successfully instantitated chip object." << '\n';

        // info about the 40 pin header
        std::cout << main_header.get_info() << '\n';

        /*
        To do anything with the gpio lines on the main header you need to build a line request.
        You can include line settings for multiple pins at once as part of one request.
        A basic line request to blink an LED should have the following:
        ----------------------------------------------------------------------------------------------------------------------
        CONSUMER-NAME: the name of the file tampering with a pin at a given moment.
        LINE-SETTINGS: the actual line and attributes you want to give to that line.
            LINE-NUMBER: BCM # of the pin you'd like to access.
            ELECTRICITY-DIRECTION: OUTPUT if you're pushing volts to a pin, INPUT if you're listening for a signal from a pin.
        ----------------------------------------------------------------------------------------------------------------------
        */

        // THE CODE BELOW DESCRIBES HOW TO BUILD A REQUEST WITH MULTIPLE GPIO LINES 
        gpiod::request_builder line_request = main_header.prepare_request();
        gpiod::line_settings line_settings = gpiod::line_settings(); // Initializes a line_settings object with default values.

        unsigned int pin_bcm{18};
        unsigned int second_pin_bcm{24};
        unsigned int third_pin_bcm{25};

        line_request.set_consumer("main.cpp");
        line_request.add_line_settings(pin_bcm, line_settings.set_direction(gpiod::line::direction::OUTPUT));
        line_request.add_line_settings(second_pin_bcm, line_settings.set_direction(gpiod::line::direction::OUTPUT));
        line_request.add_line_settings(third_pin_bcm, line_settings.set_direction(gpiod::line::direction::OUTPUT));

        // After running do_request() you can make various changes to the requested GPIO pins using the library's methods.
        // See blink_modes.hpp for more info.
        gpiod::line_request gpio_pins = line_request.do_request();

        std::cout << "Successfully requested pins. Ready for input." << '\n' << std::endl;
        std::cout << "------ BLINK-MODE OPTIONS ------" << '\n';
        std::cout << "1 - Blink all LEDs 5 times." << '\n'
                  << "2 - LEDs will alternate between the inner light and the outer lights." << '\n'
                  << "3 - LEDs will light up and turn off in a wave pattern." << std::endl;

        int userInput{0};

        while(userInput > 3 || userInput < 1) {
            std::cout << "Please press any of the keys listed above: " << '\n';
            std::cin >> userInput;

            if (userInput == 1) {
                std::cout << "Running Example 1." << '\n' << "Blinking LEDs..." << '\n';
                modes::blink(&pin_bcm, &second_pin_bcm, &third_pin_bcm, &gpio_pins);
            } else if (userInput == 2) {
                std::cout << "Running Example 2." << '\n' << "Alternating LEDs..." << '\n';
                modes::alternate(&pin_bcm, &second_pin_bcm, &third_pin_bcm, &gpio_pins);
            } else if (userInput == 3) {
                std::cout << "Running Example 3." << '\n' << "Waving LEDs..." << '\n';
                modes::wave(&pin_bcm, &second_pin_bcm, &third_pin_bcm, &gpio_pins);
            } else {
                std::cout << "Something was wrong with your input, you likely entered a weird float value or a letter." << '\n';
            }
        }

        // Once you're done working with the pins make sure to close up shop.
        gpio_pins.release()
        main_header.close();

    } catch (const std::system_error& e) {
        std::cout << "Hardware failed!" << '\n';
        std::cout << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}