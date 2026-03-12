#include <iostream>
#include <chrono> // time handling tools

#include <unistd.h>
#include <gpiod.hpp>

/*
This code is an attempting to explain how to monitor a gpiopins for edge-events (voltage changes) via polling using std::chrono
*/

// sleep for X seconds
double fsleep(double x){
    return usleep(x * 1000000);
}

int main(){

    try {
    
        gpiod::chip main_header("/dev/gpiochip0"); // 40pin header on the rpi4.
        std::cout << "Successfully instantitated chip object." << '\n' << '\n';

        // ---- MY ACTIVE GPIO PINS ----
        // inputs (listening for a signal)
        const unsigned int button{21};

        // outputs (pushing voltage to them)
        const unsigned int active_buzzer{5};
        const unsigned int red_led{6};

        // THE CODE BELOW DESCRIBES HOW TO BUILD A REQUEST FOR MULTIPLE OUTPUT LINES.
        gpiod::request_builder outputs_request = main_header.prepare_request();
        gpiod::line_settings default_settings = gpiod::line_settings();
        outputs_request.set_consumer("Thread 1");
        outputs_request.add_line_settings(active_buzzer, default_settings.set_direction(gpiod::line::direction::OUTPUT));
        outputs_request.add_line_settings(red_led, default_settings.set_direction(gpiod::line::direction::OUTPUT));

        // THE CODE BELOW DESCRIBES HOW TO BUILD A REQUEST FOR AN INPUT LINE.
        gpiod::request_builder inputs_request = main_header.prepare_request();
        gpiod::line_settings input_settings =  gpiod::line_settings();
        inputs_request.set_consumer("Thread 1");
        inputs_request.add_line_settings(button, input_settings.set_direction(gpiod::line::direction::INPUT));
        inputs_request.add_line_settings(button, input_settings.set_edge_detection(gpiod::line::edge::BOTH));

        // This tethers the button to the high state, more explanation needed write notes on this.
        // KEEPS PIN AT 3.3V UNLESS INTERACTED WITH.
        input_settings.set_bias(gpiod::line::bias::PULL_UP);
        inputs_request.add_line_settings(button, input_settings);
        
        // After running do_request(), you can make various changes to the requested GPIO pins using the library's methods via this line_request object.
        // methods for this object can be found here --> https://libgpiod.readthedocs.io/en/latest/cpp_line_request.html
        gpiod::line_request output_pins = outputs_request.do_request();
        gpiod::line_request input_pins = inputs_request.do_request();


        // Polling
        std::chrono::time_point last_poll{ std::chrono::steady_clock::now() }; 

        // This whole thing will run every 50ms, the polling will ask for every 1000ms
        while(true){

            std::chrono::time_point current_iteration{ std::chrono::steady_clock::now() };

            // Normally subtracting time_points returns an object based in nanoseconds. We need to use chrono's typecast to get it in ms.
            auto time_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(current_iteration - last_poll);

            // .count() returns the actual time passed as a number
            if(time_elapsed_ms.count() > 3000) {
                std::cout << "Poll: Three seconds have passed." << '\n';
                std::cout << "Your extra polling logic would occur here while other monitoring logic would occur outside this if statement." << '\n';

                // Update the poll timestamp
                last_poll = current_iteration;
            };

            // Begin monitoring pins by creating your event buffer (essentially an array to catch and store edge-events)
            gpiod::edge_event_buffer buffer(10); // We'll say it can capture events per 100 milliseconds.

            // if this method returns true that means an event was caught and the logic below will occur.
            if(input_pins.wait_edge_events(std::chrono::milliseconds(100))){

                // if any events were caught you need to write to the buffer array.
                input_pins.read_edge_events(buffer);

                // now just loop through the buffer array and print out everything we caught.
                // we can also add real-time logic for what to do in response to an event in this loop.
                for (const auto& event : buffer){
                    if (event.type() == gpiod::edge_event::event_type::FALLING_EDGE){
                        std::cout << "Monitor: Button press detected! (Falling Edge)" << '\n';
                        
                        // Extra logic to respond to the button press
                        output_pins.set_value(active_buzzer, gpiod::line::value::ACTIVE);
                        output_pins.set_value(red_led, gpiod::line::value::ACTIVE);

                        // Give some time for feedback before reading the next event
                        fsleep(0.3);

                    } else if (event.type() == gpiod::edge_event::event_type::RISING_EDGE){

                        std::cout << "Monitor: Button was released! (Rising Edge)" << '\n';
                        output_pins.set_value(active_buzzer, gpiod::line::value::INACTIVE);
                        output_pins.set_value(red_led, gpiod::line::value::INACTIVE);

                    } else {

                        std::cout << "An unexpected event type was detected. Please check your code/wiring!" << '\n';

                    }
                }
            }

            // 50ms
            usleep(50000);
        }

        output_pins.release();
        input_pins.release();
        main_header.close();

        std::cout << "Succesfully released all resources. Terminating..." << std::endl;

    } catch (const std::system_error& e) {

        std::cout << "Hardware failed!" << '\n';
        std::cout << "Error: " << e.what() << '\n';
        
        return 1;
    }

    return 0;
}


