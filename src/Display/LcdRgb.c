#include "LcdRgb.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/gpio.h"

#include "esp_log.h"

#include "lvgl.h"

esp_lcd_panel_handle_t panel_handle = NULL; // Screen device handle

uint8_t *LCD_Buf = NULL, *LCD_Buf0 = NULL, *LCD_Buf1 = NULL; // Selected buffer pointer & Double-buffer actual pointer


void init_rgb(void)
{
	// 1. Configure RGB panel parameters
	esp_lcd_rgb_panel_config_t rgb_cfg = {
		.clk_src = LCD_CLK_SRC_DEFAULT,
		.timings = {
			// Clock frequency
			.pclk_hz = 20 * 1000 * 1000, // 20MHz
			// Screen size
			.h_res = LCD_H_RES,
			.v_res = LCD_V_RES,
			// Timing parameters
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
			LCD_PIN_DATA4, LCD_PIN_DATA5, LCD_PIN_DATA6, LCD_PIN_DATA7, // Fill 8 or 16 pins
		},
		.flags.fb_in_psram = 1, // Place the frame buffer in PSRAM
		.num_fbs = 2, // Request for two buffers
		.flags.double_fb = 0, // Disable double-buffer auto switch. Manually switch later.
		.flags.refresh_on_demand = 0, // Do not refresh manually. Use autoc refresh.
	};

	// 2. Install RGB panel driver
	ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&rgb_cfg, &panel_handle));

	// 3. Init panel
	ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
	ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

	// Get the addr of the two buffers auto created during driver init.
	ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, (void**)&LCD_Buf0, (void**)&LCD_Buf1));
	// printf("LCD_Buf0: 0x%lX, LCD_Buf1: 0x%lX\n", (uint32_t)LCD_Buf0, (uint32_t)LCD_Buf1);
	// memset(LCD_Buf0, COLOR_R, LCD_FRAME_SIZE);
	// memset(LCD_Buf1, COLOR_G, LCD_FRAME_SIZE);
}

// Parameter structure
typedef struct {
	uint8_t *canvas;		// Raw image buffer pointer
	uint16_t canvas_w;		// Raw image width
	uint16_t canvas_h;		// Raw image height
	
	const uint8_t *matrix;	// Color value matrix pointer
	uint16_t matrix_m;		// Matrix row count (height)
	uint16_t matrix_n;		// Matrix column count (width)
	uint16_t block_w;		// Single tile block pixel width
	uint16_t block_h;		// Single tile block pixel height
	
	uint16_t start_x;		// X offset of fill area top-left
	uint16_t start_y;		// Y offset of fill area top-left
	uint16_t fill_w;		// Total width of filled region
	uint16_t fill_h;		// Total height of filled region
} TileConfig_t;

/**
 * @brief Draw tiled pattern onto existing image buffer
 * @param cfg Pointer to tile configuration struct
 */
void fill_tiled_pattern(const TileConfig_t *cfg) {
	// Pixel dimensions of one repeating tile unit
	uint16_t unit_pixel_w = cfg->matrix_n * cfg->block_w;
	uint16_t unit_pixel_h = cfg->matrix_m * cfg->block_h;

	// Iterate every pixel inside target fill area (ry/rx = local coordinate relative to fill origin)
	for (uint16_t ry = 0; ry < cfg->fill_h; ry++) {
		// Abort current row if out of canvas vertical bound
		if (cfg->start_y + ry >= cfg->canvas_h) break;

		for (uint16_t rx = 0; rx < cfg->fill_w; rx++) {
			// Skip pixel if out of canvas horizontal bound
			if (cfg->start_x + rx >= cfg->canvas_w) break;

			// --- Core calculation: map pixel to matrix cell ---
			
			// 1. Pixel offset inside one repeating tile unit
			uint16_t inner_x = rx % unit_pixel_w;
			uint16_t inner_y = ry % unit_pixel_h;

			// 2. Resolve corresponding matrix row & column index
			uint16_t col_idx = inner_x / cfg->block_w;
			uint16_t row_idx = inner_y / cfg->block_h;

			// 3. Fetch target color value from matrix
			uint16_t color = cfg->matrix[row_idx * cfg->matrix_n + col_idx];

			// 4. Write color data to canvas buffer
			// Canvas layout: row-major 1D array, index = y * canvas_width + x
			uint32_t target_idx = (uint32_t)(cfg->start_y + ry) * cfg->canvas_w + (cfg->start_x + rx);
			cfg->canvas[target_idx] = color;
		}
	}
}

void fill_oblique_pattern(uint8_t *canvas, uint16_t canvas_w, uint16_t canvas_h){
	static uint16_t frameCnt = 0;
	frameCnt += 8;
	for(uint16_t y = 0; y < canvas_h; y++){ // Oblique Color-Changing Effect
		for(uint16_t x = 0; x < canvas_w; x++){
			uint8_t r8 = frameCnt + y + x; // R
			uint8_t g8 = frameCnt - y; // G
			uint8_t b8 = frameCnt + y; // B
			canvas[y*canvas_w + x] = (r8 & 0xE0) | (g8 & 0xE0) >> 3 | (b8 & 0xC0) >> 6;
		}
	}
}

