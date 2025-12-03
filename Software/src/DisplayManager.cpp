#include "DisplayManager.h"
#include "settings.h"

// The u8g2 object is initialized using the constructor arguments
// defined in settings.h. This makes the display hardware configuration
// fully modular.
DisplayManager::DisplayManager() : 
    u8g2(U8G2_CONSTRUCTOR_ARGS),
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
    is_initialized = u8g2.begin();
    if(!is_initialized) {
        Serial.println("SSD1306 initialization failed!");
    } else {
        Serial.println("SSD1306 initialized successfully");

        // U8g2 drawing commands work on a buffer.
        // After drawing, we send the buffer to the display.
        u8g2.clearBuffer();

        // Show splash screen if enabled
        #if defined(SHOW_SPLASH_SCREEN) && SHOW_SPLASH_SCREEN
            // Center the bitmap
            const int16_t x = (SCREEN_WIDTH - LOGO_WIDTH) / 2;
            const int16_t y = (SCREEN_HEIGHT - LOGO_HEIGHT) / 2;
            u8g2.drawXBM(x, y, LOGO_WIDTH, LOGO_HEIGHT, splash_screen_logo);
            u8g2.sendBuffer();
            delay(SPLASH_SCREEN_DURATION);
        #endif

        // Clear display after splash screen, ready for main UI
        u8g2.clearBuffer();
        u8g2.sendBuffer();
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

    // Only redraw the screen if something has changed
    if (current_track == last_displayed_track && player_state == last_player_state && bt_connected == last_bt_status) {
        return; 
    }

    // Update last known states
    last_displayed_track = current_track;
    last_player_state = player_state;
    last_bt_status = bt_connected;

    u8g2.clearBuffer();
    
    // Select a font. U8g2 has a wide variety of fonts. 
    // The user has selected u8g2_font_boutique_bitmap_9x9_t_all.
    u8g2.setFont(u8g2_font_boutique_bitmap_9x9_t_all);

    // 1. Bluetooth Status (Top-right)
    String bt_status_text = "BT: ";
    if (bt_connected) {
        bt_status_text += "Connected";
    } else {
        bt_status_text += "Not Connected";
    }
    
    // Calculate position to right-align the text
    u8g2_uint_t bt_text_width = u8g2.getStrWidth(bt_status_text.c_str());
    u8g2.setCursor(SCREEN_WIDTH - bt_text_width, 12); // Adjusted for new font height
    u8g2.print(bt_status_text);
    
    // 2. Track Title
    u8g2.setCursor(0, 28); // Adjusted for new font height
    if (current_track != "None") {
        u8g2.print(current_track);
    } else {
        u8g2.print("No track playing");
    }

    // 3. Player Status (Bottom-left)
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
    u8g2.setCursor(0, 60); // Bottom of the screen
    u8g2.print(player_status_text);

    // Send the buffer to the display to show the changes
    u8g2.sendBuffer();
}
