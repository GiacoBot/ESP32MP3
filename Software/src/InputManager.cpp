#include "InputManager.h"

InputManager::InputManager() {
    // Initialize button states
    for (int i = 0; i < NUM_BUTTONS; i++) {
        last_button_state[i] = HIGH; // Buttons are pulled up, so HIGH is the idle state
        button_state[i] = HIGH;
        last_debounce_time[i] = 0;
    }
}

void InputManager::initialize() {
    Serial.println("Setting up button pins for polling...");
    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(button_pins[i], INPUT_PULLUP);
    }
}

InputEvent InputManager::handleInputs() {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        // Read the state of the button
        int reading = digitalRead(button_pins[i]);

        // Check if the state has changed (e.g., due to a press or noise)
        if (reading != last_button_state[i]) {
            // Reset the debounce timer
            last_debounce_time[i] = millis();
        }

        // Check if the state has been stable for longer than the debounce delay
        if ((millis() - last_debounce_time[i]) > DEBOUNCE_DELAY) {
            // If the stable state is different from the last registered state, it's a valid change
            if (reading != button_state[i]) {
                button_state[i] = reading;

                // If the new stable state is LOW, it means the button was just pressed
                if (button_state[i] == LOW) {
                    Serial.printf("Input Event: %d\n", i);
                    return button_events[i];
                }
            }
        }
        
        last_button_state[i] = reading;
    }
    
    // If no button press was detected, return NONE
    return InputEvent::INPUT_EVENT_NONE;
}
