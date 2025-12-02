#ifndef SETTINGS_H
#define SETTINGS_H

// =================================================================================
// HARDWARE CONFIGURATION
// =================================================================================
#define DEV_BOARD_REV_1_0
// #define CUSTOM_BOARD


// =================================================================================
// GENERAL SETTINGS
// =================================================================================
#define SERIAL_BAUD_RATE 115200


// =================================================================================
// DEV BOARD V1.0 CONFIGURATION
// =================================================================================
#if defined(DEV_BOARD_REV_1_0)

// Target Bluetooth device name
#define TARGET_DEVICE_NAME "Lenovo LP40"

// Music root directory on SD card
#define MUSIC_ROOT "/"

// --- SPI BUS ---
#define SPI_MISO 26
#define SPI_MOSI 33
#define SPI_SCK  25

// --- SD Card ---
#define SD_CS_PIN 22

// --- OLED Display ---
#define OLED_DC    27
#define OLED_RESET 14
#define OLED_CS    12
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// --- Input Buttons ---
#define BTN_UP    13
#define BTN_DOWN  17
#define BTN_LEFT  4
#define BTN_RIGHT 15
#define BTN_ENTER 2
#define DEBOUNCE_DELAY 300 // in milliseconds

#endif // DEV_BOARD_REV_1_0

// =================================================================================
// CUSTOM BOARD CONFIGURATION (Example)
// =================================================================================
#if defined(CUSTOM_BOARD)

// Custom pin and settings definition

#endif // CUSTOM_BOARD


#endif // SETTINGS_H
