#include "MusicPlayer.h"
#include "PlaylistManager.h"
#include "AudioProcessor.h"
#include "BluetoothManager.h"
#include <SD.h>
#include "AudioTools/CoreAudio/AudioMetaData/MetaDataID3.h"

// Global objects defined in main.cpp
extern PlaylistManager playlist_manager;
extern AudioProcessor audio_processor;
extern BluetoothManager bluetooth_manager;

// Static pointer for metadata callback context
static TrackMetadata* s_metadata_ptr = nullptr;

// Callback for ID3 metadata parsing
static void id3Callback(audio_tools::MetaDataType type, const char* str, int len) {
    if (s_metadata_ptr == nullptr || str == nullptr || len <= 0) return;

    // Create null-terminated copy
    char buffer[65];
    int copy_len = (len < 64) ? len : 64;
    memcpy(buffer, str, copy_len);
    buffer[copy_len] = '\0';

    switch (type) {
        case audio_tools::Title:
            s_metadata_ptr->setTitle(buffer);
            s_metadata_ptr->has_metadata = true;
            break;
        case audio_tools::Artist:
            s_metadata_ptr->setArtist(buffer);
            s_metadata_ptr->has_metadata = true;
            break;
        case audio_tools::Album:
            s_metadata_ptr->setAlbum(buffer);
            s_metadata_ptr->has_metadata = true;
            break;
        default:
            break;
    }
}

MusicPlayer::MusicPlayer() :
    current_state(PlayerState::STOPPED),
    current_track_index(-1),
    current_track_name("None"),
    is_busy(false) {
    current_metadata.clear();
}

void MusicPlayer::addStateChangeCallback(StateChangeCallback callback) {
    state_callbacks.push_back(callback);
}

void MusicPlayer::addLogCallback(LogCallback callback) {
    log_callbacks.push_back(callback);
}

bool MusicPlayer::executeCommand(PlayerCommand cmd, int parameter) {
    if (is_busy) return false; // Don't accept commands while busy

    switch (cmd) {
        case PlayerCommand::PLAY:
            if (current_state == PlayerState::PAUSED) {
                current_state = PlayerState::PLAYING;
                logMessage("Resumed");
                notifyStateChange();
                return true;
            } else if (current_track_index >= 0) {
                current_state = PlayerState::PLAYING;
                logMessage("Playing");
                notifyStateChange();
                return true;
            }
            return false;
            
        case PlayerCommand::PAUSE:
            if (current_state == PlayerState::PLAYING) {
                current_state = PlayerState::PAUSED;
                logMessage("Paused");
                notifyStateChange();
                return true;
            }
            return false;
            
        case PlayerCommand::STOP:
            current_state = PlayerState::STOPPED;
            logMessage("Stopped");
            notifyStateChange();
            return true;
            
        case PlayerCommand::NEXT_TRACK:
            nextTrack();
            return true;
            
        case PlayerCommand::PREV_TRACK:
            prevTrack();
            return true;
            
        case PlayerCommand::PLAY_TRACK:
            if (parameter >= 0) {
                return openTrack(parameter);
            }
            return false;
            
        case PlayerCommand::VOLUME_UP:
            bluetooth_manager.volumeUp();
            return true;
        case PlayerCommand::VOLUME_DOWN:
            bluetooth_manager.volumeDown();
            return true;
    }
    return false;
}

void MusicPlayer::nextTrack() {
    if (playlist_manager.getTrackCount() == 0) return;
    
    int next_index = (current_track_index + 1) % playlist_manager.getTrackCount();
    openTrack(next_index);
}

void MusicPlayer::prevTrack() {
    if (playlist_manager.getTrackCount() == 0) return;
    
    int prev_index = (current_track_index - 1 + playlist_manager.getTrackCount()) % playlist_manager.getTrackCount();
    openTrack(prev_index);
}

void MusicPlayer::extractMetadata(const String& filepath) {
    current_metadata.clear();

    File file = SD.open(filepath);
    if (!file) {
        return;
    }

    // Read first 4KB for ID3 tags (ID3v2 is at beginning of file)
    const size_t BUFFER_SIZE = 4096;
    uint8_t buffer[BUFFER_SIZE];
    size_t bytes_read = file.read(buffer, BUFFER_SIZE);
    file.close();

    if (bytes_read == 0) {
        return;
    }

    // Set up static pointer for callback
    s_metadata_ptr = &current_metadata;

    // Parse ID3 tags
    audio_tools::MetaDataID3 id3;
    id3.setCallback(id3Callback);
    id3.begin();
    id3.write(buffer, bytes_read);
    id3.end();

    // Clear static pointer
    s_metadata_ptr = nullptr;

    // If no title found, use filename as fallback
    if (current_metadata.title[0] == '\0') {
        // Extract filename from path
        int last_slash = filepath.lastIndexOf('/');
        String filename = (last_slash >= 0) ? filepath.substring(last_slash + 1) : filepath;
        // Remove .mp3 extension if present
        if (filename.endsWith(".mp3") || filename.endsWith(".MP3")) {
            filename = filename.substring(0, filename.length() - 4);
        }
        current_metadata.setTitle(filename.c_str());
    }
}

bool MusicPlayer::openTrack(int index) {
    setBusy(true);

    if (!playlist_manager.isValidIndex(index)) {
        setBusy(false);
        return false;
    }

    String track_path = playlist_manager.getTrackPath(index);

    // Extract ID3 metadata before opening for audio
    extractMetadata(track_path);

    if (!audio_processor.openFile(track_path)) {
        logMessage("Failed to open: " + track_path);
        setBusy(false);
        return false;
    }

    // Reset playback position counter for new track
    bluetooth_manager.resetPlaybackPosition();

    current_track_index = index;
    current_track_name = playlist_manager.getTrackName(index);  // Cache nome
    current_state = PlayerState::PLAYING;

    logMessage("Playing: " + current_track_name);
    notifyStateChange();

    setBusy(false);
    return true;
}

void MusicPlayer::notifyStateChange() {
    String track_name = getCurrentTrackName();
    for (auto& callback : state_callbacks) {
        callback(current_state, current_track_index, track_name);
    }
}

void MusicPlayer::logMessage(const String& message) {
    for (auto& callback : log_callbacks) {
        callback(message);
    }
}

void MusicPlayer::notifyTrackFinished() {
    if (is_busy) return;
    logMessage("Track finished");
    nextTrack();
}

void MusicPlayer::notifyConnectionStateChanged(bool connected) {
    if (connected) {
        logMessage("Bluetooth connected");
        if (playlist_manager.getTrackCount() > 0 && current_track_index == -1) {
            openTrack(0);
        }else{
            current_state = PlayerState::PLAYING;
            notifyStateChange();
        }
    } else {
        logMessage("Bluetooth disconnected");
        current_state = PlayerState::STOPPED;
        notifyStateChange();
    }
}

int MusicPlayer::getTrackCount() const {
    return playlist_manager.getTrackCount();
}

String MusicPlayer::getCurrentTrackName() const {
    return current_track_name;  // Ritorna cache, nessuna lettura SD
}