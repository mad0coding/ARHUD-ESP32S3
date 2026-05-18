#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "esp_flash.h"
#include "esp_partition.h"

#include <stdio.h>
#include "esp_log.h"
#include "esp_heap_caps.h"

#include "LcdRgb.h"
#include "BasicIO.h"

void check_memory() {
	// 检查总的外置内存
	size_t psram_size = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
	size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
	
	printf("PSRAM total size: %d bytes (%d MB)\n", psram_size, psram_size / 1024 / 1024);
	printf("PSRAM free size: %d bytes\n", free_psram);

	// 测试申请一段大的内存
	void* test_ptr = heap_caps_malloc(1024 * 1024 * 2, MALLOC_CAP_SPIRAM); // 申请 2MB
	if (test_ptr != NULL) {
		ESP_LOGI("MEM", "PSRAM malloc succeed.");
		heap_caps_free(test_ptr);
	} else {
		ESP_LOGE("MEM", "PSRAM malloc failed.");
	}
}

void check_flash() {
	uint32_t flash_size;
	esp_flash_get_size(NULL, &flash_size);
	printf("Flash size: %lu MB\n", flash_size / (1024 * 1024));
	
	// 打印分区表信息
	esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
	while (it != NULL) {
		const esp_partition_t *p = esp_partition_get(it);
		printf("partition: %s, size: %ld KB\n", p->label, p->size / 1024);
		it = esp_partition_next(it);
	}
}

static const char *TAG = "app_main"; // 定义日志标签

void app_main(void)
{
	io_main();
	init_rgb();
	rgb_test();
	while(1){
		// check_memory();
		// check_flash();
		ESP_LOGI(TAG, "Hello ESP32S3!"); // 输出日志到串口
		vTaskDelay(pdMS_TO_TICKS(1000)); // 延时1000ms
	}
}
