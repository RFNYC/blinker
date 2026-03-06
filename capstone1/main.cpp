#include <iostream>
#include <cstdint>     // for unsigned-types
#include <cstdlib>

#include <fcntl.h>     // open files
#include <sys/ioctl.h> // input/output for files [write()]
#include <unistd.h>
#include <linux/i2c-dev.h>

#include "headers/bin-cmds.hpp"
#include "headers/HTTPRequest.hpp"
#include "headers/dotenv.hpp"

/* 
PCF875 Microcontroller docs --> https://cdn.sparkfun.com/assets/9/5/f/7/b/HD44780.pdf | Table 6 (pg. 23-25), Table 12 (pg. 42), Wakeup (pg. 46)
PCF875 SCHEMATIC --> https://github.com/HvandeVen/PCF8574-Display/blob/master/I2C%20LCD%20Adapter%20Schematic.pdf

PIN ROLE
P0: RS |  0 = Sending a Command; 1 = Sending Data (Text)  
P1: RW |  Read/Write. We almost always leave this bit set to 0 (Write)
P2: EN |  The LCD only looks at the other pins when this flips from 1 to 0
       |  Whatever instructions you send will only be applied after this pin flips so we have to turn it on and then off.
P3: BL |  1 = BackLight ON; 0 = BackLight OFF

These 4 databits can be used so send whatever information we want.
P4: D4 | Data Bit 4
P5: D5 | Data Bit 5
P6: D6 | Data Bit 6
P7: D7 | Data Bit 7

These 8 pins represent a byte which we're using to communicate. Obviously 1 means ON, 0 means OFF.
Byte arrangement: D7 D6 D5 D4 BL EN RW RS --> when we bit shift left the data bits are the first to fall off.

When I refer to any pages/tables im talking about the microcontroller documentation.
*/

// We're abstracting this so we can use bit operations easier.
// Learning hexadecimal format (an abstraction for binary) just to abstract it behind a variable is stupid so im using base2, sue me.

// MANAGEMENT PINS
const uint8_t PIN0_RS{ 0b00000001 };
const uint8_t PIN1_RW{ 0b00000010 }; // usually remains 0
const uint8_t PIN2_EN{ 0b00000100 }; 
const uint8_t PIN3_BL{ 0b00001000 }; // 1 for ON, 0 for OFF

// DATA PINS
const uint8_t PIN4_D4{ 0b00010000 };
const uint8_t PIN5_D5{ 0b00100000 };
const uint8_t PIN6_D6{ 0b01000000 };
const uint8_t PIN7_D7{ 0b10000000 };


// This function makes use of the write() so it will expect the i2c-adapter file, an 8bit address to write to, and a number of bytes to read.
// In our case pin_state will represent the state of the 8 pins of the backpack. Each pin_state is just a snapshot of the last instruction byte.
void edit_pins(int i2c_adapter, uint8_t pin_state){

    // Stage our instructions by flipping the pin up:
    uint8_t up_instruction = PIN2_EN | pin_state;
    write(i2c_adapter, &up_instruction, 1);
    usleep(500);

    // Confirm instructions by flipping the pin down:
    uint8_t down_instruction = pin_state;
    write(i2c_adapter, &down_instruction, 1);
    usleep(500);

}

// use this after LCD is set to 4-bit
void send_byte(int i2c_adapter, uint8_t value, uint8_t rs_mode){

    // rs_mode = 0 if sending a command, rs_mode = 1 if sending data (letters)

    /* 
    Bitshifting logic:
    We need the first four bits to be clear at all times for our management bits to fall in nicely.
    This means we need to split up our letters into 4bit chunks and then send them one after the other.

    Example: H = 0b01001000 we're gonna turn that into 0100 1000 by excluding the first four bits and sending the management bits their place.
    Then once we flip the pin on we'll do it again. This time we'll shift the first four bits resulting in 1000 0000 and then management bits.
    Then we'll flip the enable pin back down and that should complete the message.

    The LCD has an internal system somewhere which understands that when its running in 4-bit mode it needs to wait for two distinct
    write calls. It has an 8-bit brain so 4 databits isn't enough, it will literally wait until the next 4 databits come through and piece
    those bits together into one byte which it can then display. It executes whatever function the management bits 
    */

    // let value = 0100 1000:
    // exclude first four bits, then editpins(high), bitshift four bits to the left and exclude first four again then editpins(low)
    // uint8_t my_val{ 0b01001000 };

    uint8_t high_instruction = (value & 0b11110000) | PIN3_BL | rs_mode;
    edit_pins(i2c_adapter, high_instruction);

    uint8_t low_instruction = ((value << 4) & 0b11110000) | PIN3_BL | rs_mode;
    edit_pins(i2c_adapter, low_instruction);

}

