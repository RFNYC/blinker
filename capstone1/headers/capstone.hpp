#ifndef CAPSTONE_HPP
#define CAPSTONE_HPP

#include <cstdint>
#include <unistd.h>

// OTHER
double fsleep(double seconds) {
    return usleep(seconds * 1000000);
}

/* 
PCF875 Microcontroller docs --> https://cdn.sparkfun.com/assets/9/5/f/7/b/HD44780.pdf | Table 6 (pg. 23-25), Table 12 (pg. 42), Wakeup (pg. 46)
PCF875 SCHEMATIC --> https://github.com/HvandeVen/PCF8574-Display/blob/master/I2C%20LCD%20Adapter%20Schematic.pdf
PIN ROLES
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
*/

// To avoid overbloating main im gonna define some abstractions which should almost never change here.
// I'm going to denote any startup version of a byte with S_... so variants can be defined later.
// shutdown commands will be denoted with K_... for kill x feature.

/*
1. Display clear => 0000 0001

2. Function set:
DL = 1; 8-bit interface data, 0 = 4-Bit interface data (we're using 4-bit due to pin limitations)
N = 0; 1-line display, 1 = 2-line display
F = 0; 5 × 8 dot character font (standard keep this)

3. Display on/off control:
D = 0; Display off, D = 1 Display on
C = 0; Cursor off (this is a small underline under the next character position, not needed but set to 1 if you want it)
B = 0; Blinking off (will flash the block of next character as if waiting for you to type, not needed for displaying messages)

4. Entry mode set:
I/D = 1; Increment by 1 (How many blocks the cursor moves when writing a new letter, keep at 1 for your own sanity.)
S = 0; No shift         (DO NOT CHANGE)


The 8 pins on the controller represent a byte which we're using to communicate. Obviously 1 means ON, 0 means OFF.
Byte arrangement: D7 D6 D5 D4 BL EN RW RS --> when we bit shift left the data bits are the first to fall off.
When I refer to any pages/tables im talking about the microcontroller documentation.
*/

// ---------------------------------------------------------------------------------------------------

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


// The following bytes are commands based on their Table 6 counterparts, binary passed to the func represents the structure given in the table. 
// for the 2nd line command: see table 13, step 8 (set DDRAM address) - move cursor to second line

// STARTUP COMMANDS PRESETS:
const uint8_t S_FUNCTION_SET{ 0b00101000 };
const uint8_t S_DISPLAY_SET{ 0b00001100 };
const uint8_t S_CHAR_ENTRY_SET{ 0b00000110 };

// OTHER:
const uint8_t CLEAR_DISPLAY{ 0b00000001 };
const uint8_t GOTO_SECOND_LINE{ 0b11000000 };

// SHUTDOWN:
const uint8_t K_INTERNAL_DISPLAY{ 0b00001000 };
const uint8_t K_BACKLIGHT{ 0b00000000 };

class LCD1602{
    private:
        // Member variables
        int i2c_adapter;
        int slave_address;


        // ---- FUNCTIONS ----

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

        // We use edit_pins to send a raw byte with instructions for the LCD to swap to 4-bit, we use this from then on (uses bitshifting).
        void send_byte(int i2c_adapter, uint8_t value, uint8_t rs_mode){
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

            uint8_t high_instruction = (value & 0b11110000) | PIN3_BL | rs_mode;
            edit_pins(i2c_adapter, high_instruction);

            uint8_t low_instruction = ((value << 4) & 0b11110000) | PIN3_BL | rs_mode;
            edit_pins(i2c_adapter, low_instruction);

        }

        void shutdown_lcd(const int i2c_adapter){
            send_byte(i2c_adapter, CLEAR_DISPLAY, 0);
            send_byte(i2c_adapter, K_INTERNAL_DISPLAY, 0);
            write(i2c_adapter, &K_BACKLIGHT, 1);
            close(i2c_adapter);
        }

        void wake_lcd(const int i2c_adapter, const uint8_t DATAPIN4, const uint8_t DATAPIN5){

            // Wake up the LCD and set it to recieve 4-bit instructions
            const uint8_t wake_up = DATAPIN4 | DATAPIN5;
            const uint8_t set_4_bit = DATAPIN5;
            edit_pins(i2c_adapter, wake_up);
            usleep(500);
            edit_pins(i2c_adapter, wake_up);
            usleep(500);
            edit_pins(i2c_adapter, set_4_bit);

            // STARTUP ORDER: check page 23 for the values associated with each byte command shown here
            // 1.) FUNCTION SET
            // 2.) DISPLAY ON/OFF CONTROL
            // 3.) ENTRY MODE SET
            // 4.) CLEAR DISPLAY
            // --- READY FOR USE ---
            send_byte(i2c_adapter, S_FUNCTION_SET, 0);
            send_byte(i2c_adapter, S_DISPLAY_SET, 0);
            send_byte(i2c_adapter, S_CHAR_ENTRY_SET, 0);
            send_byte(i2c_adapter, CLEAR_DISPLAY, 0);

        }

        void clear_display(const int i2c_adapter){
            send_byte(i2c_adapter, CLEAR_DISPLAY, 0);
        }

        // -------------------

    public:

    void wake_up(){
        wake_lcd(i2c_adapter, PIN4_D4, PIN5_D5);
    }

    void shutdown(){
        shutdown_lcd(i2c_adapter);
    }

    // Pass the string by reference to avoid copying a compound type (cardinal sin)
    void send_string(const std::string& message = "", const std::string& message2 = ""){

        int char_count = 0;
        int lines_in_use = 1;

        for(char character : message){
            if(char_count == 16){
                send_byte(i2c_adapter, GOTO_SECOND_LINE, 0);
                lines_in_use++;
            }

            if(char_count == 31){
                std::cout << "You've reached the LCD's character limit. Any characters after the 32nd character will not be displayed." << std::endl;
            }

            send_byte(i2c_adapter, character, 1);
            char_count++;
        }
        
        // Only send the second message if the first does not exceed 16 chars
        if(message2 != ""){
            if(lines_in_use > 1){
                std::cout << "Cannot accept the message: " << message2 << ". The second line is already in use by message 1.\nPlease keep your first message under 17 characters." << std::endl;
            } else {
                send_byte(i2c_adapter, GOTO_SECOND_LINE, 0);

                for(char character : message2){
                    if(char_count == 31){
                        std::cout << "You've reached the LCD's character limit. Any characters after the 32nd character will not be displayed." << std::endl;
                    }

                    send_byte(i2c_adapter, character, 1);
                    char_count++;
                }
            }
        }
    }

    /*
    Sets the cursor to your chosen (x, y) coordinate on the screen.
    y: y=line-1, 1=line-2
    */
    void cursor_pos(uint8_t x, uint8_t y) {
        uint8_t address;
        
        if (y == 0) {
            address = 0b00000000 + x;
        } else {
            address = 0b01000000 + x;
        }

        send_byte(i2c_adapter, 0b10000000 | address, 0); 
    }

    /*
    Overwrites the character to your chosen (x, y) coordinate with an empty space.
    y: y=line-1, 1=line-2
    */
    void delete_char(uint8_t x, uint8_t y) {
        cursor_pos(x,y);
        send_byte(i2c_adapter, ' ', 1);

        // Send the cursor back to the origin for future use
        cursor_pos(0,0);
    }

    // Constructor (runs on initialization)
    LCD1602(int adapter_val, uint8_t address_val ){

        // Assign member vars values given by the user (me)
        i2c_adapter = adapter_val;
        slave_address = address_val;

    }

    // Destructor (runs on object deletion)
    ~LCD1602(){
        shutdown();
    }
};

#endif