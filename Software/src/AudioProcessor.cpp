#include "AudioProcessor.h"

// 1. Initialize 'volume' passing 'decoder' as the source
AudioProcessor::AudioProcessor() : decoder(&current_file, &mp3), volume(decoder) {
    // Initialize volume with default config (usually 16-bit, 2 channels)
    auto cfg = volume.defaultConfig();
    volume.begin(cfg);
    
    volume.setVolume(0.5f);
}

void AudioProcessor::setVolume(float f) {
    // 2. Implement the setter (0.0 to 1.0)
    volume.setVolume(f);
}

bool AudioProcessor::openFile(const String& filepath) {
    if (current_file) {
        current_file.close();
    }
    
    current_file = SD.open(filepath);
    if (!current_file) {
        Serial.println("Failed to open file: " + filepath);
        return false;
    }
    
    decoder.transformationReader().resizeResultQueue(1024 * 8);
    
    if (!decoder.begin()) {
        Serial.println("Decoder begin() failed");
        current_file.close();
        return false;
    }
    
    Serial.printf("Opened file: %s\n", filepath.c_str());
    return true;
}

void AudioProcessor::closeFile() {
    if (current_file) {
        current_file.close();
    }
    decoder.end();
}

int32_t AudioProcessor::readAudioData(uint8_t* buffer, int32_t len) {
    if (!current_file || !current_file.available()) {
        return 0; // Signal end of track
    }
    
    // 3. CHANGE: Read from 'volume' instead of 'decoder'
    // The volume stream pulls from decoder, applies gain, and returns data.
    int32_t bytes_read = volume.readBytes(buffer, len);
    
    // If we didn't get the full buffer, fill the rest with silence
    if (bytes_read < len) {
        memset(buffer + bytes_read, 0, len - bytes_read);
    }
    
    return len; // Always return the requested length for A2DP
}