int main() {

    // Start off by initializing your devices, I2C adapter first and then the slave.
    // For me its going to be the RPI4 so we need to ask it permission to access the pins on the main header responsible for I2C.
    const std::string filepath{ "/dev/i2c-1" };
    const int i2c_adapter = open(filepath.c_str(), O_RDWR);

    if (i2c_adapter < 0){
        std::cout << "Failed to open bus." << std::endl;
        exit(1);
    }

    // i2c slave address (the LCD screen) in my case is 27 (do `i2cdetect -y 1` in terminal to see yours)
    const uint8_t lcd_address = 0x27;
    if(ioctl(i2c_adapter, I2C_SLAVE, lcd_address) < 0) {
        std::cout << "Failed to reach I2C slave." << std::endl;
    }

    // After this point we're good to go, now we can use the write function to send instructions.
    // Before we do anything we need to tell the thing to wake up and run in 4 bit mode. Refer to pg46 and then Table-12, Step 3 - Function set
    // They expect this in 2 goes i think? I'm just going to send the instruction to ping pins 4 & 5 three times.
    const uint8_t wake_up{ PIN4_D4 | PIN5_D5 };
    const uint8_t set_4_bit{ PIN5_D5 };
    edit_pins(i2c_adapter, wake_up);
    usleep(500);
    edit_pins(i2c_adapter, wake_up);
    usleep(500);
    edit_pins(i2c_adapter, set_4_bit);

    // Now that the system is running in 4-bit we should refer to table 6 for the rest of our instructions.
    // All of the bytes shown in table 6 represent how the microcontroller will interpret your bytes when RS is set to 0.
    // In other words whenever you want to set a command, set RS to 0 and check table 6 for what you want.

    /*
    After you do that use the send_byte() to send (PAGE 23) -> commands 2, 3, and 4 using RS=1.  (These are startup commands)
    command 1 is the display clear (clear text) command so you'd run that after you figure out how to write text to screen.
    When you aren't sending commands you let RS=0 and in theory the 8-bits you send should represent letters? Look at the ASCII conversion.
    */

    // STARTUP ORDER: check page 23 for the values associated with each byte command shown here
    // 1.) FUNCTION SET
    // 2.) DISPLAY ON/OFF CONTROL
    // 3.) ENTRY MODE SET
    // --- READY FOR USE ---

    send_byte(i2c_adapter, S_FUNCTION_SET, 0);
    send_byte(i2c_adapter, S_DISPLAY_SET, 0);
    send_byte(i2c_adapter, S_CHAR_ENTRY_SET, 0);
    send_byte(i2c_adapter, CLEAR_DISPLAY, 0);

    // Countdown
    uint8_t seconds = 10;

    for (int i = 0; i <= 10; i++){
        send_byte(i2c_adapter, CLEAR_DISPLAY, 0);

        std::string s = std::to_string(seconds);
        for (char c : s) {
            send_byte(i2c_adapter, c, 1); // '1' is the rs_mode for DATA
        }

        seconds--;

        fsleep(1);
    }

    send_byte(i2c_adapter, CLEAR_DISPLAY, 0);

    // print hello world (with two lines)
    std::string message = "Come on come on";
    std::string message2 = "and F.T.W.W.W!";

    for (char character : message){
        send_byte(i2c_adapter, character, 1);
    }

    send_byte(i2c_adapter, GOTO_SECOND_LINE, 0);

     for (char character : message2){
        send_byte(i2c_adapter, character, 1);
    }

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

    std::cout << "Finished instructions. Closing in 5 seconds..." << '\n';
    fsleep(5);
    std::cout << "Closing..." << std::endl;


    // TODO: TURN THESE INTO COMMANDS FOR YOUR HEADER
    send_byte(i2c_adapter, CLEAR_DISPLAY, 0); 
    // Shuts down internal display
    send_byte(i2c_adapter, 0x08, 0);
    // Completely turns off LCD Backlight.
    uint8_t blackout = 0x00;
    write(i2c_adapter, &blackout, 1);

    // Give the OS back its resource
    close(i2c_adapter);

    return 0;
}