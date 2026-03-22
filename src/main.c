#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "app_main"; // 定义日志标签

void app_main(void)
{
	while(1){
		ESP_LOGI(TAG, "Hello ESP32S3!"); // 输出日志到串口
		vTaskDelay(pdMS_TO_TICKS(1000)); // 延时1000ms
	}
}
