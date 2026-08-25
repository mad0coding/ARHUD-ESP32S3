#include "OBD.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"

static const char *TAG = "OBD";

uint8_t obd_valid = 0, obd_speed = 0;

static void init_twai(void){
	// Mount and configure the TWAI controller (500 kbps)
	twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(IO_CAN_TX, IO_CAN_RX, TWAI_MODE_NO_ACK); // no ack mode
	twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();

	// Acceptance filter: only accept the engine ECU speed response ID (0x7E8).
	// In the SJA1000-compatible acceptance filter, a standard 11-bit ID is placed
	// at bits [31:21] of the acceptance code register; mask bit = 0 means "must match",
	// bit = 1 means "don't care".
	twai_filter_config_t f_config = {
		.acceptance_code = ((uint32_t)OBD_ID_SPEED_RESP) << 21,
		.acceptance_mask = ~(((uint32_t)0x7FF) << 21), // Only the 11 ID bits are checked
		.single_filter = true,
	};

	ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
	ESP_ERROR_CHECK(twai_start());
	ESP_LOGI(TAG, "TWAI driver initialized successfully at 500 kbps");
}

static void obd_send_speed_req(void){
	twai_message_t request = {
		.identifier = OBD_ID_BROADCAST,
		.extd = 0, // 11-bit standard frame
		.rtr = 0, // Data frame
		.data_length_code = 8, // OBD always transmits 8 bytes
		.data = {0x02, 0x01, 0x0D, 0x55, 0x55, 0x55, 0x55, 0x55} // Length=2, Mode=01, PID=0x0D
	};

	if(twai_transmit(&request, pdMS_TO_TICKS(100)) != ESP_OK){
		ESP_LOGE(TAG, "Speed req failed to send.");
	}
}

// Return the speed (km/h) if rx_msg is a valid speed response frame,
// otherwise return OBD_SPEED_INVALID.
static uint16_t obd_parse_speed_resp(const twai_message_t *rx_msg)
{
	if (!rx_msg->extd && rx_msg->identifier == OBD_ID_SPEED_RESP &&
		rx_msg->data[1] == 0x41 && rx_msg->data[2] == 0x0D){
		return rx_msg->data[3]; // Byte 3 carries the speed in km/h
	}
	return OBD_SPEED_INVALID;
}

static uint16_t obd_get_speed_resp(uint32_t timeout_ms)
{
	twai_message_t rx_msg;

	// Non-blocking mode (timeout_ms == 0): do not send a new request, just
	// drain the current RX queue until it is empty or a speed response is
	// found. twai_receive(..., 0) never blocks, so no tick/deadline accounting
	// is involved and the drain cannot be cut short at a tick boundary.
	if(timeout_ms == 0){
		while(twai_receive(&rx_msg, 0) == ESP_OK){
			uint16_t speed = obd_parse_speed_resp(&rx_msg);
			if (speed != OBD_SPEED_INVALID){
				ESP_LOGI(TAG, "Speed: %d kph", speed);
				return speed;
			}
		}
		return OBD_SPEED_INVALID; // Queue drained without a speed response
	}

	// Blocking mode: send the speed query once, then wait up to timeout_ms.
	TickType_t start = xTaskGetTickCount();
	uint32_t remaining_ms = timeout_ms;

	// obd_send_speed_req();

	while(remaining_ms > 0)
	{
		// Wait for the next frame, or until the remaining timeout elapses.
		if (twai_receive(&rx_msg, pdMS_TO_TICKS(remaining_ms)) != ESP_OK){
			break; // Timed out without receiving any frame
		}

		uint16_t speed = obd_parse_speed_resp(&rx_msg);
		if (speed != OBD_SPEED_INVALID){
			ESP_LOGI(TAG, "Speed: %d kph", speed);
			return speed;
		}

		// Ignore unrelated frames and recompute the remaining time.
		uint32_t elapsed_ms = (uint32_t)(xTaskGetTickCount() - start) * portTICK_PERIOD_MS;
		remaining_ms = (elapsed_ms >= timeout_ms) ? 0 : (timeout_ms - elapsed_ms);
	}

	// ESP_LOGW(TAG, "Speed resp timeout.");
	return OBD_SPEED_INVALID;
}

void obd_task(void *pvParameters){
	TickType_t xLastWakeTime = xTaskGetTickCount(); // record current time
	const TickType_t xFrequency = pdMS_TO_TICKS(500); // 500ms

	init_twai();

	while(1){
		obd_send_speed_req(); // send the query request

		uint16_t resp_data = obd_get_speed_resp(200); // timeout 200ms

		if(resp_data <= 255){ // valid resp
			obd_speed = resp_data;
			obd_valid = 1;
			// ESP_LOGI(TAG, "Speed resp: %d.", resp_data);
		}
		else{ // no resp
			obd_valid = 0;
			// ESP_LOGW(TAG, "Speed resp timeout.");
		}

		xTaskDelayUntil(&xLastWakeTime, xFrequency); // sampling interval
	}
}




