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
        int i2c_adapter;
        int slave_address;

        // ---- FUNCTIONS ----
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

    public:

    void wake_up(){
        wake_lcd(i2c_adapter, PIN4_D4, PIN5_D5);
    }

    void shutdown(){
        shutdown_lcd(i2c_adapter);
    }

    /*
    Send a message to the LCD screen. Args: Message1, Message2

    Notes:
    If you want to only write on the bottom line you can just write ("") as arg1 and then write the second message.
    Writes to the cursor's position square. You set where you want to start writing yourself manually using cursor_pos().
    */
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
    x: 0-16 = square position (from left to right) y: y=line-1, 1=line-2
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
    x: 0-16 = square position (from left to right) y: y=line-1, 1=line-2
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