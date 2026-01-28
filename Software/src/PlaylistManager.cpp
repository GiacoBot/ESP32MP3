#include "PlaylistManager.h"

PlaylistManager::PlaylistManager(const String& root) :
    music_root(root), track_count(0) {
    if (!music_root.endsWith("/")) {
        music_root += "/";
    }
    path_buffer[0] = '\0';
}

bool PlaylistManager::scanForMP3Files() {
    track_count = 0;

    // Create playlist directory if it doesn't exist
    if (!SD.exists(PLAYLIST_DIR)) {
        SD.mkdir(PLAYLIST_DIR);
    }

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
    scanDirectory(root, indexFile, 0);
    root.close();
    indexFile.close();

    Serial.printf("Found %d MP3 files, index created\n", track_count);
    return track_count > 0;
}

void PlaylistManager::scanDirectory(File dir, File& indexFile, size_t base_len) {
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
            scanDirectory(entry, indexFile, new_len);
        } else if (hasMP3Extension(entry_name)) {
            indexFile.print(music_root);
            indexFile.println(path_buffer);
            track_count++;
        }
        entry.close();
    }
}

bool PlaylistManager::loadIndex() {
    track_count = 0;

    if (!SD.exists(PLAYLIST_INDEX_FILE)) {
        Serial.println("Index file not found, scanning...");
        return scanForMP3Files();
    }

    // Count lines in index file
    File indexFile = SD.open(PLAYLIST_INDEX_FILE, FILE_READ);
    if (!indexFile) {
        Serial.println("Failed to open index file");
        return false;
    }

    while (indexFile.available()) {
        char c = indexFile.read();
        if (c == '\n') {
            track_count++;
        }
    }
    indexFile.close();

    Serial.printf("Index loaded: %d tracks\n", track_count);
    return track_count > 0;
}

String PlaylistManager::getTrackPath(int index) const {
    if (!isValidIndex(index)) return "";

    File indexFile = SD.open(PLAYLIST_INDEX_FILE, FILE_READ);
    if (!indexFile) return "";

    // Skip 'index' lines
    int current_line = 0;
    while (current_line < index && indexFile.available()) {
        if (indexFile.read() == '\n') {
            current_line++;
        }
    }

    if (current_line != index) {
        indexFile.close();
        return "";
    }

    // Read current line
    String path = indexFile.readStringUntil('\n');
    indexFile.close();

    path.trim();
    return path;
}

String PlaylistManager::getTrackName(int index) const {
    if (!isValidIndex(index)) return "Invalid";

    File indexFile = SD.open(PLAYLIST_INDEX_FILE, FILE_READ);
    if (!indexFile) return "Invalid";

    // Skip 'index' lines
    int current_line = 0;
    while (current_line < index && indexFile.available()) {
        if (indexFile.read() == '\n') {
            current_line++;
        }
    }

    if (current_line != index) {
        indexFile.close();
        return "Invalid";
    }

    // Read path into char buffer
    char buffer[256];
    size_t len = 0;
    while (indexFile.available() && len < sizeof(buffer) - 1) {
        char c = indexFile.read();
        if (c == '\n' || c == '\r') break;
        buffer[len++] = c;
    }
    buffer[len] = '\0';
    indexFile.close();

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
    // Inizializza output
    for (int i = 0; i < count; i++) {
        output[i] = "";
    }

    if (start_index < 0 || start_index >= (int)track_count || count <= 0) {
        return;
    }

    File indexFile = SD.open(PLAYLIST_INDEX_FILE, FILE_READ);
    if (!indexFile) return;

    // Salta a start_index (una sola volta)
    int current_line = 0;
    while (current_line < start_index && indexFile.available()) {
        if (indexFile.read() == '\n') {
            current_line++;
        }
    }

    if (current_line != start_index) {
        indexFile.close();
        return;
    }

    // Legge count nomi consecutivi
    char buffer[256];
    for (int i = 0; i < count && indexFile.available(); i++) {
        int track_index = start_index + i;
        if (track_index >= (int)track_count) break;

        // Legge linea nel buffer
        size_t len = 0;
        while (indexFile.available() && len < sizeof(buffer) - 1) {
            char c = indexFile.read();
            if (c == '\n' || c == '\r') {
                if (c == '\r' && indexFile.available()) {
                    char next = indexFile.peek();
                    if (next == '\n') indexFile.read();
                }
                break;
            }
            buffer[len++] = c;
        }
        buffer[len] = '\0';

        // Estrae nome file (dopo ultimo /)
        const char* name_start = strrchr(buffer, '/');
        if (name_start) {
            name_start++;
        } else {
            name_start = buffer;
        }

        // Rimuove estensione
        const char* dot = strrchr(name_start, '.');
        size_t name_len = dot ? (dot - name_start) : strlen(name_start);

        output[i] = String(name_start).substring(0, name_len);
    }

    indexFile.close();
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
