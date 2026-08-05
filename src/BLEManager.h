#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Service and Characteristic UUIDs matching the Android App
// SERVICE_UUID: 0000ffe0-0000-1000-8000-00805f9b34fb
// CHARACTERISTIC_UUID: 0000ffe1-0000-1000-8000-00805f9b34fb

#define BLE_SERVICE_UUID 0xFFE0
#define BLE_CHARACTERISTIC_UUID 0xFFE1

typedef struct
{
    uint8_t turn_direction;
    uint8_t lane_index;
    uint8_t total_lanes;
    uint16_t distance_to_turn;
} nav_data_t;

/**
 * @brief Callback function type for received navigation data
 */
typedef void (*ble_nav_data_callback_t)(const nav_data_t *nav_data);

/**
 * @brief Initialize BLE stack and start advertising as "ESP32_NAV"
 * @param callback Callback to execute when valid navigation data is received
 */
void BLE_Manager_Init(ble_nav_data_callback_t callback);

/**
 * @brief Stop BLE advertising and release resources if necessary
 */
void BLE_Manager_Deinit(void);

/**
 * @brief Send data to the connected BLE master device via GATT notification
 * @param data Pointer to the buffer containing data to send
 * @param len Length of the data buffer in bytes
 * @return ESP_OK on success, ESP_FAIL or error code otherwise
 */
esp_err_t BLE_Manager_SendData(const uint8_t *data, uint16_t len);

/**
 * @brief Check if a BLE master device is currently connected
 * @return true if connected, false otherwise
 */
bool BLE_Manager_IsConnected(void);

/**
 * @brief FreeRTOS task sending 8-byte payload (Start frame 0xAA, 10-bit pitch, 10-bit roll, 10-bit yaw, reserved padding, End frame 0x55) to BLE master once per second (1 Hz)
 * @param pvParameters FreeRTOS task parameters (unused)
 */
void BLE_Send_Task(void *pvParameters);

#endif // BLE_MANAGER_H

