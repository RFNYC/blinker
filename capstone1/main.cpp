#include <iostream>
#include <cstdint>    // for unsigned-types
#include <cstdlib>
#include <chrono>     // needed for time
#include <atomic>
#include <csignal>

#include <fcntl.h>     // open files
#include <sys/ioctl.h> // input/output for files [write()]
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <gpiod.hpp>

#include "headers/HTTPRequest.hpp"
#include "headers/dotenv.hpp"
#include "headers/capstone.hpp" // my userspace-driver/HAL

std::atomic<bool> continue_loop = true;
void stop_loop(int signum){
    continue_loop = false;
}

// DO NOT USE IN MAIN LOOP. MEANT FOR SMALL FLASHES DURING STARTUP.
void single_beep(gpiod::line_request& outputs, float time, int output_pin){
    outputs.set_value(output_pin, gpiod::line::value::ACTIVE);
    fsleep(time);
    outputs.set_value(output_pin, gpiod::line::value::INACTIVE);
}

int main() {

    // Register stop_loop() as your CTRL+C function via <csignal>
    std::signal(SIGINT, stop_loop);

    // --- LCD ---
    const std::string filepath{ "/dev/i2c-1" };
    const int i2c_adapter = open(filepath.c_str(), O_RDWR);

    if (i2c_adapter < 0){
        std::cout << "Failed to open bus." << std::endl;
        exit(1);
    }

    const uint8_t lcd_address = 0x27;
    if(ioctl(i2c_adapter, I2C_SLAVE, lcd_address) < 0) {
        std::cout << "Failed to reach I2C slave." << std::endl;
    }

    LCD1602 lcd(i2c_adapter, lcd_address);
    std::cout << "Successfully created LCD object" << '\n';

    // We'll use this to keep track of the cursor later.
    int prev_coordinates[2];
    // -----------


    // --- 40-pin Header ---
    gpiod::chip main_header{"/dev/gpiochip0"};
    std::cout << "Successfully created chip object." << '\n' << '\n';

    // gpiopins (bcm)
    const unsigned int button{21};
    const unsigned int active_buzzer{5};
    const unsigned int red_led{6};
    const unsigned int white_led{19};
    const unsigned int green_led{13};

    // prepare gpiopins
    gpiod::request_builder output_request = main_header.prepare_request();
    gpiod::line_settings output_settings = gpiod::line_settings();
    output_request.set_consumer("main.cpp");
    output_request.add_line_settings(red_led, output_settings.set_direction(gpiod::line::direction::OUTPUT));
    output_request.add_line_settings(white_led, output_settings.set_direction(gpiod::line::direction::OUTPUT));
    output_request.add_line_settings(green_led, output_settings.set_direction(gpiod::line::direction::OUTPUT));
    output_request.add_line_settings(active_buzzer, output_settings.set_direction(gpiod::line::direction::OUTPUT));

    gpiod::request_builder input_request = main_header.prepare_request();
    gpiod::line_settings input_settings = gpiod::line_settings();
    input_request.set_consumer("main.cpp");
    input_settings.set_direction(gpiod::line::direction::INPUT);
    input_settings.set_edge_detection(gpiod::line::edge::BOTH);

    // Prevent misreads from switch bouncing up and down after you press the button 
    input_settings.set_debounce_period(std::chrono::milliseconds(10));

    // This tethers the button to the high state, more explanation needed write notes on this.
    // KEEPS PIN AT 3.3V UNLESS INTERACTED WITH.
    input_settings.set_bias(gpiod::line::bias::PULL_UP);
    input_request.add_line_settings(button, input_settings);

    gpiod::line_request outputs = output_request.do_request();
    gpiod::line_request inputs = input_request.do_request();
    // ---------------------

    // FLAG FOR STOPPING HTTP-REQUEST PORTION
    bool testing_script = false;
    
    // Now we can start cooking
    lcd.wake_up();

    // 2 quick green flashes + beeps
    single_beep(outputs, 0.1, active_buzzer);
    single_beep(outputs, 0.2, green_led);
    single_beep(outputs, 0.1, active_buzzer);
    single_beep(outputs, 0.2, green_led);
    fsleep(2);

    single_beep(outputs, 0.1, active_buzzer);
    lcd.send_string("CHALLENGE ACTIVE");
    fsleep(0.5);

    lcd.cursor_pos(0,1);
    single_beep(outputs, 0.1, active_buzzer);
    lcd.send_string("MORSE-CODE");
    fsleep(1);

    lcd.clear();

    single_beep(outputs, 0.1, active_buzzer);
    lcd.send_string("You will have");
    fsleep(0.5);

    lcd.cursor_pos(0,1);
    single_beep(outputs, 0.1, active_buzzer);
    lcd.send_string("30 seconds");
    fsleep(1);

    lcd.clear();

    single_beep(outputs, 0.1, active_buzzer);
    lcd.send_string("BEGIN!");
    fsleep(1);

    // --- TIMER & CHALLENGE LOGIC ---
    lcd.clear();

    // CHARACTERS 12 AND 13 are to be overwritten
    lcd.send_string("Time Left: 30");
    lcd.cursor_pos(0,1);
    lcd.send_string("KEY: ");
    lcd.cursor_pos(4,1);

    // Begin monitoring w/polling
    std::chrono::time_point last_state_check{ std::chrono::steady_clock::now() };

    // We're going to initialize these variables here as well but they will be overwritten if a valid button press has been detected.
    std::chrono::time_point last_rising_edge{ std::chrono::steady_clock::now() };
    std::chrono::time_point last_falling_edge{ std::chrono::steady_clock::now() };

    int timer_value{ 30 };
    bool button_pressed = false;
    int64_t click_duration_ms;

    // Using this string to build up a letter via characters e.g. "." | ".-" | ".--" | ".---"
    // If it matches a letter in the map it will be printed to the screen and this string will be cleared again.
    std::string morse_letter = "";

    // this is where we'll store the status of the key on-screen for validation
    std::string final_key = "";
    bool challenge_passed = false;

    try{
        while(continue_loop) {
            std::chrono::time_point current_iteration{ std::chrono::steady_clock::now() };
            auto time_elapsed_ms = (std::chrono::duration_cast<std::chrono::milliseconds>(current_iteration - last_state_check).count());

            // BY DEFAULT BUZZER WILL BE OFF, FOR EACH ITERATION, ON LCD ITERATIONS IT WILL TURN ON (CREATING A BEEP)
            // outputs.set_value(active_buzzer, gpiod::line::value::INACTIVE);
            outputs.set_value(red_led, gpiod::line::value::INACTIVE);

            // Every 1000ms the LCD will expect an update to the timer as well as a basic iteration
            if(time_elapsed_ms > 1000){
                // outputs.set_value(active_buzzer, gpiod::line::value::ACTIVE);
                outputs.set_value(red_led, gpiod::line::value::ACTIVE);

                if(timer_value == 0){
                    std::cout << "TIME RAN OUT, FINAL KEY SUBMITTED." << std::endl;
                    continue_loop = false;
                } else {
                    last_state_check = current_iteration;
                    
                    // Sending empty spaces to this location clears the last increment of the timer
                    timer_value--;
                    std::string sendable_num = std::to_string(timer_value);
                    lcd.cursor_pos(11, 0);
                    lcd.send_string("  ");
                    lcd.cursor_pos(11, 0);
                    lcd.send_string(sendable_num);

                    if(final_key == "SOS"){
                        std::cout << "CORRECT PASSWORD PASSED, FINAL KEY SUBMITTED." << std::endl;
                        continue_loop = false;
                        challenge_passed = true;
                    }
                }
            }

            /*
                At 5wpm in morse code according to the 1:3:7 rule if the feed is silent for about 1680ms its understood
                as a space between letters. We'll use that logic to print whatevers in the string and clear it without having
                to max out the number of characters.
            */ 
            auto silence_duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_iteration - last_rising_edge).count();

            if (!button_pressed && !morse_letter.empty() && silence_duration > 1680) {
                std::cout << "Silence timeout reached. Sending letter to screen." << '\n';
                std::cout << morse_letter << '\n';
                
                // For the first morse_code print the cursor has to be manually set to the start position. Later positions will increment the previous.
                if(final_key.empty()){
                    lcd.cursor_pos(5, 1);
                    lcd.save_cursor_pos(prev_coordinates);

                    lcd.print_morse(morse_letter);
                    final_key += lcd.char_to_morse(morse_letter);
                } else {
                    int next_x = prev_coordinates[0] + 1;
                    int next_y = prev_coordinates[1];

                    lcd.cursor_pos(next_x, next_y);
                    lcd.save_cursor_pos(prev_coordinates);

                    lcd.print_morse(morse_letter);
                    final_key += lcd.char_to_morse(morse_letter);
                }

                // clear the string for new characters
                morse_letter = "";
            }

            gpiod::edge_event_buffer buffer(10);
            if(inputs.wait_edge_events(std::chrono::milliseconds(10))){
                
                // If any edge events are detected from our inputs write them to the buffer
                inputs.read_edge_events(buffer);

                for (const auto& edge_event : buffer){

                    if (edge_event.type() == gpiod::edge_event::event_type::FALLING_EDGE){
                        std::cout << "Monitor: Button press detected! (Falling Edge)" << '\n';    

                        outputs.set_value(white_led, gpiod::line::value::ACTIVE);
                        last_falling_edge = std::chrono::steady_clock::now();
                        button_pressed = true;


                    } else if (edge_event.type() == gpiod::edge_event::event_type::RISING_EDGE){
                        std::cout << "Monitor: Button was released! (Rising Edge)" << '\n';

                        outputs.set_value(white_led, gpiod::line::value::INACTIVE);
                        last_rising_edge = std::chrono::steady_clock::now();

                        if(button_pressed == true){

                            click_duration_ms = (std::chrono::duration_cast<std::chrono::milliseconds>(last_rising_edge - last_falling_edge).count());
                            std::cout << "Click Duration(ms): " << click_duration_ms << '\n';

                        } else {
                            std::cout << "Click duration was not saved. Invalid button press occured." << std::endl;
                        }
                        
                        // MORSE CODE LOGIC
                        if(morse_letter.size() < 4){
                            if(click_duration_ms < 241){
                                morse_letter += '.';
                            } else if (click_duration_ms > 240) {
                                morse_letter += '-';
                            }
                        
                            if (morse_letter.size() == 4) {
                                std::cout << "Maximum characters recieved, sending letter to screen." << '\n';
                                std::cout << morse_letter << '\n';
                                
                                if(final_key.empty()){
                                    lcd.cursor_pos(6, 1);
                                    lcd.save_cursor_pos(prev_coordinates);

                                    lcd.print_morse(morse_letter);
                                    final_key += lcd.char_to_morse(morse_letter);
                                } else {
                                    int next_x = prev_coordinates[0] + 1;
                                    int next_y = prev_coordinates[1];

                                    lcd.cursor_pos(next_x, next_y);
                                    lcd.save_cursor_pos(prev_coordinates);

                                    lcd.print_morse(morse_letter);
                                    final_key += lcd.char_to_morse(morse_letter);
                                }

                                // clear the string for new characters
                                morse_letter = "";
                            }
                        }

                        button_pressed = false;

                    } else {
                        std::cout << "Unexpected error occured when fetching event type from buffer." << std::endl;
                    }
                }
            }
            // Every 50ms this loop will check for button presses
            usleep(50000);
        }
    } catch (const std::system_error& e) {
        std::cout << "Main loop stopped!" << '\n';
        std::cout << "Interrupted by: " << e.what() << '\n';
    }

    std::cout << "Cleaning up..." << '\n';
    fsleep(0.5);

    outputs.set_value(active_buzzer, gpiod::line::value::INACTIVE);
    outputs.set_value(red_led, gpiod::line::value::INACTIVE);
    lcd.clear();

    if (challenge_passed == true){
        single_beep(outputs, 0.1, active_buzzer);
        lcd.send_string("CHALLENGE PASSED");
        fsleep(0.5);

        lcd.cursor_pos(0,1);
        single_beep(outputs, 0.1, active_buzzer);
        lcd.send_string("Access Granted.");
        fsleep(3);

        // 2 quick green flashes + beeps
        single_beep(outputs, 0.1, active_buzzer);
        single_beep(outputs, 0.2, green_led);
        single_beep(outputs, 0.1, active_buzzer);
        single_beep(outputs, 0.2, green_led);
    } else {

        single_beep(outputs, 0.1, active_buzzer);
        lcd.send_string("CHALLENGE FAILED");
        fsleep(0.5);

        lcd.cursor_pos(0,1);
        single_beep(outputs, 0.1, active_buzzer);
        lcd.send_string("Access Denied.");
        
        // 2 quick red flashes + beeps
        single_beep(outputs, 0.1, active_buzzer);
        single_beep(outputs, 0.2, red_led);
        single_beep(outputs, 0.1, active_buzzer);
        single_beep(outputs, 0.2, red_led);
    }

    std::cout << "Finished Loop. Starting HTTP request..." << '\n';
    std::cout << "Final key recieved: " << final_key << std::endl;
    
    if(testing_script == false){
        try{
            dotenv::init();
            const auto address = std::getenv("NOTIFICATION_ADDRESS");
            http::Request request{address};

            std::string body;
            if(final_key == "SOS"){
                body = "200";
            } else {
                body = "401";
            }

            const auto response = request.send("POST", body, {
                {"Content-Type", "application/json"}
            });
            std::cout << std::string{response.body.begin(), response.body.end()} << '\n';
        }
        catch (const std::exception& e)
        {
            std::cerr << "Request failed, error: " << e.what() << '\n';
        }
    } else {
        std::cout << "HTTP-REQUEST SKIPPED DUE TO TESTING." << std::endl;
    }

    lcd.shutdown();
    inputs.release();
    outputs.release();
    main_header.close();

    std::cout << "Closing program." << '\n';
    fsleep(1); 
    return 0;
}