#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>
#include "InputEvents.h"
#include "settings.h" // For button pin definitions

#define NUM_BUTTONS 5

class InputManager {
public:
    InputManager();
    void initialize();
    InputEvent handleInputs();

private:
    // --- Polling and Debounce State ---
    const int button_pins[NUM_BUTTONS] = {BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_ENTER};
    const InputEvent button_events[NUM_BUTTONS] = {
        InputEvent::INPUT_EVENT_UP,
        InputEvent::INPUT_EVENT_DOWN,
        InputEvent::INPUT_EVENT_LEFT,
        InputEvent::INPUT_EVENT_RIGHT,
        InputEvent::INPUT_EVENT_ENTER
    };

    byte last_button_state[NUM_BUTTONS];
    byte button_state[NUM_BUTTONS]; // Add this line
    unsigned long last_debounce_time[NUM_BUTTONS];
};

#endif // INPUT_MANAGER_H
