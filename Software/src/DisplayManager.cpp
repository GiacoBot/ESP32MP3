#include "DisplayManager.h"
#include "settings.h"

DisplayManager::DisplayManager() : 
    u8g2(U8G2_CONSTRUCTOR_ARGS),
    music_player(nullptr),
    bluetooth_manager(nullptr),
    playlist_manager(nullptr),
    is_initialized(false),
    bt_menu_selected_index(0),
    bt_menu_scroll_offset(0),
    playlist_menu_selected_index(0),
    playlist_menu_scroll_offset(0),
    last_displayed_track(""),
    last_bt_status(false),
    last_player_state(PlayerState::STOPPED)
    {}

bool DisplayManager::initialize() {
    is_initialized = u8g2.begin();
    if(!is_initialized) {
        Serial.println("SSD1306 initialization failed!");
    } else {
        Serial.println("SSD1306 initialized successfully");
        u8g2.clearBuffer();
        #if defined(SHOW_SPLASH_SCREEN) && SHOW_SPLASH_SCREEN
            const int16_t x = (SCREEN_WIDTH - LOGO_WIDTH) / 2;
            const int16_t y = (SCREEN_HEIGHT - LOGO_HEIGHT) / 2;
            u8g2.drawXBM(x, y, LOGO_WIDTH, LOGO_HEIGHT, splash_screen_logo);
            u8g2.sendBuffer();
            delay(SPLASH_SCREEN_DURATION);
        #endif
        u8g2.clearBuffer();
        u8g2.sendBuffer();
    }
    return is_initialized;
}

// --- Setters for Managers ---
void DisplayManager::setMusicPlayer(MusicPlayer* player) {
    music_player = player;
}

void DisplayManager::setBluetoothManager(BluetoothManager* btManager) {
    bluetooth_manager = btManager;
}

void DisplayManager::setPlaylistManager(PlaylistManager* plManager) {
    playlist_manager = plManager;
}

// --- Setters for UI State ---
void DisplayManager::setBluetoothMenuState(int selected_index, int scroll_offset) {
    bt_menu_selected_index = selected_index;
    bt_menu_scroll_offset = scroll_offset;
}

void DisplayManager::setPlaylistMenuState(int selected_index, int scroll_offset) {
    playlist_menu_selected_index = selected_index;
    playlist_menu_scroll_offset = scroll_offset;
}

// --- Core Update Method ---
void DisplayManager::update(AppScreen current_screen) {
    if (!is_initialized) return;

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_boutique_bitmap_9x9_t_all);

    switch (current_screen) {
        case AppScreen::SCREEN_BLUETOOTH_SELECTION:
            drawBluetoothMenu();
            break;
        case AppScreen::SCREEN_TRACK_SELECTION:
            drawPlaylistMenu();
            break;
        case AppScreen::SCREEN_NOW_PLAYING:
            drawNowPlayingScreen();
            break;
    }

    u8g2.sendBuffer();
}

// --- Drawing Methods ---

void DisplayManager::drawBluetoothMenu() {
    if (!bluetooth_manager) return;

    u8g2.drawStr(0, 12, "Bluetooth Devices");
    
    const auto& devices = bluetooth_manager->getDiscoveredDevices();
    if (bluetooth_manager->isDiscovering() && devices.empty()) {
        u8g2.drawStr(0, 32, "Scanning...");
    } else if (devices.empty()) {
        u8g2.drawStr(0, 32, "No devices found.");
    } else {
        int y = 26; // Starting Y for the list
        const int line_height = 11;
        
        for (size_t i = 0; i < devices.size() && i < 4; ++i) {
            String device_name = devices[i].name;
            
            if (i == bt_menu_selected_index) {
                // Highlight the selected item
                u8g2.drawBox(0, y - line_height + 2, SCREEN_WIDTH, line_height);
                u8g2.setDrawColor(0); // Black text on white box
                u8g2.drawStr(2, y, device_name.c_str());
                u8g2.setDrawColor(1); // Reset to white text
            } else {
                u8g2.drawStr(2, y, device_name.c_str());
            }
            y += line_height;
        }
    }
}

void DisplayManager::drawPlaylistMenu() {
    if (!playlist_manager) return;

    u8g2.drawStr(0, 12, "Playlist");

    const auto& tracks = playlist_manager->getPlaylist();
    if (tracks.empty()) {
        u8g2.drawStr(0, 32, "No tracks on SD card.");
    } else {
        int y = 26; // Starting Y for the list
        const int line_height = 11;

        for (size_t i = 0; i < tracks.size() && i < 4; ++i) {
            String track_name = playlist_manager->getTrackName(i);
            
            if (i == playlist_menu_selected_index) {
                u8g2.drawBox(0, y - line_height + 2, SCREEN_WIDTH, line_height);
                u8g2.setDrawColor(0);
                u8g2.drawStr(2, y, track_name.c_str());
                u8g2.setDrawColor(1);
            } else {
                u8g2.drawStr(2, y, track_name.c_str());
            }
            y += line_height;
        }
    }
}

void DisplayManager::drawNowPlayingScreen() {
    if (!music_player || !bluetooth_manager) {
        u8g2.drawStr(0, 32, "Managers not set!");
        return;
    }

    String current_track = music_player->getCurrentTrackName();
    PlayerState player_state = music_player->getState();
    bool bt_connected = bluetooth_manager->isConnected();

    // If nothing has changed, we don't need to do anything.
    // This check was removed and is now re-added to prevent flicker,
    // but the final sendBuffer() call in update() will handle the drawing.
    if (current_track == last_displayed_track && player_state == last_player_state && bt_connected == last_bt_status) {
        // We still need to draw the last known state since the buffer was cleared.
    } else {
        // Update last known states
        last_displayed_track = current_track;
        last_player_state = player_state;
        last_bt_status = bt_connected;
    }

    // 1. Bluetooth Status (Top-right)
    String bt_status_text = "BT: ";
    bt_status_text += (last_bt_status) ? "Connected" : "Not Connected";
    u8g2_uint_t bt_text_width = u8g2.getStrWidth(bt_status_text.c_str());
    u8g2.setCursor(SCREEN_WIDTH - bt_text_width, 12);
    u8g2.print(bt_status_text);
    
    // 2. Track Title
    u8g2.setCursor(0, 28);
    u8g2.print((last_displayed_track != "None") ? last_displayed_track : "No track playing");

    // 3. Player Status (Bottom-left)
    String player_status_text = "Status: ";
    switch(last_player_state) {
        case PlayerState::PLAYING: player_status_text += "Playing"; break;
        case PlayerState::PAUSED:  player_status_text += "Paused"; break;
        case PlayerState::STOPPED: player_status_text += "Stopped"; break;
    }
    u8g2.setCursor(0, 60);
    u8g2.print(player_status_text);
}
