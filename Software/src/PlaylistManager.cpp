#include "PlaylistManager.h"
#include <algorithm>

PlaylistManager::PlaylistManager(const String& root) :
    music_root(root), track_count(0) {
    if (!music_root.endsWith("/")) {
        music_root += "/";
    }
}

bool PlaylistManager::scanForMP3Files() {
    track_count = 0;
    file_offsets.clear();

    // Delete old index if it exists
    if (SD.exists(PLAYLIST_INDEX_FILE)) {
        SD.remove(PLAYLIST_INDEX_FILE);
    }

    File indexFile = SD.open(PLAYLIST_INDEX_FILE, FILE_WRITE);
    if (!indexFile) {
        Serial.println("Failed to create index file");
        return false;
    }

    File root = SD.open(music_root);
    if (!root || !root.isDirectory()) {
        Serial.println("Failed to open music directory");
        indexFile.close();
        return false;
    }

    Serial.println("Scanning for MP3 files...");
    scanDirectory(root, indexFile, "");
    root.close();
    indexFile.close();

    // Build offset table
    buildOffsetTable();

    Serial.printf("Found %d MP3 files, index created\n", track_count);
    return track_count > 0;
}

void PlaylistManager::scanDirectory(File dir, File& indexFile, const String& path) {
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;

        String entry_name = String(entry.name());
        String full_path = path.isEmpty() ? entry_name : path + "/" + entry_name;

        if (entry.isDirectory()) {
            scanDirectory(entry, indexFile, full_path);
        } else if (hasMP3Extension(entry_name)) {
            String absolute_path = music_root + full_path;
            absolute_path.replace("//", "/");

            // Write path to file (with newline)
            indexFile.println(absolute_path);
            track_count++;
        }
        entry.close();
    }
}

void PlaylistManager::buildOffsetTable() {
    file_offsets.clear();
    file_offsets.reserve(track_count);

    File indexFile = SD.open(PLAYLIST_INDEX_FILE, FILE_READ);
    if (!indexFile) return;

    uint32_t offset = 0;
    while (indexFile.available()) {
        file_offsets.push_back(offset);

        // Skip to next line
        while (indexFile.available()) {
            char c = indexFile.read();
            offset++;
            if (c == '\n') break;
        }
    }
    indexFile.close();

    track_count = file_offsets.size();
    Serial.printf("Offset table built: %d entries, %d bytes RAM\n",
                  track_count, track_count * 4);
}

bool PlaylistManager::loadIndex() {
    track_count = 0;
    file_offsets.clear();

    if (!SD.exists(PLAYLIST_INDEX_FILE)) {
        Serial.println("Index file not found, scanning...");
        return scanForMP3Files();
    }

    // Count lines and build offset table
    File indexFile = SD.open(PLAYLIST_INDEX_FILE, FILE_READ);
    if (!indexFile) {
        Serial.println("Failed to open index file");
        return false;
    }

    uint32_t offset = 0;
    while (indexFile.available()) {
        file_offsets.push_back(offset);

        // Skip to next line
        while (indexFile.available()) {
            char c = indexFile.read();
            offset++;
            if (c == '\n') break;
        }
    }
    indexFile.close();

    track_count = file_offsets.size();
    Serial.printf("Index loaded: %d tracks, %d bytes RAM\n",
                  track_count, track_count * 4);
    return track_count > 0;
}

String PlaylistManager::getTrackPath(int index) const {
    if (!isValidIndex(index)) return "";

    File indexFile = SD.open(PLAYLIST_INDEX_FILE, FILE_READ);
    if (!indexFile) return "";

    indexFile.seek(file_offsets[index]);
    String path = indexFile.readStringUntil('\n');
    indexFile.close();

    path.trim();  // Remove \r\n
    return path;
}

String PlaylistManager::getTrackName(int index) const {
    String path = getTrackPath(index);
    if (path.isEmpty()) return "Invalid";

    int last_slash = path.lastIndexOf('/');
    if (last_slash >= 0) {
        path = path.substring(last_slash + 1);
    }

    int dot_pos = path.lastIndexOf('.');
    if (dot_pos > 0) {
        path = path.substring(0, dot_pos);
    }

    return path;
}

bool PlaylistManager::isValidIndex(int index) const {
    return index >= 0 && index < (int)track_count;
}

bool PlaylistManager::hasMP3Extension(const String& filename) {
    int dot_pos = filename.lastIndexOf('.');
    if (dot_pos < 0) return false;

    String ext = filename.substring(dot_pos + 1);
    ext.toLowerCase();
    return ext == "mp3";
}
