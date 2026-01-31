#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <Arduino.h>
#include <SD.h>
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"

class AudioProcessor {
private:
    File current_file;
    MP3DecoderHelix mp3;
    EncodedAudioStream decoder;

    // Duration/position tracking
    size_t file_size;           // Total file size in bytes
    size_t bytes_read;          // Bytes read so far
    int bitrate_kbps;           // Bitrate from first frame (kbps)
    bool duration_valid;        // True if bitrate has been detected

public:
    AudioProcessor();

    bool openFile(const String& filepath);
    void closeFile();

    int32_t readAudioData(uint8_t* buffer, int32_t len);

    // Duration and position accessors
    uint32_t getDurationSeconds() const;
    uint32_t getPositionSeconds() const;
    float getProgress() const;  // Returns 0.0 - 1.0
    bool hasDurationInfo() const { return duration_valid; }
};

#endif
