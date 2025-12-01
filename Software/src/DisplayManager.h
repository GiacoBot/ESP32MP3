#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MusicPlayer.h"

class DisplayManager {
public:
    DisplayManager();
    bool initialize();
    void update();
    void setMusicPlayer(MusicPlayer* player);

private:
    MusicPlayer* music_player;
    Adafruit_SSD1306 display;
    bool is_initialized;
    String last_displayed_track;
};

#endif // DISPLAY_MANAGER_H
