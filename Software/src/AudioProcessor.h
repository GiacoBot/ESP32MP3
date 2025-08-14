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
    EncodedAudioStream* decoder;
    bool is_initialized;
    
public:
    AudioProcessor();
    ~AudioProcessor();
    
    bool openFile(const String& filepath);
    void closeFile();
    bool isFileOpen() const;
    bool hasDataAvailable() const;
    
    int32_t readAudioData(uint8_t* buffer, int32_t len);
    
    // For debugging
    size_t getFileSize() const;
    size_t getCurrentPosition() const;
    
private:
    bool initializeDecoder();
    void cleanup();
};

#endif