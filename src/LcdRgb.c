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

// 定义屏幕参数 (根据你的屏幕手册修改)
#define LCD_H_RES				(4*160)//640
#define LCD_V_RES				(3*160)//480
// #define LCD_PIN_PCLK			GPIO_NUM_42
// #define LCD_PIN_DE				GPIO_NUM_41
// #define LCD_PIN_VSYNC			GPIO_NUM_40
// #define LCD_PIN_HSYNC			GPIO_NUM_39

#define LCD_PIN_PCLK			GPIO_NUM_8
#define LCD_PIN_DE				GPIO_NUM_46
#define LCD_PIN_VSYNC			GPIO_NUM_3
#define LCD_PIN_HSYNC			GPIO_NUM_NC

// #define LCD_PIN_PCLK			GPIO_NUM_8
// #define LCD_PIN_DE				GPIO_NUM_NC
// #define LCD_PIN_VSYNC			GPIO_NUM_3
// #define LCD_PIN_HSYNC			GPIO_NUM_46

// RGB 数据引脚 (这里仅列出部分示例，S3最多支持8位或16位)
// #define LCD_PIN_DATA0			GPIO_NUM_1
// #define LCD_PIN_DATA1			GPIO_NUM_2
// #define LCD_PIN_DATA2			GPIO_NUM_3
// #define LCD_PIN_DATA3			GPIO_NUM_4

// #define LCD_PIN_DATA0			GPIO_NUM_4
// #define LCD_PIN_DATA1			GPIO_NUM_5
// #define LCD_PIN_DATA2			GPIO_NUM_6
// #define LCD_PIN_DATA3			GPIO_NUM_8
// #define LCD_PIN_DATA4			GPIO_NUM_15
// #define LCD_PIN_DATA5			GPIO_NUM_16
// #define LCD_PIN_DATA6			GPIO_NUM_17
// #define LCD_PIN_DATA7			GPIO_NUM_18

#define LCD_PIN_DATA0			GPIO_NUM_16
#define LCD_PIN_DATA1			GPIO_NUM_15
#define LCD_PIN_DATA2			GPIO_NUM_18
#define LCD_PIN_DATA3			GPIO_NUM_17
#define LCD_PIN_DATA4			GPIO_NUM_7
#define LCD_PIN_DATA5			GPIO_NUM_6
#define LCD_PIN_DATA6			GPIO_NUM_5
#define LCD_PIN_DATA7			GPIO_NUM_4
// ... 继续定义其他数据引脚

void rgb_main(void)
{
	esp_lcd_panel_handle_t panel_handle = NULL;

	// 1. 配置 RGB 面板参数
	esp_lcd_rgb_panel_config_t rgb_cfg = {
		.clk_src = LCD_CLK_SRC_DEFAULT,
		.timings = {
			.pclk_hz = 8 * 1000 * 1000, // 6MHz
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
		.data_width = /*16*/8, // number of data lines
		.bits_per_pixel = /*16*/24, // color depth
		.de_gpio_num = LCD_PIN_DE,
		.pclk_gpio_num = LCD_PIN_PCLK,
		.vsync_gpio_num = LCD_PIN_VSYNC,
		.hsync_gpio_num = LCD_PIN_HSYNC,
		.data_gpio_nums = {
			LCD_PIN_DATA0, LCD_PIN_DATA1, LCD_PIN_DATA2, LCD_PIN_DATA3,
			LCD_PIN_DATA4, LCD_PIN_DATA5, LCD_PIN_DATA6, LCD_PIN_DATA7, // ... 填满 16 个引脚
		},
		.flags.refresh_on_demand = 1, // 不用自动刷新 改为手动刷新
		.flags.fb_in_psram = 1, // 将帧缓冲放在PSRAM
		.flags.double_fb = 1, // 使用双缓冲
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
		color_data[i] = (~(i+1) << 12) | ((i+1) << 4);//0xABCD;
	}
	
	uint16_t frameCnt = 0;
	uint16_t DVP_bufAddr[512];
	while(1){frameCnt += 8;
	// vTaskDelay(pdMS_TO_TICKS(1)); // 延时1ms
	// esp_task_wdt_reset();
	ESP_LOGI("RGB", "test"); // 输出日志到串口
	for(int j = 0; j < (640*480/160); j++){
		uint16_t x = (j % 4) * 160, y = j / 4;
		for(int i = 0; i < 160; i++){ // 模拟DVP DMA填充RGB数据
			((volatile uint8_t*)DVP_bufAddr)[i*3 + 0] = frameCnt + y+x+i/* (((y+x+i)/320)*2-1)*/; // R
			((volatile uint8_t*)DVP_bufAddr)[i*3 + 1] = frameCnt - y; // G
			((volatile uint8_t*)DVP_bufAddr)[i*3 + 2] = frameCnt + y; // B
			// ((volatile uint8_t*)DVP_bufAddr)[i*3 + 0] = j+i*3+0;//128; // R
			// ((volatile uint8_t*)DVP_bufAddr)[i*3 + 1] = j+i*3+1;//128 - j; // G
			// ((volatile uint8_t*)DVP_bufAddr)[i*3 + 2] = j+i*3+2;//128 + j; // B
			// ((volatile uint8_t*)DVP_bufAddr)[i*3 + 0] = j;//128; // R
			// ((volatile uint8_t*)DVP_bufAddr)[i*3 + 1] = j;//128 - j; // G
			// ((volatile uint8_t*)DVP_bufAddr)[i*3 + 2] = j;//128 + j; // B
			// ((volatile uint8_t*)DVP_bufAddr)[i*3 + 0] = 0;//128; // R
			// ((volatile uint8_t*)DVP_bufAddr)[i*3 + 1] = 0;//128 - j; // G
			// ((volatile uint8_t*)DVP_bufAddr)[i*3 + 2] = 0;//128 + j; // B
		}
		// ((uint16_t*)DVP_bufAddr)[0] = 0x5A5A; // 测试代码
		// ((uint16_t*)DVP_bufAddr)[1] = j; // j
		// ((uint16_t*)DVP_bufAddr)[2] = x; // x
		// ((uint16_t*)DVP_bufAddr)[3] = y; // y
		// ((uint16_t*)DVP_bufAddr)[(160*3-2)/2] = 0x1616; // 
		// ((uint16_t*)DVP_bufAddr)[(160*3+0)/2] = 0x2F2F; // 
		// esp_lcd_panel_draw_bitmap(panel_handle, 0, j, 160, j+1, DVP_bufAddr);
		esp_lcd_panel_draw_bitmap(panel_handle, x+0, y, x+160, y+1, DVP_bufAddr);
	}
	vTaskDelay(pdMS_TO_TICKS(40));
	esp_lcd_rgb_panel_refresh(panel_handle);
	}
	// 将这块数据刷新到帧缓存的 (0,0) 到 (10,10) 坐标 外设会自动反复发送
	// esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 10, 10, color_data);
	// esp_lcd_panel_draw_bitmap(panel_handle, 10, 10, 10, 10, color_data);

	printf("LCD RGB panel initialized and test pattern sent.\n");
}