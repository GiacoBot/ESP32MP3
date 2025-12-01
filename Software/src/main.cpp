#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "MusicPlayer.h"
#include "PlaylistManager.h" 
#include "BluetoothManager.h"
#include "SerialController.h"
#include "AudioProcessor.h"

// --- Configuration ---
const char* TARGET_DEVICE_NAME = "Lenovo LP40";
const int SD_CS_PIN = 22;
const int SPI_MOSI = 33;
const int SPI_MISO = 26;
const int SPI_SCK = 25;
const char* MUSIC_ROOT = "/";
const int BTN_UP = 13;
const int BTN_DOWN = 17;
const int BTN_LEFT = 4;
const int BTN_RIGHT = 15;
const int BTN_ENTER = 2;
const int OLED_DC = 27;
const int OLED_RESET = 14;
const int OLED_CS = 12;
const int SCREEN_WIDTH = 128; // OLED display width, in pixels
const int SCREEN_HEIGHT = 64;

// --- Global Objects ---
MusicPlayer music_player;
PlaylistManager playlist_manager(MUSIC_ROOT);
BluetoothManager bluetooth_manager(TARGET_DEVICE_NAME);
SerialController serial_controller;
AudioProcessor audio_processor;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// --- Global Variable ---
bool detected_screen = false;

// --- Interrupt Flags ---
volatile bool btn_up_pressed = false;
volatile bool btn_down_pressed = false;
volatile bool btn_left_pressed = false;
volatile bool btn_right_pressed = false;
volatile bool btn_enter_pressed = false;

// --- ISRs (Interrupt Service Routines) ---
// These functions are called by hardware interrupts.
// They should be as short and fast as possible.
// IRAM_ATTR places the function in internal RAM for faster execution.
void IRAM_ATTR isr_btn_up() {
    btn_up_pressed = true;
}

void IRAM_ATTR isr_btn_down() {
    btn_down_pressed = true;
}

void IRAM_ATTR isr_btn_left() {
    btn_left_pressed = true;
}

void IRAM_ATTR isr_btn_right() {
    btn_right_pressed = true;
}

void IRAM_ATTR isr_btn_enter() {
    btn_enter_pressed = true;
}

void handleButtonPresses(); // Forward declaration

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    
    Serial.println("ESP32 Bluetooth MP3 Player Starting...");
    
    // Initialize SD card
    Serial.println("Initializing SD card...");
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SPI.setDataMode(SPI_MODE0);
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD card initialization failed!");
        return;
    }
    Serial.println("SD card initialized successfully");

    if(!display.begin(SSD1306_SWITCHCAPVCC)) {
        Serial.println("SSD1306 initialization failed!");
    }else{
        Serial.println("SSD1306 initialized successfully");
        detected_screen = true;
        display.clearDisplay();
        display.setRotation(2); // rotate the screen 180 degree
        display.drawRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
        display.display();
    }
    
    // Build playlist
    if (!playlist_manager.scanForMP3Files()) {
        Serial.println("Failed to scan for MP3 files");
    }
    
    if (playlist_manager.getTrackCount() == 0) {
        Serial.println("No MP3 files found on SD card!");
    }
    
    // Setup controllers
    serial_controller.setMusicPlayer(&music_player);
    serial_controller.setPlaylistManager(&playlist_manager);
    serial_controller.setBluetoothManager(&bluetooth_manager);
    serial_controller.initialize();
    
    // Setup bluetooth
    bluetooth_manager.setMusicPlayer(&music_player);
    if (!bluetooth_manager.initialize("ESP32_MP3_Player")) {
        Serial.println("Failed to initialize Bluetooth");
        return;
    }
    
    // Setup button pins
    Serial.println("Setting up button interrupts...");
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_ENTER, INPUT_PULLUP);

    // Attach interrupts
    attachInterrupt(digitalPinToInterrupt(BTN_UP), isr_btn_up, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_LEFT), isr_btn_left, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_RIGHT), isr_btn_right, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_ENTER), isr_btn_enter, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_DOWN), isr_btn_down, FALLING);
    
    Serial.println("System ready! Type 'c' to connect or 'h' for help.");
}

void loop() {
    static String current_track;

    serial_controller.handleInput();
    handleButtonPresses();
    if(detected_screen){
        current_track = music_player.getCurrentTrackName();
        if (current_track != "None"){
            display.clearDisplay();
            display.setTextSize(2);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0,0);
            display.println(current_track);
            display.display();
        }
    }
    delay(100);
}

// --- Button Handling Logic ---
void handleButtonPresses() {
    static unsigned long last_press_time = 0;
    const unsigned long debounce_delay = 300; // 300ms debounce delay

    // Use a local copy of flags to avoid race conditions with ISRs
    bool up = btn_up_pressed;
    bool down = btn_down_pressed;
    bool left = btn_left_pressed;
    bool right = btn_right_pressed;
    bool enter = btn_enter_pressed;
    
    // If any button is pressed, check debounce timer
    if (up || down || left || right || enter) {
        unsigned long current_time = millis();
        if (current_time - last_press_time > debounce_delay) {
            last_press_time = current_time;
            
            // Reset flags inside the debounced check
            btn_up_pressed = false;
            btn_down_pressed = false;
            btn_left_pressed = false;
            btn_right_pressed = false;
            btn_enter_pressed = false;

            // Handle one button press at a time
            if (up) {
                Serial.println("Button UP: Connecting...");
                bluetooth_manager.connect();
            } else if (down) {
                Serial.println("Button DOWN: Disconnecting...");
                bluetooth_manager.disconnect();
            } else if (left) {
                Serial.println("Button LEFT: Previous track...");
                music_player.executeCommand(PlayerCommand::PREV_TRACK);
            } else if (right) {
                Serial.println("Button RIGHT: Next track...");
                music_player.executeCommand(PlayerCommand::NEXT_TRACK);
            } else if (enter) {
                Serial.println("Button ENTER: Play/Pause...");
                if (music_player.getState() == PlayerState::PLAYING) {
                    music_player.executeCommand(PlayerCommand::PAUSE);
                } else {
                    music_player.executeCommand(PlayerCommand::PLAY);
                }
            }
        } else {
            // It's a bounce, clear the flags
            btn_up_pressed = false;
            btn_down_pressed = false;
            btn_left_pressed = false;
            btn_right_pressed = false;
            btn_enter_pressed = false;
        }
    }
}
