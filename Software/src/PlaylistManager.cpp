// PlaylistManager.cpp
#include "PlaylistManager.h"
#include <algorithm>

PlaylistManager::PlaylistManager(const String& root) :
    music_root(root) {
    if (!music_root.endsWith("/")) {
        music_root += "/";
    }
}

bool PlaylistManager::scanForMP3Files() {
    clearPlaylist();
    
    File root = SD.open(music_root);
    if (!root) {
        Serial.println("Failed to open music directory: " + music_root);
        return false;
    }
    
    if (!root.isDirectory()) {
        Serial.println("Music root is not a directory: " + music_root);
        root.close();
        return false;
    }
    
    Serial.println("Scanning for MP3 files in: " + music_root);
    scanDirectory(root, "");
    root.close();
    
    // Sort alphabetically
    sortPlaylist();
    
    Serial.printf("Found %d MP3 files\n", playlist.size());
    return true;
}

void PlaylistManager::scanDirectory(File dir, const String& path) {
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;
        
        String entry_name = String(entry.name());
        String full_path = path.isEmpty() ? entry_name : path + "/" + entry_name;
        
        // Normalize the path (remove double slashes)
        full_path.replace("//", "/");
        
        if (entry.isDirectory()) {
            // Recursive scan of subdirectories
            Serial.println("Scanning directory: " + full_path);
            scanDirectory(entry, full_path);
        } else if (hasMP3Extension(entry_name)) {
            // Add the full path from the SD card root
            String absolute_path = music_root + full_path;
            absolute_path.replace("//", "/");
            
            playlist.push_back(absolute_path);
            Serial.println("Found: " + absolute_path);
        }
        
        entry.close();
    }
}

bool PlaylistManager::hasMP3Extension(const String& filename) {
    int dot_pos = filename.lastIndexOf('.');
    if (dot_pos < 0) return false;
    
    String extension = filename.substring(dot_pos + 1);
    extension.toLowerCase();
    return extension == "mp3";
}

void PlaylistManager::clearPlaylist() {
    playlist.clear();
}

void PlaylistManager::sortPlaylist() {
    std::sort(playlist.begin(), playlist.end());
}

String PlaylistManager::getTrackPath(int index) const {
    if (!isValidIndex(index)) {
        return "";
    }
    return playlist[index];
}

String PlaylistManager::getTrackName(int index) const {
    if (!isValidIndex(index)) {
        return "Invalid";
    }
    
    String path = playlist[index];
    
    // Extract only the file name from the full path
    int last_slash = path.lastIndexOf('/');
    if (last_slash >= 0) {
        path = path.substring(last_slash + 1);
    }
    
    // Remove the .mp3 extension
    int dot_pos = path.lastIndexOf('.');
    if (dot_pos > 0) {
        path = path.substring(0, dot_pos);
    }
    
    return path;
}

bool PlaylistManager::isValidIndex(int index) const {
    return index >= 0 && index < (int)playlist.size();
}

void PlaylistManager::printPlaylist(int current_index) const {
    Serial.println("\n--- Playlist ---");
    
    if (playlist.empty()) {
        Serial.println("No tracks found");
        Serial.println("----------------");
        return;
    }
    
    for (size_t i = 0; i < playlist.size(); i++) {
        String marker = (i == current_index) ? " > " : "   ";
        String track_name = getTrackName(i);
        
        Serial.printf("%s%2d: %s\n", 
                     marker.c_str(), 
                     i + 1, 
                     track_name.c_str());
        
        // Also show the full path for debugging (optional)
        if (i == current_index) {
            Serial.printf("     Path: %s\n", playlist[i].c_str());
        }
    }
    
    Serial.printf("Total: %d tracks\n", playlist.size());
    Serial.println("----------------");
}