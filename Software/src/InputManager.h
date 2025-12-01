#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>
#include "MusicPlayer.h"
#include "BluetoothManager.h"

class InputManager {
public:
    InputManager();
    void initialize();
    void handleInputs();

    void setMusicPlayer(MusicPlayer* player);
    void setBluetoothManager(BluetoothManager* btManager);

private:
    MusicPlayer* music_player;
    BluetoothManager* bluetooth_manager;

    // --- Interrupt Flags ---
    static volatile bool btn_up_pressed;
    static volatile bool btn_down_pressed;
    static volatile bool btn_left_pressed;
    static volatile bool btn_right_pressed;
    static volatile bool btn_enter_pressed;
    
    // --- ISRs ---
    static void IRAM_ATTR isr_btn_up();
    static void IRAM_ATTR isr_btn_down();
    static void IRAM_ATTR isr_btn_left();
    static void IRAM_ATTR isr_btn_right();
    static void IRAM_ATTR isr_btn_enter();
};

#endif // INPUT_MANAGER_H
