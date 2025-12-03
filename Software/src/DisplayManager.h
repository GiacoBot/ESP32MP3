#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <U8g2lib.h>
#include "MusicPlayer.h"
#include "BluetoothManager.h"
#include "settings.h"

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
    U8G2_DISPLAY_TYPE u8g2;
    bool is_initialized;

    // To track changes
    String last_displayed_track;
    bool last_bt_status;
    PlayerState last_player_state;
};


#endif // DISPLAY_MANAGER_H
