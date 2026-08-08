#include "UI.h"

#include "freertos/FreeRTOS.h"
#include "lvgl.h"

#include "LcdRgb.h"

// custom font
LV_FONT_DECLARE(lv_font_MontserratBold_150);
LV_FONT_DECLARE(lv_font_MontserratBold_60);
LV_FONT_DECLARE(lv_font_RobotoBold_50);

// -------------------------------------------------- Speed Limit Sign --------------------------------------------------
typedef struct{
	lv_obj_t *out_circle; // Outer circle (as the parent object)
	lv_obj_t *in_circle; // Inner circle
	lv_obj_t *label; // Number label
}speed_limit_sign_t;
speed_limit_sign_t speed_limit_sign;

void speed_limit_sign_set_visible(uint8_t visible){
	if(visible) lv_obj_clear_flag(speed_limit_sign.out_circle, LV_OBJ_FLAG_HIDDEN);
	else lv_obj_add_flag(speed_limit_sign.out_circle, LV_OBJ_FLAG_HIDDEN);
}

void speed_limit_sign_set_value(int16_t new_value){
	static int16_t old_value = 100;
	lv_label_set_text_fmt(speed_limit_sign.label, "%d", new_value); // set value
	if(new_value < 100) lv_obj_set_style_text_letter_space(speed_limit_sign.label, 0, LV_PART_MAIN);
	else lv_obj_set_style_text_letter_space(speed_limit_sign.label, -5, LV_PART_MAIN); // neg spacing for 3-digit num
	if((old_value < 100) ^ (new_value < 100)) lv_obj_center(speed_limit_sign.label); // re-center
	old_value = new_value;
}

