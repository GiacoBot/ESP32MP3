#include "DisplayManager.h"
#include <SPI.h>

// --- Pin Definitions ---
const int OLED_DC = 27;
const int OLED_RESET = 14;
const int OLED_CS = 12;
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;

DisplayManager::DisplayManager() : 
    display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS),
    music_player(nullptr),
    is_initialized(false),
    last_displayed_track("")
    {}

void DisplayManager::setMusicPlayer(MusicPlayer* player) {
    music_player = player;
}

bool DisplayManager::initialize() {
    if(!display.begin(SSD1306_SWITCHCAPVCC)) {
        Serial.println("SSD1306 initialization failed!");
        is_initialized = false;
    } else {
        Serial.println("SSD1306 initialized successfully");
        is_initialized = true;
        display.clearDisplay();
        display.setRotation(2); // rotate the screen 180 degree
        display.drawRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
        display.display();
    }
    return is_initialized;
}

void DisplayManager::update() {
    if (!is_initialized || !music_player) {
        return;
    }

    String current_track = music_player->getCurrentTrackName();
    
    // Only update display if track name has changed and it is a valid track
    if (current_track != "None" && current_track != last_displayed_track) {
        last_displayed_track = current_track;
        
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(20,0);
        display.println(current_track); // Using println to handle wrapping
        display.display();
    } else if (current_track == "None" && last_displayed_track != "None") {
        // Music stopped, clear the screen
        last_displayed_track = "None";
        display.clearDisplay();
        display.display();
    }
}
