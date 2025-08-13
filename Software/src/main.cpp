#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <BluetoothA2DPSource.h>
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include <vector>
#include <algorithm>

// --- Configuration ---
const char* TARGET_DEVICE_NAME = "Lenovo LP40";
const int SD_CS_PIN = 5;  // Adjust to your wiring
const char* MUSIC_ROOT = "/";  // Root folder to scan for MP3s

// --- Global Objects ---
BluetoothA2DPSource a2dp_source;
File current_file;
MP3DecoderHelix mp3;
EncodedAudioStream decoder(&current_file, &mp3);

// --- Playlist Management ---
std::vector<String> playlist;
int current_track_index = -1;
bool is_playing = false;
bool is_paused = false;

// --- Utility Functions ---
bool hasMP3Extension(const String &filename) {
    int dot = filename.lastIndexOf('.');
    if (dot < 0) return false;
    String ext = filename.substring(dot + 1);
    ext.toLowerCase();
    return ext == "mp3";
}

void scanForMP3Files(File dir, String path = "") {
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;
        
        String fullPath = path + "/" + entry.name();
        fullPath.replace("//", "/");
        
        if (entry.isDirectory()) {
            // Recursive scan
            scanForMP3Files(entry, fullPath);
        } else if (hasMP3Extension(entry.name())) {
            playlist.push_back(fullPath);
            Serial.println("Found: " + fullPath);
        }
        entry.close();
    }
}

void buildPlaylist() {
    playlist.clear();
    File root = SD.open(MUSIC_ROOT);
    if (!root) {
        Serial.println("Failed to open music directory");
        return;
    }
    
    Serial.println("Scanning for MP3 files...");
    scanForMP3Files(root);
    root.close();
    
    // Sort alphabetically
    std::sort(playlist.begin(), playlist.end());
    
    Serial.printf("Found %d MP3 files\n", playlist.size());
}

void printPlaylist() {
    Serial.println("\n--- Playlist ---");
    for (size_t i = 0; i < playlist.size(); i++) {
        String marker = (i == current_track_index) ? " > " : "   ";
        Serial.printf("%s%2d: %s\n", marker.c_str(), i + 1, playlist[i].c_str());
    }
    Serial.printf("Total: %d tracks\n", playlist.size());
}

// --- Track Management ---
bool openTrack(int index) {
    if (index < 0 || index >= (int)playlist.size()) {
        return false;
    }
    
    // Close current file if open
    if (current_file) {
        current_file.close();
    }
    
    // Open new file
    current_file = SD.open(playlist[index]);
    if (!current_file) {
        Serial.printf("Failed to open: %s\n", playlist[index].c_str());
        return false;
    }
    
    Serial.printf("Playing: %s\n", playlist[index].c_str());
    current_track_index = index;
    
    // Initialize decoder with new file
    // Note: decoder was initialized with current_file reference in global declaration
    decoder.transformationReader().resizeResultQueue(1024 * 8);
    
    if (!decoder.begin()) {
        Serial.println("Decoder failed to start");
        current_file.close();
        return false;
    }
    
    is_playing = true;
    is_paused = false;
    return true;
}

void nextTrack() {
    if (playlist.empty()) return;
    
    int next_index = (current_track_index + 1) % playlist.size();
    openTrack(next_index);
}

void prevTrack() {
    if (playlist.empty()) return;
    
    int prev_index = (current_track_index - 1 + playlist.size()) % playlist.size();
    openTrack(prev_index);
}

void playTrack(int index) {
    if (index >= 0 && index < (int)playlist.size()) {
        openTrack(index);
    }
}

// --- A2DP Callbacks ---
int32_t get_audio_data(uint8_t *data, int32_t len) {
    if (!is_playing || is_paused || !current_file || !current_file.available()) {
        // No audio or paused - return silence
        memset(data, 0, len);
        return len;
    }
    
    int32_t result = decoder.readBytes(data, len);
    
    // If we can't read enough data, the track might be finished
    if (result == 0 && current_file && !current_file.available()) {
        Serial.println("Track finished, moving to next...");
        nextTrack();
        // Return silence for this buffer
        memset(data, 0, len);
        return len;
    }
    
    // If we didn't get the full buffer, fill the rest with silence
    if (result < len) {
        memset(data + result, 0, len - result);
        result = len;
    }
    
    // Feed the watchdog
    delay(1);
    
    return result;
}

void avrc_connection_state_changed(esp_a2d_connection_state_t state, void *ptr) {
    Serial.print("A2DP connection state: ");
    switch (state) {
        case ESP_A2D_CONNECTION_STATE_DISCONNECTED:
            Serial.println("DISCONNECTED");
            break;
        case ESP_A2D_CONNECTION_STATE_CONNECTING:
            Serial.println("CONNECTING");
            break;
        case ESP_A2D_CONNECTION_STATE_CONNECTED:
            Serial.println("CONNECTED");
            // Auto-start first track when connected
            if (playlist.size() > 0 && current_track_index == -1) {
                openTrack(0);
            }
            break;
        case ESP_A2D_CONNECTION_STATE_DISCONNECTING:
            Serial.println("DISCONNECTING");
            break;
    }
}

