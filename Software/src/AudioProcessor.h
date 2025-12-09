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
    VolumeStream volume; // <--- 1. Add VolumeStream object
    
public:
    AudioProcessor();
    
    bool openFile(const String& filepath);
    void closeFile();
    
    void setVolume(float f); // <--- 2. Add setter
    
    int32_t readAudioData(uint8_t* buffer, int32_t len);
};

#endif