#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <U8g2lib.h>
#include "settings.h"
#include "AppState.h"
#include "MusicPlayer.h"
#include "BluetoothManager.h"
#include "PlaylistManager.h"
#include "TextScroller.h"
#include "AudioProcessor.h"

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
    void setAudioProcessor(AudioProcessor* audioProcessor);

    // --- Setters for UI State ---
    // These will be called from the main loop to update the UI state
    void setBluetoothMenuState(int selected_index, int scroll_offset);
    void setPlaylistMenuState(int selected_index, int scroll_offset);

private:
    // --- Managers ---
    MusicPlayer* music_player;
    BluetoothManager* bluetooth_manager;
    PlaylistManager* playlist_manager;
    AudioProcessor* audio_processor;

    // --- Display & State ---
    U8G2_DISPLAY_TYPE u8g2;
    bool is_initialized;

    // --- UI State Variables ---
    int bt_menu_selected_index;
    int bt_menu_scroll_offset;
    int playlist_menu_selected_index;
    int playlist_menu_scroll_offset;

    // Internal state for tracking scroll direction
    int prev_bt_menu_selected_index;
    int prev_playlist_menu_selected_index;

    // Internal state for horizontal text scrolling
    unsigned long bt_last_selection_time;
    unsigned long bt_last_scroll_time;
    int bt_text_scroll_offset_pixels;
    unsigned long playlist_last_selection_time;
    unsigned long playlist_last_scroll_time;
    int playlist_text_scroll_offset_pixels;

    // Track name cache for playlist menu
    static const int PLAYLIST_VISIBLE_ITEMS = 4;
    String playlist_cached_names[PLAYLIST_VISIBLE_ITEMS];
    int playlist_cache_start_index;  // -1 = cache invalid

    // --- Drawing Methods ---
    void drawBluetoothMenu();
    void drawPlaylistMenu();
    void drawNowPlayingScreen();
    void drawVolumeScreen();

    // --- Cache Methods ---
    void updatePlaylistCache(int start_index);

    // --- "Now Playing" screen state tracking ---
    String last_displayed_track;
    bool last_bt_status;
    PlayerState last_player_state;

    // --- Now Playing scrollers ---
    TextScroller np_title_scroller;
    TextScroller np_artist_scroller;
    TextScroller np_album_scroller;
    String last_np_title;  // Track title changes to reset scrollers

    // --- Now Playing drawing helpers ---
    void drawPlayStateIcon(int x, int y, PlayerState state);
    void drawBtIcon(int x, int y, bool connected);
    void drawProgressBar(int x, int y, int width, int height, float progress,
                         uint32_t position_sec, uint32_t duration_sec);
    void drawScrollingText(int x, int y, int available_width, const char* text,
                           TextScroller& scroller);
};

// Bluetooth icon 8x8 (XBM format)
static const unsigned char bt_icon_bits[] PROGMEM = {
    0x18, 0x28, 0x4a, 0x2c, 0x18, 0x2c, 0x4a, 0x28
};

// Bluetooth disconnected icon 8x8 (XBM format)
static const unsigned char bt_disconnected_bits[] PROGMEM = {
    0x18, 0x28, 0x4a, 0x2d, 0x1a, 0x2c, 0x4a, 0x28
};

// User/Artist icon 8x8 (XBM format)
static const unsigned char user_icon_bits[] PROGMEM = {
    0x18, 0x24, 0x24, 0x18, 0x18, 0x66, 0x42, 0x3c
};

// Album icon 8x8 (XBM format)
static const unsigned char album_icon_bits[] PROGMEM = {
    0x3c, 0x42, 0x99, 0xa5, 0xa5, 0x99, 0x42, 0x3c
};

#endif // DISPLAY_MANAGER_H
