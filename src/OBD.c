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
		.acceptance_code = ((uint32_t)OBD_ID_BROADCAST) << 21,
		.acceptance_mask = ~(((uint32_t)0x7FF) << 21), // Only the 11 ID bits are checked
		.single_filter = true,
	};

	ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
	ESP_ERROR_CHECK(twai_start());
	ESP_LOGI(TAG, "TWAI driver initialized successfully at 500 kbps");
}

static void obd_send_speed_resp(uint8_t speed){
	twai_message_t response = {
		.identifier = OBD_ID_SPEED_RESP,
		.extd = 0,
		.rtr = 0,
		.data_length_code = 8,
		// Data: [Length=3, Mode+0x40=0x41, PID=0x0D, Speed, Padding...]
		.data = {0x03, 0x41, 0x0D, speed, 0xAA, 0xAA, 0xAA, 0xAA}
	};

	if (twai_transmit(&response, pdMS_TO_TICKS(100)) == ESP_OK) {
		ESP_LOGI(TAG, "Resp speed: %d kph", speed);
	}else{
		ESP_LOGE(TAG, "Resp failed.");
	}
}

void obd_task(void *pvParameters){
	init_twai();
	twai_message_t rx_msg;

	while(1){
		// blocking wait req
		if (twai_receive(&rx_msg, pdMS_TO_TICKS(portMAX_DELAY)) == ESP_OK){
			// check if it's standard broadcast frame 0x7DF
			if(!rx_msg.extd && rx_msg.identifier == OBD_ID_BROADCAST){
				// check req: Mode 01 (rx_msg.data[1]) & PID 0x0D (rx_msg.data[2])
				if(rx_msg.data[1] == 0x01 && rx_msg.data[2] == 0x0D){
					
					// TEST CODE: update simulated speed (0 -> 120 kph cycle)
					obd_speed = (obd_speed + 5) % 125;
					
					obd_send_speed_resp(obd_speed); // resp speed
				}
			}
		}
	}
}




