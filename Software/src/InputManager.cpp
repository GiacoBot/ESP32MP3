#include "InputManager.h"

InputManager::InputManager() {
    // Initialize all state-tracking arrays
    for (int i = 0; i < NUM_BUTTONS; i++) {
        last_button_state[i] = HIGH; // Buttons are pulled up, so HIGH is the idle state
        button_state[i] = HIGH;
        last_debounce_time[i] = 0;
        press_start_time[i] = 0;
        last_repeat_time[i] = 0;
        is_long_press_registered[i] = false;
    }
}

void InputManager::initialize() {
    Serial.println("Setting up button pins for polling...");
    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(button_pins[i], INPUT_PULLUP);
    }
}

InputEvent InputManager::handleInputs() {
    unsigned long current_millis = millis();

    for (int i = 0; i < NUM_BUTTONS; i++) {
        // --- Debouncing Logic ---
        int reading = digitalRead(button_pins[i]);
        if (reading != last_button_state[i]) {
            last_debounce_time[i] = current_millis;
        }
        last_button_state[i] = reading;

        if ((current_millis - last_debounce_time[i]) > DEBOUNCE_DELAY) {
            // The reading has been stable, proceed if it's different from the current state
            if (reading != button_state[i]) {
                button_state[i] = reading;

                // --- Event Logic ---
                if (button_state[i] == LOW) {
                    // --- BUTTON PRESSED ---
                    press_start_time[i] = current_millis;
                    is_long_press_registered[i] = false;
                } else {
                    // --- BUTTON RELEASED ---
                    if (!is_long_press_registered[i]) {
                        // If a long press was never registered, it's a short press
                        return short_press_events[i];
                    }
                }
            }
        }

        // --- Long Press and Repeat Logic (while button is held down) ---
        if (button_state[i] == LOW) {
            // Check for initial long press
            if (!is_long_press_registered[i] && (current_millis - press_start_time[i]) > LONG_PRESS_DURATION) {
                is_long_press_registered[i] = true;
                last_repeat_time[i] = current_millis;
                return long_press_events[i];
            }
            // Check for subsequent repeat events
            else if (is_long_press_registered[i] && (current_millis - last_repeat_time[i]) > LONG_PRESS_REPEAT_DELAY) {
                last_repeat_time[i] = current_millis;
                return repeat_events[i];
            }
        }
    }
    
    // If no event was generated, return NONE
    return InputEvent::INPUT_EVENT_NONE;
}
