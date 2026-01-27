#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <Arduino.h>
#include <SD.h>
#include <vector>

#define PLAYLIST_INDEX_FILE "/playlist.idx"

class PlaylistManager {
private:
    String music_root;
    size_t track_count;
    std::vector<uint32_t> file_offsets;  // Only offsets, not strings!

public:
    PlaylistManager(const String& root = "/");

    bool scanForMP3Files();      // Scan and create index file
    bool loadIndex();            // Load offsets from existing file

    size_t getTrackCount() const { return track_count; }
    String getTrackPath(int index) const;   // Read from file on-demand
    String getTrackName(int index) const;
    bool isValidIndex(int index) const;

private:
    bool hasMP3Extension(const String& filename);
    void scanDirectory(File dir, File& indexFile, const String& path);
    void buildOffsetTable();
};

#endif
