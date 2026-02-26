#include <iostream>
#include <algorithm>
#include <csignal>
#include <atomic>

#include <unistd.h>
#include <gpiod.hpp> // library originally for C so you need to include .hpp version of it

// THIS EXAMPLE SHOWS HOW TO KEEP TRACK A GPIOLINES ELECTRICAL STATE (RISE/FALL EVENTS) AND USE THAT TO INSTRUCT HARDWARE.
// -----------------------------------------------------------------------------------------------------------------------

// Keeps loop running till we say otherwise, used in place of while(true)... -> while(loopRunning).
std::atomic<bool> loopRunning(true);

// When we press CTRL+C the OS sends an interupt signal (SIGINT) with a number we can use to ID it.
// The entire point of this is so that we have a specific while loop we can turn on and off via CTRL+C without killing the entire program.
void signal_handler(int signum) {
    std::cout << "Interupt Signal: " << signum << " recieved. Cleaning up..." << '\n';

    // kill the loop.
    loopRunning = false;
}

int main() {

    try {

        gpiod::chip main_header("/dev/gpiochip0"); // 40pin header on the rpi4.
        std::cout << "Successfully instantitated chip object." << '\n';

        // ---- MY ACTIVE GPIO PINS ----
        // inputs (listening for a signal)
        const unsigned int sensor_echo{21};

        // outputs (pushing voltage to them)
        const unsigned int sensor_trigger{20};
        const unsigned int active_buzzer{16};
        const unsigned int red_led{17};

        // other
        constexpr double SPEED_OF_SOUND = 0.0343; // cm per microsecond.

        // THE CODE BELOW DESCRIBES HOW TO BUILD A REQUEST FOR MULTIPLE OUTPUT LINES.
        gpiod::request_builder outputs_request = main_header.prepare_request();
        gpiod::line_settings line_settings = gpiod::line_settings(); // Initializes a line_settings object with default values.
        outputs_request.set_consumer("main.cpp_outputs");
        outputs_request.add_line_settings(sensor_trigger, line_settings.set_direction(gpiod::line::direction::OUTPUT));
        outputs_request.add_line_settings(active_buzzer, line_settings.set_direction(gpiod::line::direction::OUTPUT));
        outputs_request.add_line_settings(red_led, line_settings.set_direction(gpiod::line::direction::OUTPUT));

        // THE CODE BELOW DESCRIBES HOW TO BUILD A REQUEST FOR AN INPUT LINE.
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

        // register that kill function.
        std::signal(SIGINT, signal_handler);

        while (loopRunning) {

            /*
            The main loop is going to work like this:

            My input component is an HC-SR04 proximity sensor. To start a distance measurement, I supply the trig pin with power for 10 microseconds.
            The component sends out a ultrasonic burst and sets the echo pin (my input) to high. I am going to measure how long this state lasts.
            When the burst is reflected back to the component (bouncing off an object in close proximity) the component will set the echo pin
            back to its original state (low).

            I'm going to measure how long the pin is set to high by watching for two time stamps, one when the pin is set to high and another when set to low.
            Then I will subtract the first timestamp from the second to find out the total duration the pin was in the high state.

            To find the distance of the object I will use this formula:
            DISTANCE(cm) = DURATION(microseconds) * SPEED OF SOUND / 2.
            This distance will then be printed into the console and used in the logic for whether or not the active buzzer and LED should turn on.
            */

            // THE CODE BELOW DESCRIBES HOW TO READ ELECTRICAL STATE CHANGES (RISE/FALL EVENTS) FROM WATCHED GPIO PINS.
            // This buffer essentially works as an array to store info about any rising/falling edge events that occur.
            gpiod::edge_event_buffer buffer(4); // buffer event capacity.

            // send voltage for 10 microseconds.
            output_pins.set_value(sensor_trigger, gpiod::line::value::ACTIVE);
            usleep(10);
            output_pins.set_value(sensor_trigger, gpiod::line::value::INACTIVE);

            // If the echo pin changes voltage (the method returns true and this runs).
            if (input_pins.wait_edge_events(std::chrono::milliseconds(100))) {

                // Wait just a few milliseconds for the sound to actually come back.
                usleep(30000);
                input_pins.read_edge_events(buffer); // actually reading the buffer now that we expect rise/fall events to be inside.

                /*
                Using a lambda to check if we ever recieved a falling-edge (if we didn't that means no object was found).

                Code below says:
                Check between two pointers: buffer.begin(), and buffer.end(), then read all objects of type gpiod::event_edge at their place in memory.
                If any of them are a falling edge event return false, else return true (no falling edge was found).
                It does this for all objects between those pointers and we can use the truthiness of the bool value to make judgements afterwards.
                */
                bool no_falling_edge = std::none_of( buffer.begin(), buffer.end(), [](const gpiod::edge_event& event) {
                        if (event.type() == gpiod::edge_event::event_type::FALLING_EDGE) {
                            return true;
                        } else {
                            return false;
                        }
                    }
                );

                if (no_falling_edge) {
                    std::cout << "OBJECT IS FAR TOO CLOSE OR NONE WERE FOUND. PLEASE CHECK YOUR CIRCUIT/ENVIRONMENT AND TRY AGAIN." << '\n';

                    // Flicker three times then kill the program.
                    for (unsigned int i = 0; i < 4; i++){
                        output_pins.set_value(red_led, gpiod::line::value::ACTIVE);
                        output_pins.set_value(active_buzzer, gpiod::line::value::ACTIVE);
                        usleep(200000);
                        output_pins.set_value(red_led, gpiod::line::value::INACTIVE);
                        output_pins.set_value(active_buzzer, gpiod::line::value::INACTIVE);
                    }

                    break;

                } else {

                    unsigned int start_time{0};
                    unsigned int end_time{0};

                    for (const auto& event : buffer) {

                        if (event.type() == gpiod::edge_event::event_type::RISING_EDGE){
                            start_time = event.timestamp_ns();
                        } else if (event.type() == gpiod::edge_event::event_type::FALLING_EDGE){
                            end_time = event.timestamp_ns();
                        } else {
                            std::cout << "Unexpected event type. Please check your logic for capturing events." << '\n';
                        }

                        if (start_time > 0 && end_time > 0) {

                            // start and endtimes were returned in nanoseconds, need to convert to microseconds here.
                            unsigned int burst_duration{ (end_time - start_time)/1000 };

                            // DISTANCE(cm) = DURATION(in microseconds) * SPEED OF SOUND / 2.
                            double distance_from_object{ (burst_duration * SPEED_OF_SOUND) / 2 };
                            std::cout<< "Distance: " << distance_from_object << "cm" << '\n';

                            // Activation logic.
                            // If close enough activate the light on its own slow flicker, then with a medium flicker, then a fast flicker.
                            if (distance_from_object > 15 && distance_from_object < 20){

                                output_pins.set_value(red_led, gpiod::line::value::ACTIVE);         // turns on the pin.
                                output_pins.set_value(active_buzzer, gpiod::line::value::ACTIVE);
                                usleep(500000);
                                output_pins.set_value(red_led, gpiod::line::value::INACTIVE);       // turns off the pin.
                                output_pins.set_value(active_buzzer, gpiod::line::value::INACTIVE);

                            } else if (distance_from_object > 5 && distance_from_object < 15){

                                output_pins.set_value(red_led, gpiod::line::value::ACTIVE);
                                output_pins.set_value(active_buzzer, gpiod::line::value::ACTIVE);
                                usleep(200000);
                                output_pins.set_value(red_led, gpiod::line::value::INACTIVE);
                                output_pins.set_value(active_buzzer, gpiod::line::value::INACTIVE);

                            } else if (distance_from_object >= 0 && distance_from_object < 5){

                                output_pins.set_value(red_led, gpiod::line::value::ACTIVE);
                                output_pins.set_value(active_buzzer, gpiod::line::value::ACTIVE);
                                usleep(100000);
                                output_pins.set_value(red_led, gpiod::line::value::INACTIVE);
                                output_pins.set_value(active_buzzer, gpiod::line::value::INACTIVE);

                            } else {

                                // Simply print distance, object is not close enough for a reaction.

                            }
                        }

                    }

                }

            } else {
                std::cout << "No trigger/echo received. Check your wiring/resistors." << std::endl;
            }

            usleep(100000);
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