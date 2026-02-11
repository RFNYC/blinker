#pragma once

#include <unistd.h>
#include <gpiod.hpp> // library originally for C so you need to include .hpp header

/* 
the only way to get a fraction of a second with this header is by using usleep() which only takes microseconds and 
for some reason unistd.h doesnt provide any sleep functions in between seconds and microseconds so uhhhhhh yeah! :D

fsleep allows sleeping for a fractional number of seconds.
*/
double fsleep(double seconds) {
    return usleep(seconds * 1000000);
}

// ---- EXAMPLES ---- 
namespace modes {

    // All LEDs will blink 5 times.
    void blink(unsigned int* pin_bcm, unsigned int* second_pin_bcm, unsigned int* third_pin_bcm, gpiod::line_request* gpio_pins) {

        for (int i = 0; i < 5; i++) {
                // gpio::line is a namespace for stuff you can do with each gpioline.
                // more info here on this object here: https://libgpiod.readthedocs.io/en/latest/cpp_line_request.html#_CPPv4N5gpiod12line_request9set_valueEN4line6offsetEN4line5valueE

                gpio_pins->set_value(*pin_bcm, gpiod::line::value::ACTIVE); // turns on the pin
                gpio_pins->set_value(*second_pin_bcm, gpiod::line::value::ACTIVE);
                gpio_pins->set_value(*third_pin_bcm, gpiod::line::value::ACTIVE);
                fsleep(0.5);

                gpio_pins->set_value(*pin_bcm, gpiod::line::value::INACTIVE); // turns off the pin
                gpio_pins->set_value(*second_pin_bcm, gpiod::line::value::INACTIVE);
                gpio_pins->set_value(*third_pin_bcm, gpiod::line::value::INACTIVE);
                fsleep(0.5);
            }

        std::cout << "Program Finished." << std::endl;
    }

    // LEDS will alternate between lighting up the inner 18th pin and pins 24-25.
    void alternate(unsigned int* pin_bcm, unsigned int* second_pin_bcm, unsigned int* third_pin_bcm, gpiod::line_request* gpio_pins) {

        for (int i = 0; i < 5; i++) {
                gpio_pins->set_value(*pin_bcm, gpiod::line::value::ACTIVE);
                gpio_pins->set_value(*second_pin_bcm, gpiod::line::value::INACTIVE);
                gpio_pins->set_value(*third_pin_bcm, gpiod::line::value::ACTIVE);
                fsleep(0.5);
                gpio_pins->set_value(*pin_bcm, gpiod::line::value::INACTIVE);
                gpio_pins->set_value(*second_pin_bcm, gpiod::line::value::ACTIVE);
                gpio_pins->set_value(*third_pin_bcm, gpiod::line::value::INACTIVE);
                fsleep(0.5);
            }
        
        gpio_pins->set_value(*pin_bcm, gpiod::line::value::INACTIVE);
        gpio_pins->set_value(*second_pin_bcm, gpiod::line::value::INACTIVE);
        gpio_pins->set_value(*third_pin_bcm, gpiod::line::value::INACTIVE);
        
        std::cout << "Program Finished." << std::endl;
    }

    void wave(unsigned int* pin_bcm, unsigned int* second_pin_bcm, unsigned int* third_pin_bcm, gpiod::line_request* gpio_pins) {
        for (int i = 0; i < 5; i++) {
                gpio_pins->set_value(*pin_bcm, gpiod::line::value::ACTIVE);
                fsleep(0.1);
                gpio_pins->set_value(*second_pin_bcm, gpiod::line::value::ACTIVE);
                fsleep(0.1);
                gpio_pins->set_value(*third_pin_bcm, gpiod::line::value::ACTIVE);

                fsleep(0.2);

                gpio_pins->set_value(*pin_bcm, gpiod::line::value::INACTIVE);
                fsleep(0.1);
                gpio_pins->set_value(*second_pin_bcm, gpiod::line::value::INACTIVE);
                fsleep(0.1);
                gpio_pins->set_value(*third_pin_bcm, gpiod::line::value::INACTIVE);
            }

            std::cout << "Program Finished." << std::endl;
    }

}