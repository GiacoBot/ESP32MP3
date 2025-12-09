#include "MusicPlayer.h"
#include "PlaylistManager.h"
#include "AudioProcessor.h"

// Global objects defined in main.cpp
extern PlaylistManager playlist_manager;
extern AudioProcessor audio_processor;



MusicPlayer::MusicPlayer() : 
    current_state(PlayerState::STOPPED),
    current_track_index(-1),
    is_busy(false),
    current_volume(0.5f) {
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
            setVolume(current_volume + VOLUME_STEP);
            return true;
        case PlayerCommand::VOLUME_DOWN:
            setVolume(current_volume - VOLUME_STEP);
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

bool MusicPlayer::openTrack(int index) {
    setBusy(true);

    if (!playlist_manager.isValidIndex(index)) {
        setBusy(false);
        return false;
    }
    
    String track_path = playlist_manager.getTrackPath(index);
    if (!audio_processor.openFile(track_path)) {
        logMessage("Failed to open: " + track_path);
        setBusy(false);
        return false;
    }
    
    current_track_index = index;
    current_state = PlayerState::PLAYING;
    
    logMessage("Playing: " + playlist_manager.getTrackName(index));
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
    if (current_track_index >= 0) {
        return playlist_manager.getTrackName(current_track_index);
    }
    return "None";
}

void MusicPlayer::setVolume(float new_volume) {
    // 1. Clamp volume between 0.0 and 1.0
    if (new_volume > 1.0f) new_volume = 1.0f;
    if (new_volume < 0.0f) new_volume = 0.0f;

    // 2. Only update if changed
    if (current_volume != new_volume) {
        current_volume = new_volume;
        
        // 3. Apply to AudioProcessor (See note below)
        audio_processor.setVolume(current_volume); 

        // 4. Log the change (as percentage)
        int vol_percent = (int)(current_volume * 100);
        logMessage("Volume: " + String(vol_percent) + "%");
    }
}