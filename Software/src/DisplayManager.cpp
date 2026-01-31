#include "DisplayManager.h"
#include "settings.h"

DisplayManager::DisplayManager() :
    u8g2(U8G2_CONSTRUCTOR_ARGS),
    music_player(nullptr),
    bluetooth_manager(nullptr),
    playlist_manager(nullptr),
    audio_processor(nullptr),
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
    playlist_cache_start_index(-1),
    last_displayed_track(""),
    last_bt_status(false),
    last_player_state(PlayerState::STOPPED),
    last_np_title("")
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

void DisplayManager::setAudioProcessor(AudioProcessor* processor) {
    audio_processor = processor;
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

// --- Cache Methods ---
void DisplayManager::updatePlaylistCache(int start_index) {
    if (!playlist_manager) return;

    // Usa batch read: una sola apertura file per tutti i nomi
    playlist_manager->getTrackNames(start_index, PLAYLIST_VISIBLE_ITEMS, playlist_cached_names);
    playlist_cache_start_index = start_index;
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
        case AppScreen::SCREEN_VOLUME_CONTROL:
            drawVolumeScreen();
            break;
    }

    u8g2.sendBuffer();
}

// --- Drawing Methods ---

void DisplayManager::drawBluetoothMenu() {
    if (!bluetooth_manager) return;

    u8g2.drawStr(0, 12, "Bluetooth Devices");

    // --- Mode 1: A device is being connected to ---
    if (bluetooth_manager->isConnecting()) {
        String line1 = "Connecting to:";
        String line2 = bluetooth_manager->getConnectingDeviceName();

        // Center and draw text
        u8g2_uint_t w1 = u8g2.getStrWidth(line1.c_str());
        u8g2.drawStr((SCREEN_WIDTH - w1) / 2, 30, line1.c_str());
        
        u8g2_uint_t w2 = u8g2.getStrWidth(line2.c_str());
        u8g2.drawStr((SCREEN_WIDTH - w2) / 2, 42, line2.c_str());

        // Draw the Cancel button
        u8g2.drawButtonUTF8(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 6, U8G2_BTN_INV | U8G2_BTN_HCENTER | U8G2_BTN_BW1, 0, 5, 2, "Cancel");
    }
    // --- Mode 2: A device is currently connected ---
    else if (bluetooth_manager->isConnected()) {
        String line1 = "Connected to:";
        String line2 = bluetooth_manager->getConnectedDeviceName();
        
        // Center and draw text
        u8g2_uint_t w1 = u8g2.getStrWidth(line1.c_str());
        u8g2.drawStr((SCREEN_WIDTH - w1) / 2, 30, line1.c_str());
        
        u8g2_uint_t w2 = u8g2.getStrWidth(line2.c_str());
        u8g2.drawStr((SCREEN_WIDTH - w2) / 2, 42, line2.c_str());

        // Draw the disconnect button using the u8g2 button function
        u8g2.drawButtonUTF8(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 6, U8G2_BTN_INV | U8G2_BTN_HCENTER | U8G2_BTN_BW1, 0, 5, 2, "Disconnect");
    } 
    // --- Mode 3: Not connected, show device list ---
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

    const int list_size = playlist_manager->getTrackCount();
    if (list_size == 0) {
        u8g2.drawStr(0, 32, "No tracks on SD card.");
    } else {
        const int max_items_on_screen = 4;

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

        // Update cache only when scroll position changes
        if (playlist_cache_start_index != playlist_menu_scroll_offset) {
            updatePlaylistCache(playlist_menu_scroll_offset);
        }

        int y = 29; // Starting Y for the list
        const int line_height = 11;

        for (int i = 0; i < max_items_on_screen; ++i) {
            int item_index = playlist_menu_scroll_offset + i;
            if (item_index >= list_size) {
                break;
            }

            // Use cached track name instead of reading from SD card every frame
            int cache_index = item_index - playlist_menu_scroll_offset;
            String& track_name = playlist_cached_names[cache_index];

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

void DisplayManager::drawPlayStateIcon(int x, int y, PlayerState state) {
    // Use U8g2 built-in icons from open_iconic_play font
    u8g2.setFont(u8g2_font_open_iconic_play_1x_t);
    switch (state) {
        case PlayerState::PLAYING:
            u8g2.drawGlyph(x, y, 0x45);  // Play icon
            break;
        case PlayerState::PAUSED:
            u8g2.drawGlyph(x, y, 0x44);  // Pause icon
            break;
        case PlayerState::STOPPED:
            u8g2.drawGlyph(x, y, 0x4E);  // Stop icon
            break;
    }
    // Restore main font
    u8g2.setFont(u8g2_font_boutique_bitmap_9x9_t_all);
}

void DisplayManager::drawBtIcon(int x, int y, bool connected) {
    if (connected) {
        u8g2.drawXBMP(x, y, 8, 8, bt_icon_bits);
    } else {
        u8g2.drawXBMP(x, y, 8, 8, bt_disconnected_bits);
    }
}

void DisplayManager::drawProgressBar(int x, int y, int width, int height, float progress,
                                      uint32_t position_sec, uint32_t duration_sec) {
    // Draw progress bar outline
    u8g2.drawFrame(x, y, width, height);

    // Draw filled portion
    int fill_width = (int)((width - 2) * progress);
    if (fill_width > 0) {
        u8g2.drawBox(x + 1, y + 1, fill_width, height - 2);
    }

    // Format time strings
    char time_str[16];
    int pos_min = position_sec / 60;
    int pos_sec = position_sec % 60;
    int dur_min = duration_sec / 60;
    int dur_sec = duration_sec % 60;
    snprintf(time_str, sizeof(time_str), "%d:%02d/%d:%02d", pos_min, pos_sec, dur_min, dur_sec);

    // Draw time to the right of progress bar
    u8g2_uint_t time_width = u8g2.getStrWidth(time_str);
    u8g2.drawStr(SCREEN_WIDTH - time_width, y + height - 1, time_str);
}

void DisplayManager::drawScrollingText(int x, int y, int available_width, const char* text,
                                        TextScroller& scroller) {
    u8g2_uint_t text_width = u8g2.getStrWidth(text);
    int scroll_offset = scroller.update(text_width, available_width);

    // Set clipping window to prevent text overflow
    u8g2.setClipWindow(x, y - 11, x + available_width, y + 2);
    u8g2.drawStr(x - scroll_offset, y, text);
    u8g2.setMaxClipWindow();
}

void DisplayManager::drawNowPlayingScreen() {
    if (!music_player || !bluetooth_manager) {
        u8g2.drawStr(0, 32, "Managers not set!");
        return;
    }

    PlayerState player_state = music_player->getState();
    const TrackMetadata& metadata = music_player->getCurrentMetadata();

    // Check if track changed to reset scrollers
    const char* current_title = metadata.title;
    if (last_np_title != current_title) {
        last_np_title = current_title;
        np_title_scroller.reset();
        np_artist_scroller.reset();
        np_album_scroller.reset();
    }

    // Update state cache
    last_player_state = player_state;

    // Helper variable
    int icon_x = 0;
    const int np_icon_size = 8;
    const int np_icon_gap = 2;
    const int np_text_x = 11;
    const int text_available_width = SCREEN_WIDTH - np_text_x;
    const int np_header_y = 12;
    const int np_title_y = 26;
    const int np_artist_y = 38;
    const int np_album_y = 50;
    const int np_progress_y = 57;
    const int np_progress_height = 6;

    // --- Row 1: Header "Now Playing" ---
    u8g2.drawStr(0, np_header_y, "Now Playing");

    // --- Row 2: Play state icon + Title ---
    
    // Draw play state icon (8x8)
    drawPlayStateIcon(0, np_title_y + np_icon_gap, player_state);
    icon_x += np_icon_size + np_icon_gap + 1;

    // Draw title with scrolling
    const char* title = (metadata.title[0] != '\0') ? metadata.title : "No track";
    drawScrollingText(np_text_x, np_title_y, text_available_width, title, np_title_scroller);

    // --- Row 3: Artist ---
    if (metadata.artist[0] != '\0') {
        // Draw user icon
        u8g2.drawXBMP(0, np_artist_y - np_icon_size + np_icon_gap, np_icon_size, np_icon_size, user_icon_bits);
        // Draw artist with scrolling
        drawScrollingText(np_text_x, np_artist_y, text_available_width,
                         metadata.artist, np_artist_scroller);
    }

    // --- Row 4: Album ---
    if (metadata.album[0] != '\0') {
        // Draw album icon
        u8g2.drawXBMP(0, np_album_y - np_icon_size + np_icon_gap, np_icon_size, np_icon_size, album_icon_bits);
        // Draw album with scrolling
        drawScrollingText(np_text_x, np_album_y, text_available_width,
                         metadata.album, np_album_scroller);
    }

    // --- Row 5: Progress bar + time (Y=57) ---
    uint32_t position_sec = bluetooth_manager->getPlaybackSeconds();
    uint32_t duration_sec = 0;
    float progress = 0.0f;

    if (audio_processor != nullptr) {
        duration_sec = audio_processor->getDurationSeconds();
        if (duration_sec > 0) {
            progress = (float)position_sec / (float)duration_sec;
            if (progress > 1.0f) progress = 1.0f;
        }
    }

    // Calculate progress bar width (leave space for time display)
    // Time format: "M:SS/M:SS" ~= 9 chars * 6 pixels = 54 pixels + padding
    int progress_bar_width = SCREEN_WIDTH - 58;
    drawProgressBar(0, np_progress_y, progress_bar_width, np_progress_height,
                    progress, position_sec, duration_sec);
}

void DisplayManager::drawVolumeScreen() {
    if (!bluetooth_manager) {
        u8g2.drawStr(0, 32, "BT Manager not set!");
        return;
    }

    // Title
    u8g2.drawStr(0, 12, "Volume");

    // Get volume and convert to percentage (0-127 -> 0-100%)
    uint8_t volume = bluetooth_manager->getVolume();
    int percentage = (volume * 100) / 127;

    // Draw percentage centered
    char percent_str[8];
    snprintf(percent_str, sizeof(percent_str), "%d%%", percentage);
    u8g2_uint_t text_width = u8g2.getStrWidth(percent_str);
    u8g2.drawStr((SCREEN_WIDTH - text_width) / 2, 50, percent_str);

    // Draw progress bar
    const int bar_x = 10;
    const int bar_y = 28;
    const int bar_width = SCREEN_WIDTH - 20;
    const int bar_height = 12;

    // Draw bar outline
    u8g2.drawFrame(bar_x, bar_y, bar_width, bar_height);

    // Draw filled portion
    int fill_width = (bar_width - 2) * percentage / 100;
    if (fill_width > 0) {
        u8g2.drawBox(bar_x + 1, bar_y + 1, fill_width, bar_height - 2);
    }
}