void speed_limit_sign_create(lv_obj_t *parent, lv_coord_t x, lv_coord_t y){
	// 1. Outer circle (as the parent object)
	speed_limit_sign.out_circle = lv_obj_create(parent);
	lv_obj_remove_style_all(speed_limit_sign.out_circle);
	lv_obj_set_size(speed_limit_sign.out_circle, 100, 100); // diameter
	lv_obj_set_pos(speed_limit_sign.out_circle, x, y);
	lv_obj_set_style_bg_color(speed_limit_sign.out_circle, lv_color_hex(0xFF0000), LV_STATE_DEFAULT); // red
	lv_obj_set_style_bg_opa(speed_limit_sign.out_circle, LV_OPA_COVER, LV_STATE_DEFAULT);
	lv_obj_set_style_radius(speed_limit_sign.out_circle, LV_RADIUS_CIRCLE, LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(speed_limit_sign.out_circle, 0, LV_STATE_DEFAULT);

	// 2. Inner circle (attached to the outer circle)
	speed_limit_sign.in_circle = lv_obj_create(speed_limit_sign.out_circle);
	lv_obj_remove_style_all(speed_limit_sign.in_circle);
	lv_obj_set_size(speed_limit_sign.in_circle, 76, 76); // diameter
	lv_obj_set_pos(speed_limit_sign.in_circle, 12, 12); // (100-76)/2 = 12
	lv_obj_set_style_bg_color(speed_limit_sign.in_circle, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT); // white
	lv_obj_set_style_bg_opa(speed_limit_sign.in_circle, LV_OPA_COVER, LV_STATE_DEFAULT);
	lv_obj_set_style_radius(speed_limit_sign.in_circle, LV_RADIUS_CIRCLE, LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(speed_limit_sign.in_circle, 0, LV_STATE_DEFAULT);

	// 3. Number label (attached to the outer circle)
	speed_limit_sign.label = lv_label_create(speed_limit_sign.out_circle);
	lv_obj_set_style_text_color(speed_limit_sign.label, lv_color_hex(0x000000), LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(speed_limit_sign.label, &lv_font_RobotoBold_50, LV_STATE_DEFAULT); // font
	speed_limit_sign_set_value(10); // set value
}
// ----------------------------------------------------------------------------------------------------

// -------------------------------------------------- Line Indicator --------------------------------------------------
typedef struct{
	lv_obj_t *container; // Invisible container
	lv_obj_t *lines[4]; // 4 lines
	lv_obj_t *labels[3]; // 3 num labels
}lane_indicator_t;
lane_indicator_t lane_indicator;

void lane_indicator_create(lv_obj_t *parent){
	// size parameters
	const int line_w = 10; // line width
	const int line_h = 60; // line height
	const int spacing = 40; // space between lines (center to center)
	const int total_w = line_w * 4 + spacing * 3; // total width

	// 1. create container
	lane_indicator.container = lv_obj_create(parent);
	lv_obj_remove_style_all(lane_indicator.container);
	lv_obj_set_size(lane_indicator.container, total_w, line_h);
	lv_obj_align(lane_indicator.container, LV_ALIGN_TOP_MID, 0, 0); // X centered, offset 0pix from the top
	lv_obj_set_style_bg_opa(lane_indicator.container, LV_OPA_TRANSP, LV_STATE_DEFAULT); // transparent

	// 2. create 4 lines
	for (int i = 0; i < 4; i++) {
		lane_indicator.lines[i] = lv_obj_create(lane_indicator.container);
		lv_obj_remove_style_all(lane_indicator.lines[i]);
		lv_obj_set_size(lane_indicator.lines[i], line_w, line_h);
		lv_obj_set_pos(lane_indicator.lines[i], i * (line_w + spacing), 0);
		lv_obj_set_style_bg_color(lane_indicator.lines[i], lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT); // white
		lv_obj_set_style_bg_opa(lane_indicator.lines[i], LV_OPA_COVER, LV_STATE_DEFAULT);
		lv_obj_set_style_border_width(lane_indicator.lines[i], 0, LV_STATE_DEFAULT);
	}

	// 3. create 3 number labels (between lines)
	for(int i = 0; i < 3; i++){
		lane_indicator.labels[i] = lv_label_create(lane_indicator.container);
		lv_obj_set_style_text_color(lane_indicator.labels[i], lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT); // white
		lv_obj_set_style_text_font(lane_indicator.labels[i], &lv_font_RobotoBold_50, LV_STATE_DEFAULT); // font
		lv_label_set_text_fmt(lane_indicator.labels[i], "%d", i + 1); // default number
		
		int x_pos = i * (line_w + spacing) + line_w + 5; // x position (between line i and i+1)
		lv_obj_set_pos(lane_indicator.labels[i], x_pos, 0);
	}
}
// ----------------------------------------------------------------------------------------------------

// -------------------------------------------------- Speed Display --------------------------------------------------
typedef struct{
	lv_obj_t *label_speed; // speed
	lv_obj_t *label_unit; // speed unit
}speed_display_t;
speed_display_t speed_display;

void speed_display_set_state(uint8_t state){
	if(!state){ // state0
		lv_obj_set_style_text_font(speed_display.label_speed, &lv_font_MontserratBold_150, 0);
		lv_obj_set_pos(speed_display.label_speed, 0, 120 - 35); // pos0

		lv_obj_set_pos(speed_display.label_unit, 270, 120 + 40); // pos0
	}
	else{ // state1
		lv_obj_set_style_text_font(speed_display.label_speed, &lv_font_MontserratBold_60, 0);
		lv_obj_set_pos(speed_display.label_speed, -160, 240 - 60 - 60); // pos1

		lv_obj_set_pos(speed_display.label_unit, 20, 240 - 60); // pos1
	}
}

void speed_display_create(lv_obj_t *parent){
	speed_display.label_speed = lv_label_create(parent);
	lv_obj_set_style_text_color(speed_display.label_speed, lv_color_make(0x00, 0xFF, 0x00), 0);
	lv_label_set_text(speed_display.label_speed, "123");
	lv_obj_set_width(speed_display.label_speed, 260); // text width
	lv_obj_set_style_text_align(speed_display.label_speed, LV_TEXT_ALIGN_RIGHT, 0); // right align
	// state0
	lv_obj_set_style_text_font(speed_display.label_speed, &lv_font_MontserratBold_150, 0);
	lv_obj_set_pos(speed_display.label_speed, 0, 120 - 35); // pos0
	// // state1
	// lv_obj_set_style_text_font(speed_display.label_speed, &lv_font_MontserratBold_60, 0);
	// lv_obj_set_pos(speed_display.label_speed, -160, 240 - 60 - 60); // pos1

	speed_display.label_unit = lv_label_create(parent);
	lv_obj_set_style_text_color(speed_display.label_unit, lv_color_make(0x00, 0xFF, 0x00), 0);
	lv_obj_set_style_text_font(speed_display.label_unit, &lv_font_RobotoBold_50, 0);
	lv_label_set_text(speed_display.label_unit, "kph");
	// state0
	lv_obj_set_pos(speed_display.label_unit, 270, 120 + 40); // pos0
	// // state1
	// lv_obj_set_pos(speed_display.label_unit, 20, 240 - 60); // pos1
}
// ----------------------------------------------------------------------------------------------------

// --------------------------------------------------  --------------------------------------------------
// ----------------------------------------------------------------------------------------------------

static lv_color_t lvgl_buf[DISPLAY_W * DISPLAY_H];
extern void lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);

void lvgl_init(void){ // LVGL buf and driver init
	lv_init();

	// init buffer
	static lv_disp_draw_buf_t draw_buf;
	lv_disp_draw_buf_init(&draw_buf, lvgl_buf, NULL, DISPLAY_W * DISPLAY_H);

	// init display driver
	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.draw_buf = &draw_buf;
	disp_drv.hor_res = DISPLAY_W;
	disp_drv.ver_res = DISPLAY_H;
	disp_drv.flush_cb = lvgl_flush_cb; // screen flush callback
	disp_drv.full_refresh = 1; // flush all display area everytime
	lv_disp_drv_register(&disp_drv);
}

void lvgl_ui_init(void){
	// get active screen
	lv_obj_t *scr = lv_scr_act();

	// ensure screen limits the display area
	lv_obj_add_flag(scr, LV_OBJ_FLAG_SCROLL_ON_FOCUS); // disable scrolling to prevent out of bounds auto scrolling
	lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE); // turn off the screen's scrollbar attribute

	// black background
	lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

	// 1. Create a background color block (Container/Base Object)
	lv_obj_t *bg_card = lv_obj_create(scr); // create on the current active screen
	lv_obj_set_size(bg_card, 100, 100); // size
	lv_obj_set_pos(bg_card, DISPLAY_W - 100, 0);
	lv_obj_set_style_bg_color(bg_card, lv_palette_main(LV_PALETTE_NONE), 0); // blue background
	lv_obj_set_style_radius(bg_card, 1, 0); // rounded corners 1px

	lv_obj_t *label_meter = lv_label_create(scr);
	lv_obj_set_style_text_color(label_meter, lv_color_make(0x00, 0xFF, 0xFF), 0);
	lv_obj_set_style_text_font(label_meter, &lv_font_RobotoBold_50, 0);
	lv_obj_set_pos(label_meter, DISPLAY_W - 140, 100);
	lv_label_set_text(label_meter, "2333m");
	lv_obj_set_style_text_letter_space(label_meter, -5, LV_PART_MAIN); // neg spacing
	lv_obj_set_width(label_meter, 140); // text width
	lv_obj_set_style_text_align(label_meter, LV_TEXT_ALIGN_RIGHT, 0); // right align

	// create speed display
	speed_display_create(scr);

	// create speed limit sign in the upper right corner
	speed_limit_sign_create(scr, 0, 0);

	// create lane indicator at the top center
	lane_indicator_create(scr);
}

void lvgl_task(void *pvParameters){ // LVGL FreeRTOS task
	init_rgb(); // init RGB interface

	lvgl_init(); // LVGL init
	lvgl_ui_init(); // UI init

	SET_DISP(1); // Enable screen
	SET_PWM_LIGHT(100); // Set backlight PWM

	while(1){
		uint32_t time_till_next = lv_timer_handler(); // Let LVGL handle rendering and timers
		if(time_till_next > 30) time_till_next = 30; // MAX
		else if(time_till_next < 5) time_till_next = 5; // MIN
		vTaskDelay(pdMS_TO_TICKS(time_till_next)); // wait for next rendering

		static uint8_t key_old = 0; // key old state
		static uint8_t cnt = 0;
		if(key_old != GET_KEY()){ // edge
			key_old = !key_old;
			if(key_old){ // press edge
				SET_PWM_LIGHT((cnt % 4 + 1) * 500);
				printf("gpio_get_level-0\n");
				speed_limit_sign_set_value(cnt*10);
				// speed_limit_sign_set_visible(cnt % 2);
				speed_display_set_state(cnt % 2);
				// lv_obj_t *bg_card = lv_obj_create(lv_scr_act());
				// lv_obj_set_size(bg_card, 20, 20);
				// lv_obj_set_pos(bg_card, cnt*20, cnt*20);
				// lv_obj_set_style_bg_color(bg_card, lv_palette_main(LV_PALETTE_BLUE), 0);
				// lv_obj_set_style_radius(bg_card, 10, 0);
				cnt++;
			}
		}
	}
}





