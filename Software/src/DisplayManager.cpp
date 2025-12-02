#include "DisplayManager.h"
#include "settings.h"
#include <SPI.h>

DisplayManager::DisplayManager() : 
    display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS),
    music_player(nullptr),
    bluetooth_manager(nullptr),
    is_initialized(false),
    last_displayed_track(""),
    last_bt_status(false),
    last_player_state(PlayerState::STOPPED)
    {}

void DisplayManager::setMusicPlayer(MusicPlayer* player) {
    music_player = player;
}

void DisplayManager::setBluetoothManager(BluetoothManager* btManager) {
    bluetooth_manager = btManager;
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
        display.display();
    }
    return is_initialized;
}

void DisplayManager::update() {
    if (!is_initialized || !music_player || !bluetooth_manager) {
        return;
    }

    String current_track = music_player->getCurrentTrackName();
    PlayerState player_state = music_player->getState();
    bool bt_connected = bluetooth_manager->isConnected();

    if (current_track == last_displayed_track && player_state == last_player_state && bt_connected == last_bt_status) {
        return; // Nothing changed, no need to redraw
    }

    // Something changed, update last known states and redraw everything
    last_displayed_track = current_track;
    last_player_state = player_state;
    last_bt_status = bt_connected;

    display.clearDisplay();
    display.setTextSize(1); // Smaller font size as requested
    display.setTextColor(SSD1306_WHITE);

    // 1. Bluetooth Status (Right-aligned)
    String bt_status_text = "BT: ";
    if (bt_connected) {
        bt_status_text += "Connected";
    } else {
        bt_status_text += "Not Connected";
    }
    
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(bt_status_text, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(SCREEN_WIDTH - w, 0);
    display.println(bt_status_text);
    
    // 2. Track Title
    display.setCursor(0, 20); // Leave some space after the first line
    if (current_track != "None") {
        display.println(current_track);
    } else {
        display.println("No track playing");
    }

    // 3. Player Status
    display.setCursor(0, 56); // Bottom of the screen (64 - 8 for font height)
    String player_status_text = "Status: ";
    switch(player_state) {
        case PlayerState::PLAYING:
            player_status_text += "Playing";
            break;
        case PlayerState::PAUSED:
            player_status_text += "Paused";
            break;
        case PlayerState::STOPPED:
            player_status_text += "Stopped";
            break;
    }
    display.println(player_status_text);

    display.display();
}
