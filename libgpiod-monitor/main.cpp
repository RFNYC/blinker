#include <iostream>
#include <atomic> // for threadsafe variables (variables shared between threads must be atomic)
#include <csignal> // to safely handle CTRL+C signal interrupt
#include <thread> // to open up additional threads

#include <unistd.h>
#include <gpiod.hpp>

/*
This code is an attempting to explain how to monitor a gpiopins for edge-events (voltage changes) and keep track of the time it took.
Typically you don't want to block your entire program to simply increment a number to keep track of time, so you ideally delegate that task to a
background thread while your main logic remains running. I'll show how to do that here.

fsleep calls inbetween print statements are just for readability they arent needed.
*/

// Start off by making your interrupt variables and your threadsafe counter:
std::atomic<bool> continue_running_1(true);
std::atomic<bool> continue_running_2(true);
std::atomic<int> seconds(0);

// sleep for X seconds
double fsleep(double x){
    return usleep(x * 1000000);
}

// This will be the function the second thread runs while the main thread will run main() as usual.
void begin_counting(){
    std::cout << "Thread 2: Starting monitor timer..." << '\n';
    fsleep(0.1);

    // We'll be using continue_running_2 variable as start/stop condition for thread 2 (we're going to link this to CTRL+C)
    while(continue_running_2){
        std::cout << "Thread 2: " << "Seconds Elapsed = " << seconds << '\n';
        seconds++;

        fsleep(1);
    }
}

// This function will be registered to the CTRL+C interrupt (SIGINT) via <csignal>
// That means instead of terminating the program, CTRL+C will run this function to stop the second thread & clean up before stopping.
void stop_program(int signum){
    std::cout << "Interrupt signal: " << signum << " recieved. Cleaning up..." << '\n';

    // stop both threads, after that any remaining code after the while-loop in main() will run.
    continue_running_2 = false;
    continue_running_1 = false;

}

int main(){

    // Register stop_program() as your CTRL+C function via <csignal>
    std::signal(SIGINT, stop_program);

    // Begin program
    std::cout << "Thread 1: This is the main thread. In here we'll be monitoring gpioline " << "for edge-events indicating a change in voltage." << '\n';

    try {
    
        gpiod::chip main_header("/dev/gpiochip0"); // 40pin header on the rpi4.
        std::cout << "Thread 1: Successfully instantitated chip object." << '\n' << '\n';
        fsleep(1);

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

        std::cout << "Thread 1: Successfully requested pins." << '\n';
        fsleep(1);
        std::cout << "Thread 1: Output pins - " << output_pins.offsets() << '\n';
        std::cout << "Thread 1: Input pins - " << input_pins.offsets() << '\n' << '\n';
        fsleep(1);

        std::cout << "Thread 1: Beginning main loop. (Press CTRL+C to stop.)" << '\n';
        fsleep(1);

        // Begin your second thread timer here:
        std::thread second_thread(begin_counting);

        // We'll be using continue_running_1 as start/stop condition for thread 1 (this is linked to CTRL+C via <csignal>).
        while(continue_running_1){

            // Begin monitoring pins by creating your event buffer (essentially an array to catch and store edge-events)
            gpiod::edge_event_buffer buffer(10); // We'll say it can capture events per loop (100milliseconds).

            // if this method returns true that means an event was caught and the logic below will occur.
            if(input_pins.wait_edge_events(std::chrono::milliseconds(100))){

                // if any events were caught you need to write to the buffer array.
                input_pins.read_edge_events(buffer);

                // now just loop through the buffer array and print out everything we caught.
                // we can also add real-time logic for what to do in response to an event in this loop.
                for (const auto& event : buffer){
                    if (event.type() == gpiod::edge_event::event_type::FALLING_EDGE){
                        std::cout << "Thread 1: Button press detected! (Falling Edge)" << '\n';
                        
                        // At this point we'll stop the second thread since our goal condition has been reached.
                        // However we can still continue listening for more events if we want to.
                        if (continue_running_2 == true) { 
                            std::cout << "First button press detected after: " << seconds << " seconds. Exiting second thread." << '\n';
                            continue_running_2 = false;
                        }

                        // Extra logic to respond to the button press
                        output_pins.set_value(active_buzzer, gpiod::line::value::ACTIVE);
                        output_pins.set_value(red_led, gpiod::line::value::ACTIVE);

                        // Give some time for feedback before reading the next event
                        fsleep(0.3);

                    } else if (event.type() == gpiod::edge_event::event_type::RISING_EDGE){

                        std::cout << "Thread 1: Button was released! (Rising Edge)" << '\n';
                        output_pins.set_value(active_buzzer, gpiod::line::value::INACTIVE);
                        output_pins.set_value(red_led, gpiod::line::value::INACTIVE);

                    } else {

                        std::cout << "An unexpected event type was detected. Please check your code/wiring!" << '\n';

                    }
                }
            }

            usleep(100000);
        }

        // ----- Cleanup -----

        // Release resources after killing main loop.
        output_pins.release();
        input_pins.release();
        main_header.close();

        // MUST JOIN THE SECOND THREAD DURING CLEANUP, FAILURE TO DO SO WILL CAUSE CRASHING.
        second_thread.join();

        std::cout << "Succesfully released all resources. Terminating..." << std::endl;

    } catch (const std::system_error& e) {

        std::cout << "Hardware failed!" << '\n';
        std::cout << "Error: " << e.what() << '\n';
        
        return 1;
    }

    return 0;
}


