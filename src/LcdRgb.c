#include "LcdRgb.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"


uint8_t *LCD_Buf = NULL, *LCD_Buf0 = NULL, *LCD_Buf1 = NULL; // 选中的缓冲指针 和 双缓冲实际指针


void rgb_main(void)
{
	esp_lcd_panel_handle_t panel_handle = NULL;

	// 1. 配置 RGB 面板参数
	esp_lcd_rgb_panel_config_t rgb_cfg = {
		.clk_src = LCD_CLK_SRC_DEFAULT,
		.timings = {
			// 时钟频率
			.pclk_hz = 20 * 1000 * 1000, // 20MHz
			// 屏幕尺寸
			.h_res = LCD_H_RES,
			.v_res = LCD_V_RES,
			// 时序参数
			.hsync_pulse_width = 8,
			.hsync_back_porch = 10,
			.hsync_front_porch = 50,
			.vsync_pulse_width = 4,
			.vsync_back_porch = 16,
			.vsync_front_porch = 60,
		},
		.data_width = 8, // number of data lines
		.bits_per_pixel = 8, // color depth
		.de_gpio_num = LCD_PIN_DE,
		.pclk_gpio_num = LCD_PIN_PCLK,
		.vsync_gpio_num = LCD_PIN_VSYNC,
		.hsync_gpio_num = LCD_PIN_HSYNC,
		.data_gpio_nums = {
			LCD_PIN_DATA0, LCD_PIN_DATA1, LCD_PIN_DATA2, LCD_PIN_DATA3,
			LCD_PIN_DATA4, LCD_PIN_DATA5, LCD_PIN_DATA6, LCD_PIN_DATA7, // 填满 8 或 16 个引脚
		},
		.flags.fb_in_psram = 1, // 将帧缓冲放在PSRAM
		.num_fbs = 2, // 申请两个缓冲区
		.flags.double_fb = 0, // 不使用双缓冲自动切换 后面手动设置
		.flags.refresh_on_demand = 0, // 不手动刷新 使用自动刷新
	};

	// 2. 安装 RGB 面板驱动
	ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&rgb_cfg, &panel_handle));

	// 3. 初始化面板
	ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
	ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

	// 获取驱动初始化时自动创建的2个缓冲区地址
	ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, (void**)&LCD_Buf0, (void**)&LCD_Buf1));
	printf("LCD_Buf0: 0x%lX, LCD_Buf1: 0x%lX\n", (uint32_t)LCD_Buf0, (uint32_t)LCD_Buf1);
	// memset(LCD_Buf0, 0xE0, LCD_FRAME_SIZE);
	// memset(LCD_Buf1, 0x1C, LCD_FRAME_SIZE);
	
	uint16_t frameCnt = 0;
	while(1){
		frameCnt += 8;
		ESP_LOGI("RGB", "test"); // 输出日志到串口

		LCD_Buf = (LCD_Buf != LCD_Buf0) ? LCD_Buf0 : LCD_Buf1; // 双缓冲指针切换

		for(int j = 0; j < LCD_V_RES; j++){ // 双向变色光效
			uint16_t y = j;
			for(int i = 0; i < LCD_H_RES; i++){
				uint16_t x = i;
				uint8_t r8 = frameCnt + y + x; // R
				uint8_t g8 = frameCnt - y; // G
				uint8_t b8 = frameCnt + y; // B
				LCD_Buf[y*LCD_H_RES + x] = (r8 & 0xE0) | (g8 & 0xE0) >> 3 | (b8 & 0xC0) >> 6;
			}
		}
		// gpio_set_level(IO_LED, 1);
		esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_H_RES, LCD_V_RES, LCD_Buf);
		// gpio_set_level(IO_LED, 0);
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	// 将这块数据刷新到帧缓存的 (0,0) 到 (10,10) 坐标 外设会自动反复发送
	// esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 10, 10, color_data);
	// esp_lcd_panel_draw_bitmap(panel_handle, 10, 10, 10, 10, color_data);

	printf("LCD RGB panel initialized and test pattern sent.\n");
}