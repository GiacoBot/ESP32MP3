#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <Arduino.h>
#include <SD.h>

#define PLAYLIST_DIR "/.playlist"
#define PLAYLIST_CHUNK_SIZE 10  // Tracks per chunk file

class PlaylistManager {
private:
    String music_root;
    size_t track_count;
    char path_buffer[256];  // Static buffer to avoid heap allocations

    // Used during scanForMP3Files()
    File current_chunk_file;
    int current_chunk_index;
    int tracks_in_current_chunk;

public:
    PlaylistManager(const String& root = "/");

    bool scanForMP3Files();      // Scan and create index files
    bool loadIndex();            // Load track count from existing files

    size_t getTrackCount() const { return track_count; }
    String getTrackPath(int index) const;   // Read from file on-demand
    String getTrackName(int index) const;
    void getTrackNames(int start_index, int count, String* output) const;  // Batch read
    bool isValidIndex(int index) const;

private:
    bool hasMP3Extension(const char* filename);
    void scanDirectory(File dir, size_t base_len);

    // Chunk file helpers
    String getChunkFilePath(int chunk_index) const;
    int getChunkIndex(int track_index) const { return track_index / PLAYLIST_CHUNK_SIZE; }
    int getLocalIndex(int track_index) const { return track_index % PLAYLIST_CHUNK_SIZE; }
    void deleteOldIndexFiles();
};

#endif
