#include "BluetoothManager.h"
#include "MusicPlayer.h"
#include "AudioProcessor.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"

// Static variable for callbacks
BluetoothManager* BluetoothManager::instance = nullptr;

// External references
extern AudioProcessor audio_processor;

BluetoothManager::BluetoothManager() :
    music_player(nullptr),
    connected(false),
    discovering(false) {
    instance = this; // For static callbacks
}

void BluetoothManager::setMusicPlayer(MusicPlayer* player) {
    music_player = player;
}

bool BluetoothManager::initialize(const String& local_name) {
    if (!music_player) {
        Serial.println("Error: MusicPlayer not set in BluetoothManager");
        return false;
    }
    
    Serial.println("Initializing Bluetooth...");

    // The A2DP library handles the Bluetooth stack initialization (btStart, bluedroid, etc.)
    // when we call start(). We just need to set up our callbacks first.
    
    // Initialize A2DP
    a2dp_source.set_local_name(local_name.c_str());
    a2dp_source.set_data_callback(audioDataCallback);
    a2dp_source.set_on_connection_state_changed(connectionStateCallback);
    a2dp_source.set_avrc_passthru_command_callback(avrcCommandCallback);

    // Set the callbacks for discovery, this is the proper way to use the library
    a2dp_source.set_ssid_callback(ssid_callback);
    a2dp_source.set_discovery_mode_callback(discovery_mode_callback);
    
    Serial.println("Bluetooth initialized successfully.");
    return true;
}

void BluetoothManager::startDiscovery() {
    if (discovering) {
        return;
    }
    Serial.println("Starting Bluetooth device discovery...");
    discovered_devices.clear();
    // The start call with an empty vector triggers the library's discovery process
    std::vector<const char*> empty_list;
    a2dp_source.start(empty_list);
}

void BluetoothManager::stopDiscovery() {
    if (discovering) {
        Serial.println("Stopping Bluetooth device discovery...");
        // The A2DP library doesn't expose a stop function,
        // but the underlying GAP function should work.
        if (esp_bt_gap_cancel_discovery() != ESP_OK) {
            Serial.println("Failed to cancel discovery");
        }
    }
}

bool BluetoothManager::isDiscovering() const {
    return discovering;
}

const std::vector<BluetoothDevice>& BluetoothManager::getDiscoveredDevices() const {
    return discovered_devices;
}

bool BluetoothManager::connect(const BluetoothDevice& device) {
    Serial.printf("Connecting to device: %s\n", device.name.c_str());
    stopDiscovery(); // Stop discovery before connecting
    
    // Use const_cast because the underlying C function esp_a2d_connect expects a non-const pointer,
    // but we know it does not modify the address.
    a2dp_source.connect(const_cast<uint8_t*>(device.address));
    return true;
}

void BluetoothManager::disconnect() {
    Serial.println("Disconnecting...");
    // The library doesn't expose a simple disconnect method.
    // Disconnection is usually handled by the sink device or by shutting down the source.
}

bool BluetoothManager::isConnected() const {
    return connected;
}

// --- Library Callback Implementations ---

bool BluetoothManager::ssid_callback(const char* ssid, esp_bd_addr_t address, int rssi) {
    if (!instance || ssid == nullptr) return false;

    Serial.printf("Found Device: %s, RSSI: %d\n", ssid, rssi);

    // Avoid adding duplicates
    for (const auto& dev : instance->discovered_devices) {
        if (memcmp(dev.address, address, ESP_BD_ADDR_LEN) == 0) {
            return false; // Already in the list
        }
    }

    // Add to list
    BluetoothDevice new_device;
    new_device.name = String(ssid);
    memcpy(new_device.address, address, ESP_BD_ADDR_LEN);
    instance->discovered_devices.push_back(new_device);

    // Return false tells the library to continue discovering and not to auto-connect
    return false;
}

void BluetoothManager::discovery_mode_callback(esp_bt_gap_discovery_state_t state) {
    if (!instance) return;
    if (state == ESP_BT_GAP_DISCOVERY_STOPPED) {
        Serial.println("Bluetooth discovery stopped.");
        instance->discovering = false;
    } else {
        Serial.println("Bluetooth discovery started.");
        instance->discovering = true;
    }
}


void BluetoothManager::connectionStateCallback(esp_a2d_connection_state_t state, void* ptr) {
    if (!instance) return;
    
    Serial.print("A2DP connection state: ");
    switch (state) {
        case ESP_A2D_CONNECTION_STATE_DISCONNECTED:
            Serial.println("DISCONNECTED");
            instance->connected = false;
            if (instance->music_player) instance->music_player->notifyConnectionStateChanged(false);
            break;
        case ESP_A2D_CONNECTION_STATE_CONNECTING:
            Serial.println("CONNECTING");
            instance->connected = false;
            break;
        case ESP_A2D_CONNECTION_STATE_CONNECTED:
            Serial.println("CONNECTED");
            instance->connected = true;
            if (instance->music_player) instance->music_player->notifyConnectionStateChanged(true);
            break;
        case ESP_A2D_CONNECTION_STATE_DISCONNECTING:
            Serial.println("DISCONNECTING");
            instance->connected = false;
            break;
    }
}

// --- Unchanged Callbacks ---

int32_t BluetoothManager::audioDataCallback(uint8_t* data, int32_t len) {
    if (!data || len <= 0) return 0;
    if (!instance || !instance->music_player) {
        memset(data, 0, len);
        return 0;
    }
    
    if (instance->music_player->isBusy()) {
        memset(data, 0, len);
        return len;
    }

    PlayerState state = instance->music_player->getState();
    if (state == PlayerState::STOPPED || state == PlayerState::PAUSED) {
        memset(data, 0, len);
        return len;
    }
    
    int32_t result = audio_processor.readAudioData(data, len);
    
    if (result == 0) {
        if (instance->music_player) instance->music_player->notifyTrackFinished();
        memset(data, 0, len);
        return len;
    }
    return result;
}

void BluetoothManager::avrcCommandCallback(uint8_t key, bool isReleased) {
    if (!instance || !instance->music_player || !isReleased || instance->music_player->isBusy()) return;
    
    Serial.print("AVRC Command: ");
    switch (key) {
        case ESP_AVRC_PT_CMD_PLAY:     Serial.println("PLAY"); instance->music_player->executeCommand(PlayerCommand::PLAY); break;
        case ESP_AVRC_PT_CMD_PAUSE:    Serial.println("PAUSE"); instance->music_player->executeCommand(PlayerCommand::PAUSE); break;
        case ESP_AVRC_PT_CMD_STOP:     Serial.println("STOP"); instance->music_player->executeCommand(PlayerCommand::STOP); break;
        case ESP_AVRC_PT_CMD_FORWARD:  Serial.println("NEXT"); instance->music_player->executeCommand(PlayerCommand::NEXT_TRACK); break;
        case ESP_AVRC_PT_CMD_BACKWARD: Serial.println("PREVIOUS"); instance->music_player->executeCommand(PlayerCommand::PREV_TRACK); break;
        case ESP_AVRC_PT_CMD_VOL_UP:   Serial.println("VOLUME UP"); instance->music_player->executeCommand(PlayerCommand::VOLUME_UP); break;
        case ESP_AVRC_PT_CMD_VOL_DOWN: Serial.println("VOLUME DOWN"); instance->music_player->executeCommand(PlayerCommand::VOLUME_DOWN); break;
        default: Serial.printf("Unknown: 0x%02X\n", key); break;
    }
}