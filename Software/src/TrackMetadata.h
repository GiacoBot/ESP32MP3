#ifndef TRACK_METADATA_H
#define TRACK_METADATA_H

#include <Arduino.h>

struct TrackMetadata {
    char title[64];
    char artist[64];
    char album[64];
    bool has_metadata;

    void clear() {
        title[0] = '\0';
        artist[0] = '\0';
        album[0] = '\0';
        has_metadata = false;
    }

    // Helper to set title from String (with bounds check)
    void setTitle(const char* str) {
        strncpy(title, str, sizeof(title) - 1);
        title[sizeof(title) - 1] = '\0';
    }

    // Helper to set artist from String (with bounds check)
    void setArtist(const char* str) {
        strncpy(artist, str, sizeof(artist) - 1);
        artist[sizeof(artist) - 1] = '\0';
    }

    // Helper to set album from String (with bounds check)
    void setAlbum(const char* str) {
        strncpy(album, str, sizeof(album) - 1);
        album[sizeof(album) - 1] = '\0';
    }
};

#endif // TRACK_METADATA_H
