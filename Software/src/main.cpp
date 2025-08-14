#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "MusicPlayer.h"
#include "PlaylistManager.h" 
#include "BluetoothManager.h"
#include "SerialController.h"
#include "AudioProcessor.h"

// --- Configuration ---
const char* TARGET_DEVICE_NAME = "Lenovo LP40";
const int SD_CS_PIN = 5;
const char* MUSIC_ROOT = "/";

// --- Global Objects ---
MusicPlayer music_player;
PlaylistManager playlist_manager(MUSIC_ROOT);
BluetoothManager bluetooth_manager(TARGET_DEVICE_NAME);
SerialController serial_controller;
AudioProcessor audio_processor;

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    
    Serial.println("ESP32 Bluetooth MP3 Player Starting...");
    
    // Initialize SD card
    Serial.println("Initializing SD card...");
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD card initialization failed!");
        return;
    }
    Serial.println("SD card initialized successfully");
    
    // Build playlist
    if (!playlist_manager.scanForMP3Files()) {
        Serial.println("Failed to scan for MP3 files");
    }
    
    if (playlist_manager.getTrackCount() == 0) {
        Serial.println("No MP3 files found on SD card!");
    }
    
    // Setup controllers
    serial_controller.setMusicPlayer(&music_player);
    serial_controller.setPlaylistManager(&playlist_manager);
    serial_controller.setBluetoothManager(&bluetooth_manager);
    serial_controller.initialize();
    
    // Setup bluetooth
    bluetooth_manager.setMusicPlayer(&music_player);
    if (!bluetooth_manager.initialize("ESP32_MP3_Player")) {
        Serial.println("Failed to initialize Bluetooth");
        return;
    }
    
    Serial.println("System ready! Type 'c' to connect or 'h' for help.");
}

void loop() {
    serial_controller.handleInput();
    delay(10);
}
