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


esp_lcd_panel_handle_t panel_handle = NULL; // 屏幕设备句柄

uint8_t *LCD_Buf = NULL, *LCD_Buf0 = NULL, *LCD_Buf1 = NULL; // 选中的缓冲指针 和 双缓冲实际指针


void init_rgb(void)
{
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
}

// 参数结构体
typedef struct {
	uint8_t *canvas;		// 原始大图指针 (数组)
	uint16_t canvas_w;		// 原始大图宽
	uint16_t canvas_h;		// 原始大图高
	
	const uint8_t *matrix;	// 颜色内容矩阵指针
	uint16_t matrix_m;		// 内容矩阵行数 (高)
	uint16_t matrix_n;		// 内容矩阵列数 (宽)
	uint16_t block_w;		// 每个色块的宽度 w
	uint16_t block_h;		// 每个色块的高度 h
	
	uint16_t start_x;		// 填充区域起点 x
	uint16_t start_y;		// 填充区域起点 y
	uint16_t fill_w;		// 填充区域总宽
	uint16_t fill_h;		// 填充区域总高
} TileConfig_t;

/**
 * @brief 在现有图像上进行地砖式填充
 * @param cfg 配置结构体指针
 */
void fill_tiled_pattern(const TileConfig_t *cfg) {
	// 基础单元的总像素尺寸
	uint16_t unit_pixel_w = cfg->matrix_n * cfg->block_w;
	uint16_t unit_pixel_h = cfg->matrix_m * cfg->block_h;

	// 遍历填充区域的每一个像素 (relative_y, relative_x 是相对于填充起点的坐标)
	for (uint16_t ry = 0; ry < cfg->fill_h; ry++) {
		// 边界检查：如果超出大图高度则停止该行
		if (cfg->start_y + ry >= cfg->canvas_h) break;

		for (uint16_t rx = 0; rx < cfg->fill_w; rx++) {
			// 边界检查：如果超出大图宽度则跳过该点
			if (cfg->start_x + rx >= cfg->canvas_w) break;

			// --- 核心逻辑：计算当前点落在矩阵的哪个位置 ---
			
			// 1. 计算当前点在“基础单元”内的像素偏移
			uint16_t inner_x = rx % unit_pixel_w;
			uint16_t inner_y = ry % unit_pixel_h;

			// 2. 计算当前点属于颜色矩阵的第几行、第几列
			uint16_t col_idx = inner_x / cfg->block_w;
			uint16_t row_idx = inner_y / cfg->block_h;

			// 3. 取出对应的颜色
			uint16_t color = cfg->matrix[row_idx * cfg->matrix_n + col_idx];

			// 4. 写入大图
			// 假设大图是按行存储的一维数组：index = y * width + x
			uint32_t target_idx = (uint32_t)(cfg->start_y + ry) * cfg->canvas_w + (cfg->start_x + rx);
			cfg->canvas[target_idx] = color;
		}
	}
}

void fill_oblique_pattern(uint8_t *canvas, uint16_t canvas_w, uint16_t canvas_h){
	static uint16_t frameCnt = 0;
	frameCnt += 8;
	for(uint16_t y = 0; y < canvas_h; y++){ // 斜交变色光效
		for(uint16_t x = 0; x < canvas_w; x++){
			uint8_t r8 = frameCnt + y + x; // R
			uint8_t g8 = frameCnt - y; // G
			uint8_t b8 = frameCnt + y; // B
			canvas[y*canvas_w + x] = (r8 & 0xE0) | (g8 & 0xE0) >> 3 | (b8 & 0xC0) >> 6;
		}
	}
}

const uint16_t fill_size_list[] = { // 填充尺寸切换列表
	// 250, 200,
	// 200, 200,
	// 200, 150,
	// 200, 100,
	// 150, 100,
	500, 400,
	400, 400,
	400, 300,
	400, 200,
	300, 200,
};
const uint8_t my_pattern[] = { // 定义一个内容矩阵
	// COLOR_M, COLOR_B,
	// COLOR_Y, COLOR_G,
	// COLOR_C, COLOR_R,
	// COLOR_A, COLOR_W,
	COLOR_B, COLOR_G, COLOR_R, COLOR_W,
	COLOR_M, COLOR_Y, COLOR_C, COLOR_A,
};

void rgb_test(void){
	TileConfig_t cfg;
	cfg.canvas_w = LCD_H_RES; // 屏幕尺寸
	cfg.canvas_h = LCD_V_RES;
	cfg.matrix = (const uint8_t *)my_pattern; // 内容矩阵指针
	cfg.matrix_m = 2; // 内容矩阵行数
	cfg.matrix_n = 4; // 内容矩阵列数
	cfg.block_w = 100; // 每个色块宽
	cfg.block_h = 100; // 每个色块高

	uint8_t pattern = 0; // 图案选择
	uint8_t key_old = 0; // 按键旧状态

	while(1){
		ESP_LOGI("RGB", "test"); // 输出日志到串口
		// if(!gpio_get_level(IO_KEY)) ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_LIGHT, 2048));
		// else ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_LIGHT, 500));
		// ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_LIGHT));

		if(key_old != !gpio_get_level(IO_KEY)){ // 边沿
			key_old = !key_old;
			if(key_old){ // 按下边沿
				pattern = (pattern + 1) % 6; // 切换图案
				memset(LCD_Buf0, 0, LCD_FRAME_SIZE); // 清空缓存
				memset(LCD_Buf1, 0, LCD_FRAME_SIZE);
			}
		}

		LCD_Buf = (LCD_Buf != LCD_Buf0) ? LCD_Buf0 : LCD_Buf1; // 双缓冲指针切换

		if(pattern < sizeof(fill_size_list) / (2*sizeof(fill_size_list[0]))){
			cfg.canvas = LCD_Buf; // 指向屏幕缓存
			cfg.fill_w = fill_size_list[pattern*2 + 0]; // 填充尺寸
			cfg.fill_h = fill_size_list[pattern*2 + 1];
			cfg.start_x = (cfg.canvas_w - cfg.fill_w) / 2; // 填充起点 (这里自动居中)
			cfg.start_y = (cfg.canvas_h - cfg.fill_h) / 2;
			fill_tiled_pattern(&cfg); // 填充图案
		}
		else{
			fill_oblique_pattern(LCD_Buf, LCD_H_RES, LCD_V_RES); // 填充图案
		}

		// gpio_set_level(IO_LED, 1);
		esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_H_RES, LCD_V_RES, LCD_Buf);
		// gpio_set_level(IO_LED, 0);
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	// 将这块数据刷新到帧缓存的 (0,0) 到 (10,10) 坐标 外设会自动反复发送
	// esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 10, 10, color_data);
	// esp_lcd_panel_draw_bitmap(panel_handle, 10, 10, 10, 10, color_data);
}




