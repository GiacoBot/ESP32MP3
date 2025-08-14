#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H

#include <Arduino.h>
#include <BluetoothA2DPSource.h>

class MusicPlayer; // Forward declaration

class BluetoothManager {
private:
    BluetoothA2DPSource a2dp_source;
    MusicPlayer* music_player;
    String target_device;
    bool is_connected;
    
public:
    BluetoothManager(const String& device_name);
    
    void setMusicPlayer(MusicPlayer* player);
    bool initialize(const String& local_name = "ESP32_MP3_Player");
    
    // Connection
    bool connect();
    void disconnect();
    bool isConnected() const { return is_connected; }
    
    // Static callbacks for A2DP
    static int32_t audioDataCallback(uint8_t* data, int32_t len);
    static void connectionStateCallback(esp_a2d_connection_state_t state, void* ptr);
    static void avrcCommandCallback(uint8_t key, bool isReleased);
    
private:
    static BluetoothManager* instance; // For static callbacks
};

#endif