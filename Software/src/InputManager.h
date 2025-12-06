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
    const int button_pins[NUM_BUTTONS] = {BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_ENTER};
    
    // Event mapping for each button state
    const InputEvent short_press_events[NUM_BUTTONS] = {
        InputEvent::INPUT_EVENT_UP,
        InputEvent::INPUT_EVENT_DOWN,
        InputEvent::INPUT_EVENT_LEFT,
        InputEvent::INPUT_EVENT_RIGHT,
        InputEvent::INPUT_EVENT_ENTER
    };
    const InputEvent long_press_events[NUM_BUTTONS] = {
        InputEvent::INPUT_EVENT_UP_LONG_PRESS,
        InputEvent::INPUT_EVENT_DOWN_LONG_PRESS,
        InputEvent::INPUT_EVENT_LEFT_LONG_PRESS,
        InputEvent::INPUT_EVENT_RIGHT_LONG_PRESS,
        InputEvent::INPUT_EVENT_ENTER_LONG_PRESS
    };
    const InputEvent repeat_events[NUM_BUTTONS] = {
        InputEvent::INPUT_EVENT_UP_REPEAT,
        InputEvent::INPUT_EVENT_DOWN_REPEAT,
        InputEvent::INPUT_EVENT_LEFT_REPEAT,
        InputEvent::INPUT_EVENT_RIGHT_REPEAT,
        InputEvent::INPUT_EVENT_ENTER_REPEAT
    };

    // State tracking for debounce and press types
    byte last_button_state[NUM_BUTTONS];
    byte button_state[NUM_BUTTONS];
    unsigned long last_debounce_time[NUM_BUTTONS];
    unsigned long press_start_time[NUM_BUTTONS];
    unsigned long last_repeat_time[NUM_BUTTONS];
    bool is_long_press_registered[NUM_BUTTONS];
};

#endif // INPUT_MANAGER_H
