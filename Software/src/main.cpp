#include <Arduino.h>
#include <BluetoothA2DPSource.h>

// Create a BluetoothA2DPSource object
BluetoothA2DPSource a2dp_source;

// --- Configuration ---
// Change this to the name of your Bluetooth headphones/speaker
const char* target_device_name = "Lenovo LP40"; 
// You can also use a MAC address if you prefer:
// const char* target_device_mac = "XX:XX:XX:XX:XX:XX"; 

// --- Audio Generation Parameters ---
const int sample_rate = 44100; // Hz
const int bits_per_sample = 16; // bits
const int channels = 2; // Stereo
const float frequency = 1000.0; // Hz (A4 note)
float phase = 0.0;

// --- Callbacks ---

// Callback for generating audio data (sine wave)
int32_t get_audio_data(uint8_t *data, int32_t len) {
  int16_t *samples = (int16_t *)data;
  int num_samples = len / (bits_per_sample / 8) / channels;

  for (int i = 0; i < num_samples; i++) {
    float sample_value = sin(phase) * 32767; // Max 16-bit signed value
    samples[i * 2] = (int16_t)sample_value;     // Left channel
    samples[i * 2 + 1] = (int16_t)sample_value; // Right channel

    phase += 2 * PI * frequency / sample_rate;
    if (phase >= 2 * PI) {
      phase -= 2 * PI;
    }
  }
  return len;
}

// Callback for A2DP connection state changes
void avrc_connection_state_changed(esp_a2d_connection_state_t state, void *ptr) {
  Serial.print("A2DP connection state: ");
  if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
    Serial.println("DISCONNECTED");
  } else if (state == ESP_A2D_CONNECTION_STATE_CONNECTING) {
    Serial.println("CONNECTING");
  } else if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
    Serial.println("CONNECTED");
  } else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTING) {
    Serial.println("DISCONNECTING");
  }
}

// Callback for AVRC passthrough commands received from the connected device
void avrc_passthru_command_received(uint8_t key, bool isReleased) {
  if (isReleased) { // Only process on release to avoid double-counting
    Serial.print("AVRC Passthrough Command Received: ");
    switch (key) {
      case ESP_AVRC_PT_CMD_PLAY: Serial.println("PLAY"); break;
      case ESP_AVRC_PT_CMD_PAUSE: Serial.println("PAUSE"); break;
      case ESP_AVRC_PT_CMD_STOP: Serial.println("STOP"); break;
      case ESP_AVRC_PT_CMD_FORWARD: Serial.println("NEXT"); break;
      case ESP_AVRC_PT_CMD_BACKWARD: Serial.println("PREVIOUS"); break;
      case ESP_AVRC_PT_CMD_VOL_UP: Serial.println("VOLUME UP"); break;
      case ESP_AVRC_PT_CMD_VOL_DOWN: Serial.println("VOLUME DOWN"); break;
      default: Serial.printf("Unknown Key: 0x%02X\n", key); break;
    }
  }
}

// Callback for AVRC (remote control) events - kept for library internal use, but not explicitly used for logging here
void av_hdl_avrc_evt(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param) {
  // This function is primarily for internal library use or if you want to log more detailed CT events.
  // For passthrough commands, avrc_passthru_command_received is used.
  // Serial.printf("AVRC CT event: %d\n", event);
}

void send_avrc_command(uint8_t command) {
  esp_avrc_ct_send_passthrough_cmd(0, command, ESP_AVRC_PT_CMD_STATE_PRESSED);
  esp_avrc_ct_send_passthrough_cmd(0, command, ESP_AVRC_PT_CMD_STATE_RELEASED);
}

// --- Setup and Loop ---

void print_help() {
  Serial.println("\n--- Bluetooth A2DP Source Test ---");
  Serial.println("Commands:");
  Serial.println("  c - Connect to target device");
  Serial.println("  d - Disconnect");
  Serial.println("  + - Volume up");
  Serial.println("  - - Volume down");
  Serial.println("  h - Show this help");
  Serial.println("-----------------------------------");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) ; // Wait for serial port to connect. Needed for native USB
  Serial.println("Starting ESP32 A2DP Source...");

  // Set the device name for the ESP32
  a2dp_source.set_local_name("ESP32_MP3_Player");

  // Set the audio data callback
  a2dp_source.set_data_callback(get_audio_data);

  // Set the connection state callback
  a2dp_source.set_on_connection_state_changed(avrc_connection_state_changed);

  // Set the AVRC passthrough command callback (to receive commands from headphones)
  a2dp_source.set_avrc_passthru_command_callback(avrc_passthru_command_received);

  // Set the AVRC notification events we are interested in (re-added for stability)
  a2dp_source.set_avrc_rn_events({
    ESP_AVRC_RN_PLAY_STATUS_CHANGE,
    ESP_AVRC_RN_VOLUME_CHANGE,
    ESP_AVRC_RN_TRACK_CHANGE,
    ESP_AVRC_RN_BATTERY_STATUS_CHANGE,
    ESP_AVRC_RN_SYSTEM_STATUS_CHANGE,
    ESP_AVRC_RN_APP_SETTING_CHANGE,
    ESP_AVRC_RN_NOW_PLAYING_CHANGE,
    ESP_AVRC_RN_UIDS_CHANGE
  });

  // Start the A2DP source (initializes Bluetooth and makes it discoverable)
  a2dp_source.start();

  print_help();
}

void loop() {
  if (Serial.available()) {
    char command = Serial.read();
    switch (command) {
      case 'c':
        Serial.print("Attempting to connect to: ");
        Serial.println(target_device_name);
        // The start() method with a name attempts to connect
        a2dp_source.start(target_device_name);
        break;
      case 'd':
        Serial.println("Disconnecting...");
        a2dp_source.end(false); // false = non chiamare esp_restart
        esp_bluedroid_disable();
        esp_bluedroid_deinit();
        esp_bt_controller_disable();
        break;
      case '+': // Volume Up
        Serial.println("Sending VOLUME UP...");
        send_avrc_command(ESP_AVRC_PT_CMD_VOL_UP);
        break;
      case '-': // Volume Down
        Serial.println("Sending VOLUME DOWN...");
        send_avrc_command(ESP_AVRC_PT_CMD_VOL_DOWN);
        break;
      case 'h':
        print_help();
        break;
      default:
        Serial.println("Unknown command. Press 'h' for help.");
        break;
    }
  }
}
