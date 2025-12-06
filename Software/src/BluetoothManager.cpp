#include "BluetoothManager.h"
#include "MusicPlayer.h"
#include "AudioProcessor.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"

// Static variable for callbacks
BluetoothManager* BluetoothManager::instance = nullptr;

// External references
extern AudioProcessor audio_processor;

BluetoothManager::BluetoothManager() :
    music_player(nullptr),
    connected(false),
    discovering(false),
    connecting(false),
    _connection_event_pending(false) { // Initialize the new flag
    instance = this; // For static callbacks
    memset(&connected_device, 0, sizeof(BluetoothDevice));
    memset(&connecting_device, 0, sizeof(BluetoothDevice));
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
    a2dp_source.set_local_name(local_name.c_str());
    a2dp_source.set_data_callback(audioDataCallback);
    a2dp_source.set_on_connection_state_changed(connectionStateCallback);
    a2dp_source.set_avrc_passthru_command_callback(avrcCommandCallback);
    a2dp_source.set_ssid_callback(ssid_callback);
    a2dp_source.set_discovery_mode_callback(discovery_mode_callback);
    
    Serial.println("Bluetooth initialized successfully.");
    return true;
}

void BluetoothManager::startDiscovery() {
    // Do not start a new discovery if already connected, connecting, or discovering
    if (connected || connecting || discovering) {
        return;
    }
    Serial.println("Starting Bluetooth device discovery...");
    discovered_devices.clear();
    std::vector<const char*> empty_list;
    a2dp_source.start(empty_list);
}

void BluetoothManager::stopDiscovery() {
    if (discovering) {
        Serial.println("Stopping Bluetooth device discovery...");
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
    if (discovering) {
        stopDiscovery();
    }
    this->connecting = true;
    this->connecting_device = device;
    a2dp_source.connect(const_cast<uint8_t*>(device.address));
    return true;
}

void BluetoothManager::disconnect() {
    if (connected) {
        Serial.printf("Disconnecting from %s...\n", connected_device.name.c_str());
        esp_a2d_source_disconnect(connected_device.address);
    }
}

bool BluetoothManager::isConnected() const {
    return connected;
}

bool BluetoothManager::isConnecting() const {
    return connecting;
}

String BluetoothManager::getConnectedDeviceName() const {
    return connected_device.name;
}

String BluetoothManager::getConnectingDeviceName() const {
    return connecting_device.name;
}

bool BluetoothManager::hasConnectionEvent() const {
    return _connection_event_pending;
}

void BluetoothManager::consumeConnectionEvent() {
    _connection_event_pending = false;
}

// --- Library Callback Implementations ---

bool BluetoothManager::ssid_callback(const char* ssid, esp_bd_addr_t address, int rssi) {
    if (!instance || ssid == nullptr || strlen(ssid) == 0) {
        return false;
    }

    for (const auto& dev : instance->discovered_devices) {
        if (memcmp(dev.address, address, ESP_BD_ADDR_LEN) == 0) {
            return false; // Already in the list
        }
    }

    Serial.printf("Found Device: %s, RSSI: %d\n", ssid, rssi);
    BluetoothDevice new_device;
    new_device.name = String(ssid);
    memcpy(new_device.address, address, ESP_BD_ADDR_LEN);
    instance->discovered_devices.push_back(new_device);

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
            instance->connecting = false;
            memset(&instance->connected_device, 0, sizeof(BluetoothDevice));
            if (instance->music_player) instance->music_player->notifyConnectionStateChanged(false);
            instance->startDiscovery();
            break;
        case ESP_A2D_CONNECTION_STATE_CONNECTING:
            Serial.println("CONNECTING");
            instance->connecting = true;
            instance->connected = false;
            break;
        case ESP_A2D_CONNECTION_STATE_CONNECTED:
            Serial.println("CONNECTED");
            instance->connected = true;
            instance->connecting = false;
            instance->connected_device = instance->connecting_device;
            Serial.printf("Stored connected device: %s\n", instance->connected_device.name.c_str());
            if (instance->music_player) instance->music_player->notifyConnectionStateChanged(true);
            instance->_connection_event_pending = true; // Signal the event
            break;
        case ESP_A2D_CONNECTION_STATE_DISCONNECTING:
            Serial.println("DISCONNECTING");
            instance->connecting = true; // Treat disconnecting as a form of "connecting" state for UI feedback
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