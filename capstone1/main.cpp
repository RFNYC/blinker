#include <iostream>
#include <cstdint>     // for unsigned-types
#include <cstdlib>

#include <fcntl.h>     // open files
#include <sys/ioctl.h> // input/output for files [write()]
#include <unistd.h>
#include <linux/i2c-dev.h>

#include "headers/capstone.hpp"
#include "headers/HTTPRequest.hpp"
#include "headers/dotenv.hpp"


int main() {

    const std::string filepath{ "/dev/i2c-1" };
    const int i2c_adapter = open(filepath.c_str(), O_RDWR);

    if (i2c_adapter < 0){
        std::cout << "Failed to open bus." << std::endl;
        exit(1);
    }

    // i2c slave adress (the LCD screen) in my case is 27 (do `i2cdetect -y 1` in terminal to check)
    const uint8_t lcd_address = 0x27;
    if(ioctl(i2c_adapter, I2C_SLAVE, lcd_address) < 0) {
        std::cout << "Failed to reach I2C slave." << std::endl;
    }

    LCD1602 lcd(i2c_adapter, lcd_address);

    lcd.wake_up();
    fsleep(1);

    lcd.send_msg("HELLO WORLD!");

    fsleep(5);
    lcd.shutdown();



    /*
    Extra libraries used for this section:
    dotenv.h, HTTPRequest.h
    https://github.com/laserpants/dotenv-cpp
    https://github.com/elnormous/HTTPRequest
    */
    try {
        // This code makes a post request to flask to send the code 10.
        dotenv::init();
        const auto address = std::getenv("NOTIFICATION_ADDRESS");
        http::Request request{address};

        const std::string body = "10";
        const auto response = request.send("POST", body, {
            {"Content-Type", "application/json"}
        });
        std::cout << std::string{response.body.begin(), response.body.end()} << '\n'; // print the result
    }
    catch (const std::exception& e)
    {
        std::cerr << "Request failed, error: " << e.what() << '\n';
    }

    std::cout << "Closing in 5 seconds..." << '\n';
    fsleep(5);
    std::cout << "Finished." << std::endl;

    return 0;
}