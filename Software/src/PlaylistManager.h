#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <Arduino.h>
#include <SD.h>
#include <vector>

class PlaylistManager {
private:
    std::vector<String> playlist;
    String music_root;
    
public:
    PlaylistManager(const String& root = "/");
    
    // Playlist management
    bool scanForMP3Files();
    void clearPlaylist();
    void sortPlaylist();
    
    // Data access
    size_t getTrackCount() const { return playlist.size(); }
    String getTrackPath(int index) const;
    String getTrackName(int index) const;
    const std::vector<String>& getPlaylist() const { return playlist; }
    
    // Utilities
    bool isValidIndex(int index) const;
    void printPlaylist(int current_index = -1) const;
    
private:
    bool hasMP3Extension(const String& filename);
    void scanDirectory(File dir, const String& path = "");
};

#endif