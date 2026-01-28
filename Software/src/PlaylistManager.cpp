#include "PlaylistManager.h"

PlaylistManager::PlaylistManager(const String& root) :
    music_root(root), track_count(0),
    current_chunk_index(0), tracks_in_current_chunk(0) {
    if (!music_root.endsWith("/")) {
        music_root += "/";
    }
    path_buffer[0] = '\0';
}

String PlaylistManager::getChunkFilePath(int chunk_index) const {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%s/all.%04d", PLAYLIST_DIR, chunk_index);
    return String(buffer);
}

void PlaylistManager::deleteOldIndexFiles() {
    // Delete all chunk files
    int chunk = 0;
    while (true) {
        String path = getChunkFilePath(chunk);
        if (!SD.exists(path.c_str())) break;
        SD.remove(path.c_str());
        chunk++;
    }
}

bool PlaylistManager::scanForMP3Files() {
    track_count = 0;

    // Create playlist directory if it doesn't exist
    if (!SD.exists(PLAYLIST_DIR)) {
        SD.mkdir(PLAYLIST_DIR);
    }

    // Delete old index files
    deleteOldIndexFiles();

    File root = SD.open(music_root);
    if (!root || !root.isDirectory()) {
        Serial.println("Failed to open music directory");
        return false;
    }

    // Initialize first chunk file
    current_chunk_index = 0;
    tracks_in_current_chunk = 0;
    current_chunk_file = SD.open(getChunkFilePath(0).c_str(), FILE_WRITE);
    if (!current_chunk_file) {
        Serial.println("Failed to create chunk file");
        root.close();
        return false;
    }

    Serial.println("Scanning for MP3 files...");
    scanDirectory(root, 0);
    root.close();

    // Close last chunk file
    if (current_chunk_file) {
        current_chunk_file.close();
    }

    Serial.printf("Found %d MP3 files, %d chunk files created\n",
                  track_count, current_chunk_index + 1);
    return track_count > 0;
}

void PlaylistManager::scanDirectory(File dir, size_t base_len) {
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;

        const char* entry_name = entry.name();
        size_t name_len = strlen(entry_name);

        // Build full path in path_buffer (no stack allocation)
        size_t new_len;
        if (base_len == 0) {
            if (name_len < sizeof(path_buffer)) {
                strcpy(path_buffer, entry_name);
                new_len = name_len;
            } else {
                entry.close();
                continue;
            }
        } else {
            if (base_len + 1 + name_len < sizeof(path_buffer)) {
                path_buffer[base_len] = '/';
                strcpy(path_buffer + base_len + 1, entry_name);
                new_len = base_len + 1 + name_len;
            } else {
                entry.close();
                continue;
            }
        }

        if (entry.isDirectory()) {
            scanDirectory(entry, new_len);
        } else if (hasMP3Extension(entry_name)) {
            // Write track path to current chunk
            current_chunk_file.print(music_root);
            current_chunk_file.println(path_buffer);
            track_count++;
            tracks_in_current_chunk++;

            // Rotate to new chunk file if current one is full
            if (tracks_in_current_chunk >= PLAYLIST_CHUNK_SIZE) {
                current_chunk_file.close();
                current_chunk_index++;
                current_chunk_file = SD.open(getChunkFilePath(current_chunk_index).c_str(), FILE_WRITE);
                tracks_in_current_chunk = 0;
            }
        }
        entry.close();
    }
}

bool PlaylistManager::loadIndex() {
    track_count = 0;

    // Count tracks from all chunk files
    int chunk_index = 0;
    while (true) {
        String chunk_path = getChunkFilePath(chunk_index);
        if (!SD.exists(chunk_path.c_str())) break;

        // Count lines in this chunk
        File f = SD.open(chunk_path.c_str(), FILE_READ);
        if (f) {
            while (f.available()) {
                if (f.read() == '\n') track_count++;
            }
            f.close();
        }
        chunk_index++;
    }

    if (track_count == 0) {
        Serial.println("No chunk files found, scanning...");
        return scanForMP3Files();
    }

    Serial.printf("Index loaded: %d tracks from %d chunks\n", track_count, chunk_index);
    return true;
}

