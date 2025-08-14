#ifndef SERIALCONTROLLER_H
#define SERIALCONTROLLER_H

#include <Arduino.h>
#include "MusicPlayer.h"
#include "PlaylistManager.h"
#include "BluetoothManager.h"

class SerialController {
private:
    MusicPlayer* music_player;
    PlaylistManager* playlist_manager;
    BluetoothManager* bluetooth_manager;
    
public:
    SerialController();
    
    void setMusicPlayer(MusicPlayer* player);
    void setPlaylistManager(PlaylistManager* playlist);
    void setBluetoothManager(BluetoothManager* bluetooth);
    
    void initialize();
    void handleInput(); // To be called in the loop
    
private:
    void printHelp();
    void printStatus();
    void executeCommand(char cmd);
    
    // Callbacks
    void onStateChange(PlayerState state, int track_index, const String& track_name);
    void onLogMessage(const String& message);
};

#endif