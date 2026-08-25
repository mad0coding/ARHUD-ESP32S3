#ifndef _OBD_H
#define _OBD_H

#include "esp_log.h"

// CAN GPIOs
#define IO_CAN_TX			GPIO_NUM_41
#define IO_CAN_RX			GPIO_NUM_42

// OBD-II standard definitions
#define OBD_ID_BROADCAST	0x7DF // OBD broadcast request ID
#define OBD_ID_SPEED_RESP	0x7E8 // Engine ECU response ID

// Value returned by obd_get_speed_resp() on timeout/failure
#define OBD_SPEED_INVALID	0xFFFF


extern uint8_t obd_valid, obd_speed;


/**
 * @brief Periodic OBD reader task (sends speed query and logs the response).
 */
void obd_task(void *pvParameters);


#endif