String PlaylistManager::getTrackPath(int index) const {
    if (!isValidIndex(index)) return "";

    int chunk = getChunkIndex(index);
    int local = getLocalIndex(index);

    File f = SD.open(getChunkFilePath(chunk).c_str(), FILE_READ);
    if (!f) return "";

    // Skip only 'local' lines (not 'index' lines!)
    int line = 0;
    while (line < local && f.available()) {
        if (f.read() == '\n') line++;
    }

    if (line != local) {
        f.close();
        return "";
    }

    String path = f.readStringUntil('\n');
    f.close();

    path.trim();
    return path;
}

String PlaylistManager::getTrackName(int index) const {
    if (!isValidIndex(index)) return "Invalid";

    int chunk = getChunkIndex(index);
    int local = getLocalIndex(index);

    File f = SD.open(getChunkFilePath(chunk).c_str(), FILE_READ);
    if (!f) return "Invalid";

    // Skip only 'local' lines
    int line = 0;
    while (line < local && f.available()) {
        if (f.read() == '\n') line++;
    }

    if (line != local) {
        f.close();
        return "Invalid";
    }

    // Read path into char buffer
    char buffer[256];
    size_t len = 0;
    while (f.available() && len < sizeof(buffer) - 1) {
        char c = f.read();
        if (c == '\n' || c == '\r') break;
        buffer[len++] = c;
    }
    buffer[len] = '\0';
    f.close();

    // Find last slash
    const char* name_start = strrchr(buffer, '/');
    if (name_start) {
        name_start++;
    } else {
        name_start = buffer;
    }

    // Find extension dot
    const char* dot = strrchr(name_start, '.');
    size_t name_len = dot ? (dot - name_start) : strlen(name_start);

    // Single String allocation
    return String(name_start).substring(0, name_len);
}

bool PlaylistManager::isValidIndex(int index) const {
    return index >= 0 && index < (int)track_count;
}

void PlaylistManager::getTrackNames(int start_index, int count, String* output) const {
    // Initialize output
    for (int i = 0; i < count; i++) {
        output[i] = "";
    }

    if (start_index < 0 || start_index >= (int)track_count || count <= 0) {
        return;
    }

    int current_chunk = getChunkIndex(start_index);
    int local_index = getLocalIndex(start_index);

    File f = SD.open(getChunkFilePath(current_chunk).c_str(), FILE_READ);
    if (!f) return;

    // Skip to local_index within the chunk
    int line = 0;
    while (line < local_index && f.available()) {
        if (f.read() == '\n') line++;
    }

    // Read count names
    char buffer[256];
    for (int i = 0; i < count; i++) {
        int track_idx = start_index + i;
        if (track_idx >= (int)track_count) break;

        // Check if we need to switch to next chunk
        int needed_chunk = getChunkIndex(track_idx);
        if (needed_chunk != current_chunk) {
            f.close();
            current_chunk = needed_chunk;
            f = SD.open(getChunkFilePath(current_chunk).c_str(), FILE_READ);
            if (!f) break;
            // At beginning of new chunk, no lines to skip
        }

        // Read line into buffer
        size_t len = 0;
        while (f.available() && len < sizeof(buffer) - 1) {
            char c = f.read();
            if (c == '\n' || c == '\r') {
                if (c == '\r' && f.available()) {
                    char next = f.peek();
                    if (next == '\n') f.read();
                }
                break;
            }
            buffer[len++] = c;
        }
        buffer[len] = '\0';

        // Extract filename (after last /)
        const char* name_start = strrchr(buffer, '/');
        if (name_start) {
            name_start++;
        } else {
            name_start = buffer;
        }

        // Remove extension
        const char* dot = strrchr(name_start, '.');
        size_t name_len = dot ? (dot - name_start) : strlen(name_start);

        output[i] = String(name_start).substring(0, name_len);
    }

    f.close();
}

bool PlaylistManager::hasMP3Extension(const char* filename) {
    size_t len = strlen(filename);
    if (len < 4) return false;

    const char* ext = filename + len - 4;
    return (ext[0] == '.') &&
           (ext[1] == 'm' || ext[1] == 'M') &&
           (ext[2] == 'p' || ext[2] == 'P') &&
           (ext[3] == '3');
}