const uint16_t fill_size_list[] = { // filling size switching list
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
const uint8_t my_pattern[] = { // content mat
	// COLOR_M, COLOR_B,
	// COLOR_Y, COLOR_G,
	// COLOR_C, COLOR_R,
	// COLOR_A, COLOR_W,
	COLOR_B, COLOR_G, COLOR_R, COLOR_W,
	COLOR_M, COLOR_Y, COLOR_C, COLOR_A,
};

void rgb_test(void){
	TileConfig_t cfg;
	cfg.canvas_w = LCD_H_RES; // screen size
	cfg.canvas_h = LCD_V_RES;
	cfg.matrix = (const uint8_t *)my_pattern; // content mat pointer
	cfg.matrix_m = 2; // content mat rows num
	cfg.matrix_n = 4; // content mat cols num
	cfg.block_w = 100; // color block width
	cfg.block_h = 100; // color block height

	uint8_t pattern = 0; // pattern choice
	uint8_t key_old = 0; // key old state

	while(1){
		ESP_LOGI("RGB", "test");
		// if(!gpio_get_level(IO_KEY)) ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_LIGHT, 2048));
		// else ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_LIGHT, 500));
		// ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL_LIGHT));

		if(key_old != !gpio_get_level(IO_KEY)){ // edge
			key_old = !key_old;
			if(key_old){ // press edge
				pattern = (pattern + 1) % 6; // switch pattern
				memset(LCD_Buf0, 0, LCD_FRAME_SIZE); // clear buffer
				memset(LCD_Buf1, 0, LCD_FRAME_SIZE);
			}
		}

		LCD_Buf = (LCD_Buf != LCD_Buf0) ? LCD_Buf0 : LCD_Buf1; // double-buffer pointer switching

		if(pattern < sizeof(fill_size_list) / (2*sizeof(fill_size_list[0]))){
			cfg.canvas = LCD_Buf; // point to screen buffer
			cfg.fill_w = fill_size_list[pattern*2 + 0]; // size
			cfg.fill_h = fill_size_list[pattern*2 + 1];
			cfg.start_x = (cfg.canvas_w - cfg.fill_w) / 2; // start point (auto centered here)
			cfg.start_y = (cfg.canvas_h - cfg.fill_h) / 2;
			fill_tiled_pattern(&cfg); // fill pattern
		}
		else{
			fill_oblique_pattern(LCD_Buf, LCD_H_RES, LCD_V_RES); // fill pattern
		}

		// gpio_set_level(IO_LED, 1);
		esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_H_RES, LCD_V_RES, LCD_Buf);
		// gpio_set_level(IO_LED, 0);
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	// data will be refreshed to frame buffer at coordinates (0,0) to (10,10)
	// The peripheral will auto send it repeatedly
	// esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 10, 10, color_data);
	// esp_lcd_panel_draw_bitmap(panel_handle, 10, 10, 10, 10, color_data);
}

void lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p){
	// printf("lvgl_flush_cb: (%d,%d)-(%d,%d)\n", area->x1, area->y1, area->x2, area->y2);

	// dst pos
	int32_t x_start = OFFSET_X + area->x1, x_end = OFFSET_X + area->x2 + 1;
	int32_t y_start = OFFSET_Y + area->y1, y_end = OFFSET_Y + area->y2 + 1;
	// src size
	int32_t w = lv_area_get_width(area);
	int32_t h = lv_area_get_height(area);
	// printf("lvgl_flush_cb: %ld,%ld\n", w, h);

	LCD_Buf = (LCD_Buf != LCD_Buf0) ? LCD_Buf0 : LCD_Buf1; // double-buffer pointer switching

	for(int y = 0; y < h; y++){
		uint8_t *dst_addr = LCD_Buf + ((y_start + y) * LCD_H_RES) + x_start;
#if DISPLAY_MIRROR_Y
		lv_color_t *src_addr = color_p + ((h - y - 1) * w);
#else
		lv_color_t *src_addr = color_p + (y * w);
#endif

#if DISPLAY_MIRROR_X
		uint32_t *src32 = (uint32_t *)src_addr;
		uint32_t *dst32 = (uint32_t *)dst_addr;
		int32_t w32 = w / 4;
		for(int x = 0; x < w32; x++){
			dst32[w32 - 1 - x] = __builtin_bswap32(src32[x]); // 4B alignment is required
		}
#else
		memcpy(dst_addr, src_addr, w * sizeof(lv_color_t));
#endif
	}

	esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_H_RES, LCD_V_RES, LCD_Buf); // switch
	
	lv_disp_flush_ready(disp_drv); // tell LVGL that flush done
}


