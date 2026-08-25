#ifndef _COMM_H
#define _COMM_H

#include "esp_log.h"

#include "BLEManager.h"

#define BLE_IN_BUF_ROWS		3
#define BLE_IN_BUF_COLS		(BLE_MAX_RAW_DATA_LEN + 1)


void comm_init(void);

void write_ble_in_buf(uint8_t *data, uint8_t len);
uint8_t read_ble_in_buf(uint8_t *data);

#endif