void avrc_passthru_command_received(uint8_t key, bool isReleased) {
    if (!isReleased) return; // Only process on key release
    
    Serial.print("AVRC Command: ");
    switch (key) {
        case ESP_AVRC_PT_CMD_PLAY:
            Serial.println("PLAY");
            is_paused = false;
            break;
        case ESP_AVRC_PT_CMD_PAUSE:
            Serial.println("PAUSE");
            is_paused = true;
            break;
        case ESP_AVRC_PT_CMD_STOP:
            Serial.println("STOP");
            is_playing = false;
            is_paused = false;
            break;
        case ESP_AVRC_PT_CMD_FORWARD:
            Serial.println("NEXT");
            nextTrack();
            break;
        case ESP_AVRC_PT_CMD_BACKWARD:
            Serial.println("PREVIOUS");
            prevTrack();
            break;
        case ESP_AVRC_PT_CMD_VOL_UP:
            Serial.println("VOLUME UP");
            break;
        case ESP_AVRC_PT_CMD_VOL_DOWN:
            Serial.println("VOLUME DOWN");
            break;
        default:
            Serial.printf("Unknown: 0x%02X\n", key);
            break;
    }
}

// --- Serial Commands ---
void printHelp() {
    Serial.println("\n--- ESP32 Bluetooth MP3 Player ---");
    Serial.println("Commands:");
    Serial.println("  c - Connect to Bluetooth device");
    Serial.println("  d - Disconnect");
    Serial.println("  p - Pause/Resume");
    Serial.println("  n - Next track");
    Serial.println("  b - Previous track");
    Serial.println("  l - List playlist");
    Serial.println("  r - Rescan SD card");
    Serial.println("  1-9 - Play track number (1-9)");
    Serial.println("  + - Volume up");
    Serial.println("  - - Volume down");
    Serial.println("  s - Show current status");
    Serial.println("  h - Show help");
    Serial.println("------------------------------------");
}

void printStatus() {
    Serial.println("\n--- Status ---");
    Serial.printf("Playlist: %d tracks\n", playlist.size());
    if (current_track_index >= 0) {
        Serial.printf("Current: %d - %s\n", current_track_index + 1, 
                     playlist[current_track_index].c_str());
    } else {
        Serial.println("Current: None");
    }
    Serial.printf("State: %s\n", is_paused ? "Paused" : (is_playing ? "Playing" : "Stopped"));
    Serial.println("-------------");
}

void handleSerialInput() {
    if (!Serial.available()) return;
    
    char cmd = Serial.read();
    
    switch (cmd) {
        case 'c':
            Serial.printf("Connecting to: %s\n", TARGET_DEVICE_NAME);
            a2dp_source.start(TARGET_DEVICE_NAME);
            break;
            
        case 'd':
            Serial.println("Disconnecting...");
            a2dp_source.disconnect();
            is_playing = false;
            is_paused = false;
            break;
            
        case 'p':
            is_paused = !is_paused;
            Serial.println(is_paused ? "Paused" : "Resumed");
            break;
            
        case 'n':
            Serial.println("Next track");
            nextTrack();
            break;
            
        case 'b':
            Serial.println("Previous track");
            prevTrack();
            break;
            
        case 'l':
            printPlaylist();
            break;
            
        case 'r':
            Serial.println("Rescanning SD card...");
            buildPlaylist();
            break;
            
        case 's':
            printStatus();
            break;
            
        case '+':
            Serial.println("Volume up");
            // Send AVRC command
            break;
            
        case '-':
            Serial.println("Volume down");
            // Send AVRC command
            break;
            
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9':
            {
                int track_num = cmd - '0' - 1; // Convert to 0-based index
                if (track_num < (int)playlist.size()) {
                    Serial.printf("Playing track %d\n", track_num + 1);
                    playTrack(track_num);
                } else {
                    Serial.println("Track number out of range");
                }
            }
            break;
            
        case 'h':
        default:
            printHelp();
            break;
    }
}

// --- Setup ---
void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    
    Serial.println("ESP32 Bluetooth MP3 Player Starting...");
    
    // Initialize SD card
    Serial.println("Initializing SD card...");
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD card initialization failed!");
        Serial.println("Check wiring and SD card.");
        return;
    }
    Serial.println("SD card initialized successfully");
    
    // Build playlist
    buildPlaylist();
    
    if (playlist.empty()) {
        Serial.println("No MP3 files found on SD card!");
    }
    
    // Initialize A2DP
    Serial.println("Initializing Bluetooth A2DP...");
    a2dp_source.set_local_name("ESP32_MP3_Player");
    a2dp_source.set_data_callback(get_audio_data);
    a2dp_source.set_on_connection_state_changed(avrc_connection_state_changed);
    a2dp_source.set_avrc_passthru_command_callback(avrc_passthru_command_received);
    
    // Start A2DP (discoverable mode)
    a2dp_source.start();
    
    printHelp();
    Serial.println("Ready! Type 'c' to connect or 'h' for help.");
}

// --- Main Loop ---
void loop() {
    handleSerialInput();
    delay(10); // Small delay to prevent watchdog issues
}