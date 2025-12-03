#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "settings.h"
#include "MusicPlayer.h"
#include "PlaylistManager.h" 
#include "BluetoothManager.h"
#include "SerialController.h"
#include "AudioProcessor.h"
#include "InputManager.h"
#include "DisplayManager.h"
#include "AppState.h"
#include "InputEvents.h"

// --- Global Objects ---
MusicPlayer music_player;
PlaylistManager playlist_manager(MUSIC_ROOT);
BluetoothManager bluetooth_manager;
SerialController serial_controller;
AudioProcessor audio_processor;
InputManager input_manager;
DisplayManager display_manager;

// --- Global UI State ---
AppScreen current_screen = AppScreen::SCREEN_BLUETOOTH_SELECTION;
int bt_menu_selected = 0;
int bt_menu_offset = 0;
int playlist_menu_selected = 0;
int playlist_menu_offset = 0;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial) {}
    
    Serial.println("ESP32 Bluetooth MP3 Player Starting...");
    
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SPI.setFrequency(10e6);
    SPI.setDataMode(SPI_MODE0);

    display_manager.initialize();
    display_manager.setMusicPlayer(&music_player);
    display_manager.setBluetoothManager(&bluetooth_manager);
    display_manager.setPlaylistManager(&playlist_manager);
    
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD card initialization failed!");
        return;
    }
    Serial.println("SD card initialized successfully");
    
    if (!playlist_manager.scanForMP3Files() || playlist_manager.getTrackCount() == 0) {
        Serial.println("No MP3 files found on SD card!");
    }
    
    serial_controller.setMusicPlayer(&music_player);
    serial_controller.setPlaylistManager(&playlist_manager);
    serial_controller.setBluetoothManager(&bluetooth_manager);
    serial_controller.initialize();
    
    bluetooth_manager.setMusicPlayer(&music_player);
    if (!bluetooth_manager.initialize("ESP32_MP3_Player")) {
        Serial.println("Failed to initialize Bluetooth");
        return;
    }
    
    input_manager.initialize();
    
    Serial.println("System ready! Starting Bluetooth discovery...");
    bluetooth_manager.startDiscovery();
}

void loop() {
    serial_controller.handleInput();
    InputEvent event = input_manager.handleInputs();

    // --- State Machine Logic ---
    switch (current_screen) {
        case AppScreen::SCREEN_BLUETOOTH_SELECTION: {
            int device_count = bluetooth_manager.getDiscoveredDevices().size();
            switch (event) {
                case InputEvent::INPUT_EVENT_UP:
                    if (bt_menu_selected > 0) bt_menu_selected--;
                    break;
                case InputEvent::INPUT_EVENT_DOWN:
                    if (device_count > 0 && bt_menu_selected < device_count - 1) bt_menu_selected++;
                    break;
                case InputEvent::INPUT_EVENT_RIGHT:
                    current_screen = AppScreen::SCREEN_TRACK_SELECTION;
                    break;
                case InputEvent::INPUT_EVENT_LEFT:
                    current_screen = AppScreen::SCREEN_NOW_PLAYING;
                    break;
                case InputEvent::INPUT_EVENT_ENTER:
                    if (device_count > 0 && bt_menu_selected < device_count) {
                        const auto& devices = bluetooth_manager.getDiscoveredDevices();
                        bluetooth_manager.connect(devices[bt_menu_selected]);
                        current_screen = AppScreen::SCREEN_NOW_PLAYING;
                    }
                    break;
                default: break;
            }
            display_manager.setBluetoothMenuState(bt_menu_selected, bt_menu_offset);
            break;
        }

        case AppScreen::SCREEN_TRACK_SELECTION: {
            int track_count = playlist_manager.getTrackCount();
            switch (event) {
                case InputEvent::INPUT_EVENT_UP:
                    if (playlist_menu_selected > 0) playlist_menu_selected--;
                    break;
                case InputEvent::INPUT_EVENT_DOWN:
                    if (track_count > 0 && playlist_menu_selected < track_count - 1) playlist_menu_selected++;
                    break;
                case InputEvent::INPUT_EVENT_RIGHT:
                    current_screen = AppScreen::SCREEN_NOW_PLAYING;
                    break;
                case InputEvent::INPUT_EVENT_LEFT:
                    current_screen = AppScreen::SCREEN_BLUETOOTH_SELECTION;
                    break;
                case InputEvent::INPUT_EVENT_ENTER:
                    if (track_count > 0 && playlist_menu_selected < track_count) {
                        music_player.executeCommand(PlayerCommand::PLAY_TRACK, playlist_menu_selected);
                        current_screen = AppScreen::SCREEN_NOW_PLAYING;
                    }
                    break;
                default: break;
            }
            display_manager.setPlaylistMenuState(playlist_menu_selected, playlist_menu_offset);
            break;
        }

        case AppScreen::SCREEN_NOW_PLAYING: {
            switch (event) {
                case InputEvent::INPUT_EVENT_UP:
                    music_player.executeCommand(PlayerCommand::PREV_TRACK);
                    break;
                case InputEvent::INPUT_EVENT_DOWN:
                    music_player.executeCommand(PlayerCommand::NEXT_TRACK);
                    break;
                case InputEvent::INPUT_EVENT_RIGHT:
                    current_screen = AppScreen::SCREEN_BLUETOOTH_SELECTION;
                    break;
                case InputEvent::INPUT_EVENT_LEFT:
                    current_screen = AppScreen::SCREEN_TRACK_SELECTION;
                    break;
                case InputEvent::INPUT_EVENT_ENTER:
                    if (music_player.getState() == PlayerState::PLAYING) {
                        music_player.executeCommand(PlayerCommand::PAUSE);
                    } else {
                        music_player.executeCommand(PlayerCommand::PLAY);
                    }
                    break;
                default: break;
            }
            break;
        }
    }

    display_manager.update(current_screen);
    delay(20);
}
