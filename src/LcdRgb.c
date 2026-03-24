#include "LcdRgb.h"


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/gpio.h"

// 定义屏幕参数 (根据你的屏幕手册修改)
#define LCD_H_RES				4//480
#define LCD_V_RES				3//272
#define LCD_PIN_PCLK			GPIO_NUM_42
#define LCD_PIN_DE				GPIO_NUM_41
#define LCD_PIN_VSYNC			GPIO_NUM_40
#define LCD_PIN_HSYNC			GPIO_NUM_39

// RGB 数据引脚 (这里仅列出部分示例，S3最多支持16位或24位)
#define LCD_PIN_DATA0			GPIO_NUM_1
#define LCD_PIN_DATA1			GPIO_NUM_2
#define LCD_PIN_DATA2			GPIO_NUM_3
#define LCD_PIN_DATA3			GPIO_NUM_4
#define LCD_PIN_DATA4			GPIO_NUM_15
#define LCD_PIN_DATA5			GPIO_NUM_16
#define LCD_PIN_DATA6			GPIO_NUM_17
#define LCD_PIN_DATA7			GPIO_NUM_18
// ... 继续定义其他数据引脚

void rgb_main(void)
{
	esp_lcd_panel_handle_t panel_handle = NULL;

	// 1. 配置 RGB 面板参数
	esp_lcd_rgb_panel_config_t rgb_cfg = {
		.clk_src = LCD_CLK_SRC_DEFAULT,
		.timings = {
			.pclk_hz = 6 * 1000 * 1000, // 6MHz
			.h_res = LCD_H_RES,
			.v_res = LCD_V_RES,
			// .hsync_pulse_width = 4,
			// .hsync_back_porch = 8,
			// .hsync_front_porch = 8,
			// .vsync_pulse_width = 4,
			// .vsync_back_porch = 8,
			// .vsync_front_porch = 8,
			.hsync_pulse_width = 1,
			.hsync_back_porch = 1,
			.hsync_front_porch = 1,
			.vsync_pulse_width = 1,
			.vsync_back_porch = 1,
			.vsync_front_porch = 1,
		},
		.data_width = 16, // RGB565 为 16位
		.bits_per_pixel = 16,
		.de_gpio_num = LCD_PIN_DE,
		.pclk_gpio_num = LCD_PIN_PCLK,
		.vsync_gpio_num = LCD_PIN_VSYNC,
		.hsync_gpio_num = LCD_PIN_HSYNC,
		.data_gpio_nums = {
			LCD_PIN_DATA0, LCD_PIN_DATA1, LCD_PIN_DATA2, LCD_PIN_DATA3,
			LCD_PIN_DATA4, LCD_PIN_DATA5, LCD_PIN_DATA6, LCD_PIN_DATA7, // ... 填满 16 个引脚
		},
		.flags.fb_in_psram = 1, // RGB 接口通常需要将帧缓冲区放在 PSRAM 中
	};

	// 2. 安装 RGB 面板驱动
	ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&rgb_cfg, &panel_handle));

	// 3. 初始化面板
	ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
	ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

	// 4. 测试：绘制一个简单的色块
	// 定义一个 10x10 的红色区域 (RGB565: 0xF800)
	uint16_t color_data[4 * 3];
	for(int i = 0; i < 4 * 3; i++){
		color_data[i] = (i+1) << 4;//0xABCD;
	}

	// 将这块数据刷新到帧缓存的 (0,0) 到 (10,10) 坐标 外设会自动反复发送
	esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 10, 10, color_data);

	printf("LCD RGB panel initialized and test pattern sent.\n");
}