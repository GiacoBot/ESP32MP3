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
    prev_bt_menu_selected_index(0),
    prev_playlist_menu_selected_index(0),
    bt_last_selection_time(0),
    bt_last_scroll_time(0),
    bt_text_scroll_offset_pixels(0),
    playlist_last_selection_time(0),
    playlist_last_scroll_time(0),
    playlist_text_scroll_offset_pixels(0),
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
    prev_bt_menu_selected_index = bt_menu_selected_index;

    // If the selection has truly changed, reset the horizontal scroll state
    if (bt_menu_selected_index != selected_index) {
        bt_text_scroll_offset_pixels = 0;
        bt_last_selection_time = millis();
    }
    
    bt_menu_selected_index = selected_index;
    // The incoming scroll_offset is ignored, as we will calculate it internally.
}

void DisplayManager::setPlaylistMenuState(int selected_index, int scroll_offset) {
    prev_playlist_menu_selected_index = playlist_menu_selected_index;

    // If the selection has truly changed, reset the horizontal scroll state
    if (playlist_menu_selected_index != selected_index) {
        playlist_text_scroll_offset_pixels = 0;
        playlist_last_selection_time = millis();
    }

    playlist_menu_selected_index = selected_index;
    // The incoming scroll_offset is ignored, as we will calculate it internally.
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

    // --- Mode 1: A device is currently connected ---
    if (bluetooth_manager->isConnected()) {
        String line1 = "Connected to:";
        String line2 = bluetooth_manager->getConnectedDeviceName();
        String button = "[ Disconnect ]";

        // Center and draw text
        u8g2_uint_t w1 = u8g2.getStrWidth(line1.c_str());
        u8g2.drawStr((SCREEN_WIDTH - w1) / 2, 30, line1.c_str());
        
        u8g2_uint_t w2 = u8g2.getStrWidth(line2.c_str());
        u8g2.drawStr((SCREEN_WIDTH - w2) / 2, 42, line2.c_str());

        // Draw the disconnect button, highlighted
        const int line_height = 11;
        u8g2_uint_t w3 = u8g2.getStrWidth(button.c_str());
        int button_y = SCREEN_HEIGHT - 4;
        u8g2.drawBox((SCREEN_WIDTH - w3) / 2 - 2, button_y - line_height + 2, w3 + 4, line_height);
        u8g2.setDrawColor(0); // Inverted color for text
        u8g2.drawStr((SCREEN_WIDTH - w3) / 2, button_y, button.c_str());
        u8g2.setDrawColor(1); // Reset color
    } 
    // --- Mode 2: Not connected, show device list ---
    else {
        
        const auto& devices = bluetooth_manager->getDiscoveredDevices();
        if (bluetooth_manager->isDiscovering() && devices.empty()) {
            u8g2.drawStr(0, 32, "Scanning...");
        } else if (devices.empty()) {
            u8g2.drawStr(0, 32, "No devices found.");
        } else {
            const int max_items_on_screen = 4;
            const int list_size = devices.size();

            // Determine available width based on whether the scrollbar will be drawn
            int available_width = (list_size > max_items_on_screen) ? SCREEN_WIDTH - 4 : SCREEN_WIDTH;

            // --- Vertical Scrolling Logic ---
            int direction = bt_menu_selected_index - prev_bt_menu_selected_index;
            if (direction > 0) { // Moving Down
                if (bt_menu_selected_index >= bt_menu_scroll_offset + max_items_on_screen) {
                    bt_menu_scroll_offset = bt_menu_selected_index - max_items_on_screen + 1;
                }
            } else if (direction < 0) { // Moving Up
                if (bt_menu_selected_index < bt_menu_scroll_offset) {
                    bt_menu_scroll_offset = bt_menu_selected_index;
                }
            }
            if (list_size > 0) {
                 bt_menu_scroll_offset = constrain(bt_menu_scroll_offset, 0, max(0, list_size - max_items_on_screen));
            } else {
                bt_menu_scroll_offset = 0;
            }

            int y = 29; // Starting Y for the list
            const int line_height = 11;
            
            for (int i = 0; i < max_items_on_screen; ++i) {
                int item_index = bt_menu_scroll_offset + i;
                if (item_index >= list_size) {
                    break;
                }
                
                String device_name = devices[item_index].name;
                
                if (item_index == bt_menu_selected_index) {
                    // --- Horizontal Scrolling Logic for Selected Item ---
                    u8g2_uint_t text_width = u8g2.getStrWidth(device_name.c_str());
                    int text_area_width = available_width - 2; // -2 for padding at x=2

                    if (text_width > text_area_width) {
                        int max_scroll_offset = text_width - text_area_width;
                        if (bt_text_scroll_offset_pixels > max_scroll_offset) {
                            if (millis() - bt_last_scroll_time > MENU_SCROLL_DELAY) {
                                bt_text_scroll_offset_pixels = 0;
                                bt_last_selection_time = millis();
                            }
                        } 
                        else {
                            if (millis() - bt_last_selection_time > MENU_SCROLL_DELAY) {
                                if (millis() - bt_last_scroll_time > MENU_SCROLL_SPEED) {
                                    bt_last_scroll_time = millis();
                                    bt_text_scroll_offset_pixels++;
                                }
                            }
                        }
                    } else {
                        bt_text_scroll_offset_pixels = 0;
                    }

                    u8g2.drawBox(0, y - line_height + 2, SCREEN_WIDTH, line_height);
                    u8g2.setDrawColor(0);
                    u8g2.drawStr(2 - bt_text_scroll_offset_pixels, y, device_name.c_str());
                    u8g2.setDrawColor(1);
                } else {
                    // Clip non-selected items to prevent drawing over the scrollbar
                    u8g2.setClipWindow(0, y - line_height, available_width, y + 2);
                    u8g2.drawStr(2, y, device_name.c_str());
                    u8g2.setMaxClipWindow(); // Reset clipping
                }
                y += line_height;
            }

            if (list_size > max_items_on_screen) {
                // --- Scrollbar Drawing ---
                const int scrollbar_y_start = 20;
                const int scrollbar_area_height = SCREEN_HEIGHT - scrollbar_y_start;
                int handle_height = (float)scrollbar_area_height * ((float)max_items_on_screen / list_size);
                handle_height = max(2, handle_height);
                
                float scroll_percent = (float)bt_menu_scroll_offset / (list_size - max_items_on_screen);
                int handle_y_offset = (scrollbar_area_height - handle_height) * scroll_percent;
                
                // Draw the scrollbar handle
                u8g2.setDrawColor(0);            
                u8g2.drawBox(SCREEN_WIDTH - 4, scrollbar_y_start, 4, scrollbar_area_height);
                u8g2.setDrawColor(1);
                u8g2.drawLine(SCREEN_WIDTH - 2, scrollbar_y_start, SCREEN_WIDTH - 2, SCREEN_HEIGHT);
                u8g2.drawBox(SCREEN_WIDTH - 3, scrollbar_y_start + handle_y_offset, 3, handle_height);
            }
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
        const int max_items_on_screen = 4;
        const int list_size = tracks.size();

        // Determine available width based on whether the scrollbar will be drawn
        int available_width = (list_size > max_items_on_screen) ? SCREEN_WIDTH - 4 : SCREEN_WIDTH;

        // --- Vertical Scrolling Logic ---
        int direction = playlist_menu_selected_index - prev_playlist_menu_selected_index;
        if (direction > 0) { // Moving Down
            if (playlist_menu_selected_index >= playlist_menu_scroll_offset + max_items_on_screen) {
                playlist_menu_scroll_offset = playlist_menu_selected_index - max_items_on_screen + 1;
            }
        } else if (direction < 0) { // Moving Up
            if (playlist_menu_selected_index < playlist_menu_scroll_offset) {
                playlist_menu_scroll_offset = playlist_menu_selected_index;
            }
        }
        if (list_size > 0) {
            playlist_menu_scroll_offset = constrain(playlist_menu_scroll_offset, 0, max(0, list_size - max_items_on_screen));
        } else {
            playlist_menu_scroll_offset = 0;
        }

        int y = 29; // Starting Y for the list
        const int line_height = 11;

        for (int i = 0; i < max_items_on_screen; ++i) {
            int item_index = playlist_menu_scroll_offset + i;
            if (item_index >= list_size) {
                break;
            }

            String track_name = playlist_manager->getTrackName(item_index);
            
            if (item_index == playlist_menu_selected_index) {
                // --- Horizontal Scrolling Logic for Selected Item ---
                u8g2_uint_t text_width = u8g2.getStrWidth(track_name.c_str());
                int text_area_width = available_width - 2; // -2 for padding at x=2

                if (text_width > text_area_width) {
                    int max_scroll_offset = text_width - text_area_width;
                    if (playlist_text_scroll_offset_pixels > max_scroll_offset) {
                        if (millis() - playlist_last_scroll_time > MENU_SCROLL_DELAY) {
                            playlist_text_scroll_offset_pixels = 0;
                            playlist_last_selection_time = millis();
                        }
                    }
                    else {
                        if (millis() - playlist_last_selection_time > MENU_SCROLL_DELAY) {
                            if (millis() - playlist_last_scroll_time > MENU_SCROLL_SPEED) {
                                playlist_last_scroll_time = millis();
                                playlist_text_scroll_offset_pixels++;
                            }
                        }
                    }
                } else {
                    playlist_text_scroll_offset_pixels = 0;
                }

                u8g2.drawBox(0, y - line_height + 2, SCREEN_WIDTH, line_height);
                u8g2.setDrawColor(0);
                u8g2.drawStr(2 - playlist_text_scroll_offset_pixels, y, track_name.c_str());
                u8g2.setDrawColor(1);
            } else {
                // Clip non-selected items to prevent drawing over the scrollbar
                u8g2.setClipWindow(0, y - line_height, available_width, y + 2);
                u8g2.drawStr(2, y, track_name.c_str());
                u8g2.setMaxClipWindow(); // Reset clipping
            }
            y += line_height;
        }

        if (list_size > max_items_on_screen) {
            // --- Scrollbar Drawing ---
            const int scrollbar_y_start = 20;
            const int scrollbar_area_height = SCREEN_HEIGHT - scrollbar_y_start;
            int handle_height = (float)scrollbar_area_height * ((float)max_items_on_screen / list_size);
            handle_height = max(2, handle_height);
            float scroll_percent = (float)playlist_menu_scroll_offset / (list_size - max_items_on_screen);
            int handle_y_offset = (scrollbar_area_height - handle_height) * scroll_percent;
            
            // Draw the scrollbar handle
            u8g2.setDrawColor(0);            
            u8g2.drawBox(SCREEN_WIDTH - 4, scrollbar_y_start, 4, scrollbar_area_height);
            u8g2.setDrawColor(1);
            u8g2.drawLine(SCREEN_WIDTH - 2, scrollbar_y_start, SCREEN_WIDTH - 2, SCREEN_HEIGHT);
            u8g2.drawBox(SCREEN_WIDTH - 3, scrollbar_y_start + handle_y_offset, 3, handle_height);
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
