#include "AudioProcessor.h"

AudioProcessor::AudioProcessor() :
    decoder(&current_file, &mp3),
    file_size(0),
    bytes_read(0),
    bitrate_kbps(0),
    duration_valid(false) {
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

    // Store file size and reset position tracking
    file_size = current_file.size();
    bytes_read = 0;
    bitrate_kbps = 0;
    duration_valid = false;

    decoder.transformationReader().resizeResultQueue(1024 * 8);
    if (!decoder.begin()) {
        Serial.println("Decoder begin() failed");
        current_file.close();
        return false;
    }

    Serial.printf("Opened file: %s (%u bytes)\n", filepath.c_str(), file_size);
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

    // Track file position before read
    size_t pos_before = current_file.position();

    int32_t decoded_bytes = decoder.readBytes(buffer, len);

    // Update bytes_read based on file position change
    size_t pos_after = current_file.position();
    if (pos_after > pos_before) {
        bytes_read += (pos_after - pos_before);
    }

    // Try to detect bitrate from decoder after first successful decode
    if (!duration_valid && decoded_bytes > 0) {
        // Get MP3 frame info which contains actual bitrate
        MP3FrameInfo frame_info = mp3.audioInfoEx();
        if (frame_info.bitrate > 0) {
            // bitrate is in bits/second, convert to kbps
            bitrate_kbps = frame_info.bitrate / 1000;
            duration_valid = true;
            Serial.printf("Detected bitrate: %d kbps\n", bitrate_kbps);
        } else if (file_size > 0) {
            // Fallback to 192kbps if frame info unavailable
            bitrate_kbps = 192;
            duration_valid = true;
        }
    }

    // If we didn't get the full buffer, fill the rest with silence
    if (decoded_bytes < len) {
        memset(buffer + decoded_bytes, 0, len - decoded_bytes);
    }

    return len; // Always return the requested length for A2DP
}

uint32_t AudioProcessor::getDurationSeconds() const {
    if (!duration_valid || bitrate_kbps == 0) {
        return 0;
    }
    // Duration = file_size (bytes) * 8 (bits) / bitrate (kbps * 1000)
    return (uint32_t)((file_size * 8ULL) / (bitrate_kbps * 1000ULL));
}

uint32_t AudioProcessor::getPositionSeconds() const {
    if (!duration_valid || bitrate_kbps == 0) {
        return 0;
    }
    // Position = bytes_read * 8 / (bitrate_kbps * 1000)
    return (uint32_t)((bytes_read * 8ULL) / (bitrate_kbps * 1000ULL));
}

float AudioProcessor::getProgress() const {
    if (file_size == 0) {
        return 0.0f;
    }
    return (float)bytes_read / (float)file_size;
}