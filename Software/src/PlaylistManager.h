#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <Arduino.h>
#include <SD.h>

#define PLAYLIST_DIR "/.playlist"
#define PLAYLIST_INDEX_FILE "/.playlist/all.idx"

class PlaylistManager {
private:
    String music_root;
    size_t track_count;
    char path_buffer[256];  // Static buffer to avoid heap allocations

public:
    PlaylistManager(const String& root = "/");

    bool scanForMP3Files();      // Scan and create index file
    bool loadIndex();            // Load track count from existing file

    size_t getTrackCount() const { return track_count; }
    String getTrackPath(int index) const;   // Read from file on-demand
    String getTrackName(int index) const;
    void getTrackNames(int start_index, int count, String* output) const;  // Batch read
    bool isValidIndex(int index) const;

private:
    bool hasMP3Extension(const char* filename);
    void scanDirectory(File dir, File& indexFile, size_t base_len);
};

#endif
