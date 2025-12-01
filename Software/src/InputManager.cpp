#include "InputManager.h"

// --- Pin Definitions ---
const int BTN_UP = 13;
const int BTN_DOWN = 17;
const int BTN_LEFT = 4;
const int BTN_RIGHT = 15;
const int BTN_ENTER = 2;

// --- Static member initialization ---
volatile bool InputManager::btn_up_pressed = false;
volatile bool InputManager::btn_down_pressed = false;
volatile bool InputManager::btn_left_pressed = false;
volatile bool InputManager::btn_right_pressed = false;
volatile bool InputManager::btn_enter_pressed = false;

InputManager::InputManager() : music_player(nullptr), bluetooth_manager(nullptr) {}

void InputManager::setMusicPlayer(MusicPlayer* player) {
    music_player = player;
}

void InputManager::setBluetoothManager(BluetoothManager* btManager) {
    bluetooth_manager = btManager;
}

void InputManager::initialize() {
    Serial.println("Setting up button interrupts...");
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_ENTER, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(BTN_UP), isr_btn_up, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_DOWN), isr_btn_down, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_LEFT), isr_btn_left, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_RIGHT), isr_btn_right, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_ENTER), isr_btn_enter, FALLING);
}

// --- ISRs (Interrupt Service Routines) ---
void IRAM_ATTR InputManager::isr_btn_up() {
    btn_up_pressed = true;
}

void IRAM_ATTR InputManager::isr_btn_down() {
    btn_down_pressed = true;
}

void IRAM_ATTR InputManager::isr_btn_left() {
    btn_left_pressed = true;
}

void IRAM_ATTR InputManager::isr_btn_right() {
    btn_right_pressed = true;
}

void IRAM_ATTR InputManager::isr_btn_enter() {
    btn_enter_pressed = true;
}

void InputManager::handleInputs() {
    static unsigned long last_press_time = 0;
    const unsigned long debounce_delay = 300; // 300ms debounce delay

    // Use a local copy of flags to avoid race conditions with ISRs
    bool up = btn_up_pressed;
    bool down = btn_down_pressed;
    bool left = btn_left_pressed;
    bool right = btn_right_pressed;
    bool enter = btn_enter_pressed;
    
    if (up || down || left || right || enter) {
        unsigned long current_time = millis();
        if (current_time - last_press_time > debounce_delay) {
            last_press_time = current_time;
            
            btn_up_pressed = false;
            btn_down_pressed = false;
            btn_left_pressed = false;
            btn_right_pressed = false;
            btn_enter_pressed = false;

            if (up) {
                Serial.println("Button UP: Connecting...");
                if (bluetooth_manager) bluetooth_manager->connect();
            } else if (down) {
                Serial.println("Button DOWN: Disconnecting...");
                if (bluetooth_manager) bluetooth_manager->disconnect();
            } else if (left) {
                Serial.println("Button LEFT: Previous track...");
                if (music_player) music_player->executeCommand(PlayerCommand::PREV_TRACK);
            } else if (right) {
                Serial.println("Button RIGHT: Next track...");
                if (music_player) music_player->executeCommand(PlayerCommand::NEXT_TRACK);
            } else if (enter) {
                Serial.println("Button ENTER: Play/Pause...");
                if (music_player && music_player->getState() == PlayerState::PLAYING) {
                    music_player->executeCommand(PlayerCommand::PAUSE);
                } else if (music_player) {
                    music_player->executeCommand(PlayerCommand::PLAY);
                }
            }
        } else {
            // It's a bounce, clear the flags
            btn_up_pressed = false;
            btn_down_pressed = false;
            btn_left_pressed = false;
            btn_right_pressed = false;
            btn_enter_pressed = false;
        }
    }
}
