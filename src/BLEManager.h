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
#define BLE_MAX_RAW_DATA_LEN 20

/**
 * @brief Initialize BLE stack and start advertising as "ARHUD"
 */
void BLE_Manager_Init(void);

/**
 * @brief Stop BLE advertising and release resources if necessary
 */
void BLE_Manager_Deinit(void);

/**
 * @brief Get pointer to the raw BLE data buffer received from the Android App
 * @param len Pointer to store the length of received data in bytes
 * @return Const pointer to the raw data buffer
 */
const uint8_t *BLE_Manager_GetRawData(uint16_t *len);

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
 * @brief FreeRTOS task sending 8-byte payload to BLE master once per second (1 Hz)
 * @param pvParameters FreeRTOS task parameters (unused)
 */
void BLE_Send_Task(void *pvParameters);

#endif // BLE_MANAGER_H
