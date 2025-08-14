#include "AudioProcessor.h"

AudioProcessor::AudioProcessor() :
    decoder(nullptr),
    is_initialized(false) {
}

AudioProcessor::~AudioProcessor() {
    cleanup();
}

bool AudioProcessor::openFile(const String& filepath) {
    // Close previous file if open
    closeFile();
    
    // Open new file
    current_file = SD.open(filepath);
    if (!current_file) {
        Serial.println("Failed to open file: " + filepath);
        return false;
    }
    
    if (current_file.size() == 0) {
        Serial.println("File is empty: " + filepath);
        current_file.close();
        return false;
    }
    
    Serial.printf("Opened file: %s (size: %d bytes)\n", 
                 filepath.c_str(), 
                 current_file.size());
    
    // Initialize the decoder
    if (!initializeDecoder()) {
        Serial.println("Failed to initialize audio decoder");
        current_file.close();
        return false;
    }
    
    is_initialized = true;
    return true;
}

void AudioProcessor::closeFile() {
    cleanup();
}

bool AudioProcessor::isFileOpen() const {
    return current_file && is_initialized;
}

bool AudioProcessor::hasDataAvailable() const {
    // Cast away const to call available(), which is not const
    File& non_const_file = const_cast<File&>(current_file);
    return current_file && non_const_file.available() > 0;
}

int32_t AudioProcessor::readAudioData(uint8_t* buffer, int32_t len) {
    if (!isFileOpen()) {
        // No file open - return silence
        memset(buffer, 0, len);
        return len;
    }
    
    if (!hasDataAvailable()) {
        // File finished - return 0 to signal end of track
        return 0;
    }
    
    // Read data from the decoder
    int32_t bytes_read = decoder->readBytes(buffer, len);
    
    if (bytes_read <= 0) {
        // Error or end of file
        if (!hasDataAvailable()) {
            // End of file
            return 0;
        } else {
            // Decoding error - return silence
            Serial.println("Audio decoding error, returning silence");
            memset(buffer, 0, len);
            return len;
        }
    }
    
    // If we've read less data than required, the rest has already been
    // handled by the decoder (likely filled with silence)
    return bytes_read;
}

size_t AudioProcessor::getFileSize() const {
    if (current_file) {
        return current_file.size();
    }
    return 0;
}

size_t AudioProcessor::getCurrentPosition() const {
    if (current_file) {
        return current_file.position();
    }
    return 0;
}

bool AudioProcessor::initializeDecoder() {
    if (!current_file) {
        return false;
    }
    
    try {
        // Create new decoder stream
        decoder = new EncodedAudioStream(&current_file, &mp3);
        
        // Configure output buffer (optimized for Bluetooth streaming)
        // Increase buffer size to reduce underrun
        decoder->transformationReader().resizeResultQueue(1024 * 8);
        
        // Initialize the decoder
        if (!decoder->begin()) {
            Serial.println("Decoder begin() failed");
            delete decoder;
            decoder = nullptr;
            return false;
        }
        
        Serial.println("Audio decoder initialized successfully");
        return true;
        
    } catch (...) {
        Serial.println("Exception during decoder initialization");
        if (decoder) {
            delete decoder;
            decoder = nullptr;
        }
        return false;
    }
}

void AudioProcessor::cleanup() {
    is_initialized = false;
    
    if (decoder) {
        try {
            decoder->end();
            delete decoder;
        } catch (...) {
            Serial.println("Exception during decoder cleanup");
        }
        decoder = nullptr;
    }
    
    if (current_file) {
        current_file.close();
    }
}