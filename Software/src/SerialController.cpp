// SerialController.cpp
#include "SerialController.h"

SerialController::SerialController() :
    music_player(nullptr),
    playlist_manager(nullptr),
    bluetooth_manager(nullptr) {
}

void SerialController::setMusicPlayer(MusicPlayer* player) {
    music_player = player;
}

void SerialController::setPlaylistManager(PlaylistManager* playlist) {
    playlist_manager = playlist;
}

void SerialController::setBluetoothManager(BluetoothManager* bluetooth) {
    bluetooth_manager = bluetooth;
}

void SerialController::initialize() {
    if (music_player) {
        // Registra i callback per ricevere notifiche
        music_player->addStateChangeCallback([this](PlayerState state, int track_index, const String& track_name) {
            onStateChange(state, track_index, track_name);
        });
        
        music_player->addLogCallback([this](const String& message) {
            onLogMessage(message);
        });
    }
    
    printHelp();
}

void SerialController::handleInput() {
    if (!Serial.available()) return;
    
    char cmd = Serial.read();
    
    // Consuma eventuali caratteri extra (newline, ecc.)
    while (Serial.available()) {
        Serial.read();
    }
    
    executeCommand(cmd);
}

void SerialController::executeCommand(char cmd) {
    switch (cmd) {
        case 'c':
            if (bluetooth_manager) {
                bluetooth_manager->connect();
            } else {
                Serial.println("Error: BluetoothManager not available");
            }
            break;
            
        case 'd':
            if (bluetooth_manager) {
                bluetooth_manager->disconnect();
            } else {
                Serial.println("Error: BluetoothManager not available");
            }
            break;
            
        case 'p':
            if (music_player) {
                PlayerState current_state = music_player->getState();
                if (current_state == PlayerState::PLAYING) {
                    music_player->executeCommand(PlayerCommand::PAUSE);
                } else {
                    music_player->executeCommand(PlayerCommand::PLAY);
                }
            }
            break;
            
        case 'n':
            if (music_player) {
                music_player->executeCommand(PlayerCommand::NEXT_TRACK);
            }
            break;
            
        case 'b':
            if (music_player) {
                music_player->executeCommand(PlayerCommand::PREV_TRACK);
            }
            break;
            
        case 'l':
            if (playlist_manager) {
                int current_index = music_player ? music_player->getCurrentTrackIndex() : -1;
                playlist_manager->printPlaylist(current_index);
            } else {
                Serial.println("Error: PlaylistManager not available");
            }
            break;
            
        case 'r':
            if (playlist_manager) {
                Serial.println("Rescanning SD card...");
                if (playlist_manager->scanForMP3Files()) {
                    Serial.printf("Scan completed. Found %d tracks.\n", playlist_manager->getTrackCount());
                } else {
                    Serial.println("Scan failed.");
                }
            } else {
                Serial.println("Error: PlaylistManager not available");
            }
            break;
            
        case 's':
            printStatus();
            break;
            
        case '+':
            if (music_player) {
                music_player->executeCommand(PlayerCommand::VOLUME_UP);
                Serial.println("Volume up"); // Placeholder
            }
            break;
            
        case '-':
            if (music_player) {
                music_player->executeCommand(PlayerCommand::VOLUME_DOWN);
                Serial.println("Volume down"); // Placeholder
            }
            break;
            
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9':
            {
                int track_num = cmd - '0' - 1; // Converti a indice base 0
                if (music_player && playlist_manager) {
                    if (track_num < (int)playlist_manager->getTrackCount()) {
                        Serial.printf("Playing track %d\n", track_num + 1);
                        music_player->executeCommand(PlayerCommand::PLAY_TRACK, track_num);
                    } else {
                        Serial.println("Track number out of range");
                    }
                } else {
                    Serial.println("Error: Player or playlist not available");
                }
            }
            break;
            
        case 'h':
        default:
            printHelp();
            break;
    }
}

void SerialController::printHelp() {
    Serial.println("\n--- ESP32 Bluetooth MP3 Player ---");
    Serial.println("Commands:");
    Serial.println(" c - Connect to Bluetooth device");
    Serial.println(" d - Disconnect");
    Serial.println(" p - Pause/Resume");
    Serial.println(" n - Next track");
    Serial.println(" b - Previous track");
    Serial.println(" l - List playlist");
    Serial.println(" r - Rescan SD card");
    Serial.println(" 1-9 - Play track number (1-9)");
    Serial.println(" + - Volume up");
    Serial.println(" - - Volume down");
    Serial.println(" s - Show current status");
    Serial.println(" h - Show help");
    Serial.println("------------------------------------");
}

void SerialController::printStatus() {
    Serial.println("\n--- Status ---");
    
    if (playlist_manager) {
        Serial.printf("Playlist: %d tracks\n", playlist_manager->getTrackCount());
    } else {
        Serial.println("Playlist: Not available");
    }
    
    if (music_player) {
        if (music_player->getCurrentTrackIndex() >= 0) {
            Serial.printf("Current: %d - %s\n", 
                         music_player->getCurrentTrackIndex() + 1,
                         music_player->getCurrentTrackName().c_str());
        } else {
            Serial.println("Current: None");
        }
        
        PlayerState state = music_player->getState();
        const char* state_str;
        switch (state) {
            case PlayerState::PLAYING: state_str = "Playing"; break;
            case PlayerState::PAUSED:  state_str = "Paused"; break;
            case PlayerState::STOPPED: state_str = "Stopped"; break;
            default: state_str = "Unknown"; break;
        }
        Serial.printf("State: %s\n", state_str);
    } else {
        Serial.println("Player: Not available");
    }
    
    if (bluetooth_manager) {
        Serial.printf("Bluetooth: %s\n", bluetooth_manager->isConnected() ? "Connected" : "Disconnected");
    } else {
        Serial.println("Bluetooth: Not available");
    }
    
    Serial.println("-------------");
}

void SerialController::onStateChange(PlayerState state, int track_index, const String& track_name) {
    // Callback called when the player state changes
    // You might want to print notifications here, but I avoid spamming
    // Serial.printf("[State] %s - Track %d: %s\n", state_str, track_index + 1, track_name.c_str());
}

void SerialController::onLogMessage(const String& message) {
    // Print log messages from the MusicPlayer
    Serial.println("[Player] " + message);
}