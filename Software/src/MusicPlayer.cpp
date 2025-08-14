#include "MusicPlayer.h"
#include "PlaylistManager.h"
#include "AudioProcessor.h"

// Global objects defined in main.cpp
extern PlaylistManager playlist_manager;
extern AudioProcessor audio_processor;

MusicPlayer::MusicPlayer() : 
    current_state(PlayerState::STOPPED),
    current_track_index(-1) {
}

void MusicPlayer::addStateChangeCallback(StateChangeCallback callback) {
    state_callbacks.push_back(callback);
}

void MusicPlayer::addLogCallback(LogCallback callback) {
    log_callbacks.push_back(callback);
}

bool MusicPlayer::executeCommand(PlayerCommand cmd, int parameter) {
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
        case PlayerCommand::VOLUME_DOWN:
            // To be implemented
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
    if (!playlist_manager.isValidIndex(index)) {
        return false;
    }
    
    String track_path = playlist_manager.getTrackPath(index);
    if (!audio_processor.openFile(track_path)) {
        logMessage("Failed to open: " + track_path);
        return false;
    }
    
    current_track_index = index;
    current_state = PlayerState::PLAYING;
    
    logMessage("Playing: " + playlist_manager.getTrackName(index));
    notifyStateChange();
    
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
    logMessage("Track finished");
    nextTrack();
}

void MusicPlayer::notifyConnectionStateChanged(bool connected) {
    if (connected) {
        logMessage("Bluetooth connected");
        // Auto-start the first track if available and if we're not already playing one
        if (playlist_manager.getTrackCount() > 0 && current_track_index == -1) {
            openTrack(0);
        }
    } else {
        logMessage("Bluetooth disconnected");
        // Stop playback when disconnected
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
