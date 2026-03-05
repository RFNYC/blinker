#include <iostream>
#include <cstdint>     // for unsigned-types

#include <fcntl.h>     // open files
#include <sys/ioctl.h> // input/output for files [write()]
#include <unistd.h>
#include <linux/i2c-dev.h>

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
void send_byte(int i2c_adapter, uint8_t pin_state, uint8_t rs_mode){



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
    // Wake have to make it wake up: pg
    // Before we do anything we need to tell the thing to wake up and run in 4 bit mode. Refer to Table-12, Step 3 - Function set
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
    TODO: Now that we're in four bit mode setup a function to send information in 4bit intervals (takes RS=0 and RS=1).
    After you do that use the function to send (PAGE 23) -> commands 2, 3, and 4 using RS=1.   
    command 1 is the display clear (clear text) command so you'd run that after you figure out how to write text to screen.

    When you aren't sending commands you let RS=0 and in theory the 8-bits you send should represent letters? Look at the ASCII conversion.
    */
    

    return 0;
}