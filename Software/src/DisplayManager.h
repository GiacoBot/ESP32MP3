#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <U8g2lib.h>
#include "settings.h"
#include "AppState.h"
#include "MusicPlayer.h"
#include "BluetoothManager.h"
#include "PlaylistManager.h"

class DisplayManager {
public:
    DisplayManager();
    bool initialize();

    // --- Core Update Method ---
    // This is now the main entry point for drawing, dispatched by the main loop.
    void update(AppScreen current_screen);

    // --- Setters for Managers ---
    void setMusicPlayer(MusicPlayer* player);
    void setBluetoothManager(BluetoothManager* btManager);
    void setPlaylistManager(PlaylistManager* playlistManager);

    // --- Setters for UI State ---
    // These will be called from the main loop to update the UI state
    void setBluetoothMenuState(int selected_index, int scroll_offset);
    void setPlaylistMenuState(int selected_index, int scroll_offset);

private:
    // --- Managers ---
    MusicPlayer* music_player;
    BluetoothManager* bluetooth_manager;
    PlaylistManager* playlist_manager;

    // --- Display & State ---
    U8G2_DISPLAY_TYPE u8g2;
    bool is_initialized;

    // --- UI State Variables ---
    int bt_menu_selected_index;
    int bt_menu_scroll_offset;
    int playlist_menu_selected_index;
    int playlist_menu_scroll_offset;

    // --- Drawing Methods ---
    void drawBluetoothMenu();
    void drawPlaylistMenu();
    void drawNowPlayingScreen();

    // --- "Now Playing" screen state tracking ---
    String last_displayed_track;
    bool last_bt_status;
    PlayerState last_player_state;
};

#endif // DISPLAY_MANAGER_H
