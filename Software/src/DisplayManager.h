#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MusicPlayer.h"
#include "BluetoothManager.h"

class DisplayManager {
public:
    DisplayManager();
    bool initialize();
    void update();
    void setMusicPlayer(MusicPlayer* player);
    void setBluetoothManager(BluetoothManager* btManager);

private:
    MusicPlayer* music_player;
    BluetoothManager* bluetooth_manager;
    Adafruit_SSD1306 display;
    bool is_initialized;

    // To track changes
    String last_displayed_track;
    bool last_bt_status;
    PlayerState last_player_state;
};

#endif // DISPLAY_MANAGER_H
