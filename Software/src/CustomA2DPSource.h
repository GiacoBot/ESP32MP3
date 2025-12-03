#ifndef CUSTOM_A2DP_SOURCE_H
#define CUSTOM_A2DP_SOURCE_H

#include "BluetoothA2DPSource.h"

/**
 * @brief This class extends the BluetoothA2DPSource to expose protected methods
 * that are needed for a more advanced, stateful UI. Specifically, it allows us
 * to connect to a device using its Bluetooth address, which is not possible
 * with the base class's public API.
 */
class CustomA2DPSource : public BluetoothA2DPSource {
public:
    // Expose the protected esp_a2d_connect method to the public API
    esp_err_t connect(esp_bd_addr_t address) {
        return esp_a2d_connect(address);
    }
};

#endif // CUSTOM_A2DP_SOURCE_H
