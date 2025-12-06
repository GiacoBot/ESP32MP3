#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H

#include <Arduino.h>
#include <vector>
#include "esp_gap_bt_api.h" // For GAP events and types
#include "CustomA2DPSource.h"

class MusicPlayer; // Forward declaration

// Represents a discovered Bluetooth device
struct BluetoothDevice {
    String name;
    esp_bd_addr_t address;
};

class BluetoothManager {
public:
    BluetoothManager();
    
    void setMusicPlayer(MusicPlayer* player);
    bool initialize(const String& local_name = "ESP32_MP3_Player");
    
    // --- Discovery ---
    void startDiscovery();
    void stopDiscovery();
    bool isDiscovering() const;
    const std::vector<BluetoothDevice>& getDiscoveredDevices() const;

    // --- Connection ---
    bool connect(const BluetoothDevice& device);
    void disconnect();
    bool isConnected() const;
    bool isConnecting() const;
    String getConnectedDeviceName() const;
    String getConnectingDeviceName() const;
    bool hasConnectionEvent() const;
    void consumeConnectionEvent();
    
    // --- Static Callbacks (for A2DP Library hooks) ---
    // Callback for A2DP audio data
    static int32_t audioDataCallback(uint8_t* data, int32_t len);
    // Callback for A2DP connection state changes
    static void connectionStateCallback(esp_a2d_connection_state_t state, void* ptr);
    // Callback for AVRC commands (e.g., play, pause from remote)
    static void avrcCommandCallback(uint8_t key, bool isReleased);
    // Callback for when a device is discovered by the A2DP library
    static bool ssid_callback(const char* ssid, esp_bd_addr_t address, int rssi);
    // Callback for when the discovery process starts or stops
    static void discovery_mode_callback(esp_bt_gap_discovery_state_t state);

private:
    CustomA2DPSource a2dp_source;
    MusicPlayer* music_player;
    bool connected;
    bool discovering;
    bool connecting;
    BluetoothDevice connected_device;
    BluetoothDevice connecting_device; // Temporarily store device info during connection attempt
    bool _connection_event_pending;

    std::vector<BluetoothDevice> discovered_devices;
    
    static BluetoothManager* instance; // For static callbacks
};

#endif // BLUETOOTHMANAGER_H