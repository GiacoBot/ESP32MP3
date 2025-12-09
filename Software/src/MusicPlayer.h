#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <Arduino.h>
#include <vector>

enum class PlayerState {
    STOPPED,
    PLAYING,
    PAUSED
};

enum class PlayerCommand {
    PLAY,
    PAUSE,
    STOP,
    NEXT_TRACK,
    PREV_TRACK,
    PLAY_TRACK,
    VOLUME_UP,
    VOLUME_DOWN
};

// Callback to notify state changes
typedef std::function<void(PlayerState state, int track_index, const String& track_name)> StateChangeCallback;
typedef std::function<void(const String& message)> LogCallback;

class MusicPlayer {
private:
    PlayerState current_state;
    int current_track_index;
    std::vector<StateChangeCallback> state_callbacks;
    std::vector<LogCallback> log_callbacks;
    volatile bool is_busy; // Concurrency flag
    const float VOLUME_STEP = 0.1f;
    float current_volume;
    
public:
    MusicPlayer();
    
    // Callback management
    void addStateChangeCallback(StateChangeCallback callback);
    void addLogCallback(LogCallback callback);
    
    // Main controls
    bool executeCommand(PlayerCommand cmd, int parameter = -1);
    
    // Player status
    PlayerState getState() const { return current_state; }
    int getCurrentTrackIndex() const { return current_track_index; }
    int getTrackCount() const;
    String getCurrentTrackName() const;
    bool isBusy() const { return is_busy; }
    float getVolume() { return current_volume; }
    
    // For internal use (calls from A2DP callbacks)
    void notifyTrackFinished();
    void notifyConnectionStateChanged(bool connected);
    
private:
    void setBusy(bool busy_state) { is_busy = busy_state; }
    void notifyStateChange();
    void logMessage(const String& message);
    bool openTrack(int index);
    void nextTrack();
    void prevTrack();
    void setVolume(float new_volume);
};

#endif
