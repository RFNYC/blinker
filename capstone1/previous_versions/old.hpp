#pragma once

#include <cstdint>
#include <unistd.h>

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
*/

// ---------------------------------------------------------------------------------------------------


// The following bytes are commands based on their Table 6 counterparts, binary passed to the func represents the structure given in the table. 
// for the 2nd line command: see table 13, step 8 (set DDRAM address) - move cursor to second line

// STARTUP COMMANDS:
const uint8_t S_FUNCTION_SET{ 0b00101000 };
const uint8_t S_DISPLAY_SET{ 0b00001100 };
const uint8_t S_CHAR_ENTRY_SET{ 0b00000110 };

// OTHER:
const uint8_t CLEAR_DISPLAY{ 0b00000001 };
const uint8_t GOTO_SECOND_LINE{ 0b11000000 };

// SHUTDOWN:
const uint8_t K_INTERNAL_DISPLAY{ 0b00001000 };
const uint8_t K_BACKLIGHT{ 0b00000000 };

double fsleep(double seconds) {
    return usleep(seconds * 1000000);
}