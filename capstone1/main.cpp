#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <cstdint>

// switchboard
const uint8_t LCD_ADDR = 0x27;
const uint8_t PIN0_RS  = 0x01; // P0
const uint8_t PIN2_EN  = 0x04; // P2 enable (flash key)
const uint8_t PIN3_BL  = 0x08; // P3 Backlight

void pulse_enable(int file, uint8_t data) {

    uint8_t up = data | PIN2_EN;
    write(file, &up, 1);
    usleep(500); 

    uint8_t down = data; 
    write(file, &down, 1);
    usleep(500);

}

int main() {
    int file = open("/dev/i2c-1", O_RDWR);
    ioctl(file, I2C_SLAVE, LCD_ADDR);

    // Didnt include PIN3_BL via:  0x10 | PIN3_BL, The resulting instruction byte does not include the bit that says turn on the backlight.
    uint8_t instruction = 0x10;
    pulse_enable(file, instruction);

    std::cout << "Sent 'Clear' command with Backlight OFF." << std::endl;

    close(file);
    return 0;
}