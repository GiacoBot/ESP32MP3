#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H

#include <Arduino.h>
#include <vector>
#include "esp_gap_bt_api.h" // For GAP events and types
#include "BluetoothA2DPSource.h"

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

    // --- Volume Control ---
    void setVolume(uint8_t volume);
    uint8_t getVolume() const;
    void volumeUp(uint8_t step = 10);
    void volumeDown(uint8_t step = 10);
    bool hasVolumeChanged();
    void consumeVolumeChangeEvent();

    // --- Playback Position Tracking ---
    void resetPlaybackPosition();       // Call when track changes
    uint32_t getPlaybackSeconds() const; // Get elapsed playback time from PCM bytes sent

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
    BluetoothA2DPSource a2dp_source;
    MusicPlayer* music_player;
    bool connected;
    bool discovering;
    bool connecting;
    BluetoothDevice connected_device;
    BluetoothDevice connecting_device; // Temporarily store device info during connection attempt
    bool _connection_event_pending;

    // --- Volume State ---
    uint8_t _cached_volume;
    uint8_t _last_polled_volume;
    bool _volume_change_pending;

    // --- Playback Position Tracking ---
    uint64_t pcm_bytes_sent;            // Total PCM bytes sent via A2DP
    static const uint32_t SAMPLE_RATE = 44100;  // Standard A2DP sample rate

    std::vector<BluetoothDevice> discovered_devices;
    
    static BluetoothManager* instance; // For static callbacks
};

#endif // BLUETOOTHMANAGER_H