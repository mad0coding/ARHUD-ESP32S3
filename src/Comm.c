#include "Comm.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "Comm";

static uint8_t BLE_IN_BUF[BLE_IN_BUF_ROWS][BLE_IN_BUF_COLS] = {0};
static uint8_t BLE_IN_BUF_start = 0, BLE_IN_BUF_used = 0;

/* Mutex protecting the ring buffer: the writer (BLE RX callback) and the
* reader (data consumer task) may run on different tasks/cores. */
static SemaphoreHandle_t s_ble_in_buf_mutex = NULL;

/* Create the ring buffer mutex. Call once during startup before any
* read/write operation. This function is assumed to be non-reentrant. */
void comm_init(void)
{
	s_ble_in_buf_mutex = xSemaphoreCreateMutex();
	if(s_ble_in_buf_mutex == NULL){
		ESP_LOGE(TAG, "Failed to create ring buffer mutex");
	}
}

void write_ble_in_buf(uint8_t *data, uint8_t len)
{
	// ESP_LOGI(TAG, "Received %d bytes:", len);
	// ESP_LOG_BUFFER_HEX(TAG, data, len);

	/* The last byte of each row stores the valid data length of that row,
	* so a single row can hold at most BLE_IN_BUF_COLS - 1 bytes of data. */
	if(data == NULL || len == 0 || len > BLE_IN_BUF_COLS - 1){
		ESP_LOGW(TAG, "Invalid input: data=%X, len=%d", data, len);
		return;
	}

	/* Block until the mutex is acquired (no timeout). */
	xSemaphoreTake(s_ble_in_buf_mutex, portMAX_DELAY);

	/* Buffer full: no free row, drop this packet. */
	if(BLE_IN_BUF_used >= BLE_IN_BUF_ROWS){
		ESP_LOGW(TAG, "Ring buffer full, drop %d bytes", len);
		xSemaphoreGive(s_ble_in_buf_mutex);
		return;
	}

	/* Write into the first free row = start row + used rows, modulo total rows. */
	uint8_t row = (BLE_IN_BUF_start + BLE_IN_BUF_used) % BLE_IN_BUF_ROWS;
	memcpy(BLE_IN_BUF[row], data, len);
	BLE_IN_BUF[row][BLE_IN_BUF_COLS - 1] = len; /* Store the data length in the row tail byte. */
	BLE_IN_BUF_used++;

	xSemaphoreGive(s_ble_in_buf_mutex);
}

uint8_t read_ble_in_buf(uint8_t *data)
{
	uint8_t len = 0;

	/* Block until the mutex is acquired (no timeout). */
	xSemaphoreTake(s_ble_in_buf_mutex, portMAX_DELAY);

	if(BLE_IN_BUF_used > 0){ /* Buffer not empty: read the head row, then remove it. */
		uint8_t row = BLE_IN_BUF_start;
		len = BLE_IN_BUF[row][BLE_IN_BUF_COLS - 1];
		if(len > BLE_IN_BUF_COLS - 1){ /* Defensive: clamp abnormal length to avoid an out-of-bounds read. */
			len = BLE_IN_BUF_COLS - 1;
		}
		if(len > 0 && data != NULL){
			memcpy(data, BLE_IN_BUF[row], len);
		}
		BLE_IN_BUF[row][BLE_IN_BUF_COLS - 1] = 0;                    /* Clear the length marker. */
		BLE_IN_BUF_start = (BLE_IN_BUF_start + 1) % BLE_IN_BUF_ROWS; /* Remove the row. */
		BLE_IN_BUF_used--;
	}

	xSemaphoreGive(s_ble_in_buf_mutex);
	return len;
}


