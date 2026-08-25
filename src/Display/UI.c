#include "UI.h"

#include "freertos/FreeRTOS.h"
#include "lvgl.h"

#include "LcdRgb.h"
#include "Comm.h"
#include "OBD.h"

static const char *TAG = "UI";

// custom font
LV_FONT_DECLARE(lv_font_MontserratBold_150);
LV_FONT_DECLARE(lv_font_MontserratBold_140);
LV_FONT_DECLARE(lv_font_MontserratBold_130);
LV_FONT_DECLARE(lv_font_MontserratBold_120);
LV_FONT_DECLARE(lv_font_MontserratBold_110);
LV_FONT_DECLARE(lv_font_MontserratBold_100);
LV_FONT_DECLARE(lv_font_MontserratBold_90);
LV_FONT_DECLARE(lv_font_MontserratBold_80);
LV_FONT_DECLARE(lv_font_MontserratBold_70);
LV_FONT_DECLARE(lv_font_MontserratBold_60);
LV_FONT_DECLARE(lv_font_RobotoBold_50);

static const void *font_list[] = {
	&lv_font_MontserratBold_150, &lv_font_MontserratBold_140, &lv_font_MontserratBold_130, &lv_font_MontserratBold_120,
	&lv_font_MontserratBold_110, &lv_font_MontserratBold_100, &lv_font_MontserratBold_90, &lv_font_MontserratBold_80,
	&lv_font_MontserratBold_70, &lv_font_MontserratBold_60,
	&lv_font_RobotoBold_50,
};

// custom img
LV_IMG_DECLARE(Navigation_Arrow);
LV_IMG_DECLARE(ic_depart);
LV_IMG_DECLARE(ic_destination);
LV_IMG_DECLARE(ic_destination_left);
LV_IMG_DECLARE(ic_destination_right);
LV_IMG_DECLARE(ic_merge);
LV_IMG_DECLARE(ic_straight);
LV_IMG_DECLARE(ic_turn_left);
LV_IMG_DECLARE(ic_turn_right);
LV_IMG_DECLARE(ic_turn_sharp_left);
LV_IMG_DECLARE(ic_turn_sharp_right);
LV_IMG_DECLARE(ic_turn_slight_left);
LV_IMG_DECLARE(ic_turn_slight_right);
LV_IMG_DECLARE(ic_turn_u_turn_clockwise);
LV_IMG_DECLARE(ic_turn_u_turn_counterclockwise);

static const void *icon_list[] = {
	NULL, &ic_turn_u_turn_counterclockwise, &ic_turn_sharp_left, &ic_turn_left, &ic_turn_slight_left,
	&ic_straight, &ic_turn_slight_right, &ic_turn_right, &ic_turn_sharp_right, &ic_turn_u_turn_clockwise,
	&ic_destination_left, &ic_destination_right, &ic_destination, &ic_depart, &ic_merge,
};

// -------------------------------------------------- Navigation Arrow --------------------------------------------------
#define ARROW_ANIM_TIME		150 // animation time (ms)

typedef struct{
	lv_obj_t *circle;
	lv_obj_t *arrow;

	uint8_t visible;
	bool is_animating; // is animation running
	lv_anim_t anim; // animation handle
}navigation_arrow_t;
navigation_arrow_t navigation_arrow;

static void navigation_arrow_set_angle(int16_t angle){
	lv_img_set_angle(navigation_arrow.arrow, angle); // unit: 0.1 deg
}

static void navigation_arrow_set_visible(uint8_t visible){
	if(visible) lv_obj_clear_flag(navigation_arrow.circle, LV_OBJ_FLAG_HIDDEN);
	else lv_obj_add_flag(navigation_arrow.circle, LV_OBJ_FLAG_HIDDEN);
}

static void navigation_arrow_set_visible_anim(uint8_t visible){ // set state with animation
	if(navigation_arrow.visible == visible || navigation_arrow.is_animating){
		return;
	}
	navigation_arrow.is_animating = true; // animation running

	if(visible) navigation_arrow_set_visible(1); // set visible before animation

	navigation_arrow.anim.user_data = (void *)(uintptr_t)visible; // record

	lv_anim_start(&navigation_arrow.anim); // start
}

static void arrow_anim_exec_cb(void *var, int32_t v){ // animation execution callback
	// v: 0 -> 255 = animation progress: 0.0% -> 100.0%
	int16_t progress = (navigation_arrow.visible == 0) ? v : (255 - v); // visible progress (0 -> 255 = invisible -> visible)
	lv_obj_set_style_opa(navigation_arrow.circle, progress, LV_PART_MAIN); // set opacity
}

static void arrow_anim_ready_cb(lv_anim_t *a){ // animation finished callback
	navigation_arrow.is_animating = false;
	navigation_arrow.visible = (uint8_t)(uintptr_t)a->user_data;
	if(!navigation_arrow.visible) navigation_arrow_set_visible(0); // set invisible to save cpu
}

static void navigation_arrow_create(lv_obj_t *parent){
	navigation_arrow.circle = lv_obj_create(parent);
	lv_obj_set_size(navigation_arrow.circle, 171, 171); // size
	lv_obj_set_pos(navigation_arrow.circle, DISPLAY_W/2 - 86, DISPLAY_H - 171);
	lv_obj_set_style_bg_color(navigation_arrow.circle, lv_palette_main(LV_PALETTE_NONE), 0); // blue background
	lv_obj_set_style_radius(navigation_arrow.circle, 86, 0); // rounded corners 1px
	lv_obj_add_flag(navigation_arrow.circle, LV_OBJ_FLAG_SCROLL_ON_FOCUS); // disable scrolling
	lv_obj_clear_flag(navigation_arrow.circle, LV_OBJ_FLAG_SCROLLABLE); // turn off scrollbar attribute

	navigation_arrow.arrow = lv_img_create(navigation_arrow.circle); // create img
	lv_img_set_src(navigation_arrow.arrow, &Navigation_Arrow); // set img source
	lv_obj_center(navigation_arrow.arrow); // center
	lv_obj_set_style_img_recolor(navigation_arrow.arrow, lv_color_make(0x00, 0x40, 0xFF), LV_PART_MAIN); // set color
	lv_obj_set_style_img_recolor_opa(navigation_arrow.arrow, LV_OPA_COVER, LV_PART_MAIN); // set opacity

	navigation_arrow_set_visible(0); // hide

	// create animation
	lv_anim_init(&navigation_arrow.anim);
	lv_anim_set_var(&navigation_arrow.anim, &navigation_arrow);
	lv_anim_set_values(&navigation_arrow.anim, 0, 255);
	lv_anim_set_time(&navigation_arrow.anim, ARROW_ANIM_TIME);
	lv_anim_set_exec_cb(&navigation_arrow.anim, arrow_anim_exec_cb);
	lv_anim_set_ready_cb(&navigation_arrow.anim, arrow_anim_ready_cb);
	lv_anim_set_path_cb(&navigation_arrow.anim, lv_anim_path_ease_in_out);
	navigation_arrow.is_animating = false;
}
// ----------------------------------------------------------------------------------------------------

// -------------------------------------------------- Speed Limit Sign --------------------------------------------------
typedef struct{
	lv_obj_t *out_circle; // Outer circle (as the parent object)
	lv_obj_t *in_circle; // Inner circle
	lv_obj_t *label; // Number label
}speed_limit_sign_t;
speed_limit_sign_t speed_limit_sign;

static void speed_limit_sign_set_visible(uint8_t visible){
	if(visible) lv_obj_clear_flag(speed_limit_sign.out_circle, LV_OBJ_FLAG_HIDDEN);
	else lv_obj_add_flag(speed_limit_sign.out_circle, LV_OBJ_FLAG_HIDDEN);
}

static void speed_limit_sign_set_value(int16_t value){
	lv_label_set_text_fmt(speed_limit_sign.label, "%d", value); // set value
	if(value <= 0){
		speed_limit_sign_set_visible(0); // hide
		return;
	}
	speed_limit_sign_set_visible(1); // show
	if(value < 100) lv_obj_set_style_text_letter_space(speed_limit_sign.label, 0, LV_PART_MAIN);
	else lv_obj_set_style_text_letter_space(speed_limit_sign.label, -5, LV_PART_MAIN); // neg spacing for 3-digit num
	lv_obj_align(speed_limit_sign.label, LV_ALIGN_CENTER, 0, 2); // centered with Y offset 0pix
}

static void speed_limit_sign_create(lv_obj_t *parent){
	// 1. Outer circle (as the parent object)
	speed_limit_sign.out_circle = lv_obj_create(parent);
	lv_obj_remove_style_all(speed_limit_sign.out_circle);
	lv_obj_set_size(speed_limit_sign.out_circle, 100, 100); // diameter
	lv_obj_set_pos(speed_limit_sign.out_circle, 0, 0); // upper right corner
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

	speed_limit_sign_set_value(-1); // hide
}
// ----------------------------------------------------------------------------------------------------

// -------------------------------------------------- Line Indicator --------------------------------------------------
typedef struct{
	lv_obj_t *container; // Invisible container
	lv_obj_t *lines[4]; // 4 lines
	lv_obj_t *labels[3]; // 3 num labels
}lane_indicator_t;
lane_indicator_t lane_indicator;

static void lane_indicator_set_data(uint8_t *data){
	uint8_t label_num[3] = {data[0] & 0x3F, data[1] & 0x3F, data[2] & 0x3F};
	uint8_t label_state[3] = {(data[0] & 0xC0) >> 6, (data[1] & 0xC0) >> 6, (data[2] & 0xC0) >> 6};
	uint8_t line_visible[4] = {
		label_state[0], label_state[0] | label_state[1],
		label_state[1] | label_state[2], label_state[2],
	};
	for(uint8_t i = 0; i < 4; i++){
		if(line_visible[i]) lv_obj_clear_flag(lane_indicator.lines[i], LV_OBJ_FLAG_HIDDEN);
		else lv_obj_add_flag(lane_indicator.lines[i], LV_OBJ_FLAG_HIDDEN);
	}
	for(uint8_t i = 0; i < 3; i++){
		if(label_state[i]) lv_obj_clear_flag(lane_indicator.labels[i], LV_OBJ_FLAG_HIDDEN);
		else lv_obj_add_flag(lane_indicator.labels[i], LV_OBJ_FLAG_HIDDEN);
		if(label_state[i] == 1){
			lv_obj_set_style_text_color(lane_indicator.labels[i], lv_color_hex(0x00FF00), LV_STATE_DEFAULT); // green
		}else if(label_state[i] == 2){
			lv_obj_set_style_text_color(lane_indicator.labels[i], lv_color_hex(0xFF0000), LV_STATE_DEFAULT); // red
		}else{
			lv_obj_set_style_text_color(lane_indicator.labels[i], lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT); // white
		}
		lv_label_set_text_fmt(lane_indicator.labels[i], "%d", label_num[i]); // set number
	}
}

static void lane_indicator_create(lv_obj_t *parent){
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
	for(int i = 0; i < 4; i++){
		lane_indicator.lines[i] = lv_obj_create(lane_indicator.container);
		lv_obj_remove_style_all(lane_indicator.lines[i]);
		lv_obj_set_size(lane_indicator.lines[i], line_w, line_h);
		lv_obj_set_pos(lane_indicator.lines[i], i * (line_w + spacing), 0);
		lv_obj_set_style_bg_opa(lane_indicator.lines[i], LV_OPA_COVER, LV_STATE_DEFAULT);
		lv_obj_set_style_border_width(lane_indicator.lines[i], 0, LV_STATE_DEFAULT);
	}

	// 3. create 3 number labels (between lines)
	for(int i = 0; i < 3; i++){
		lane_indicator.labels[i] = lv_label_create(lane_indicator.container);
		lv_obj_set_style_text_font(lane_indicator.labels[i], &lv_font_RobotoBold_50, LV_STATE_DEFAULT); // font
		lv_label_set_text_fmt(lane_indicator.labels[i], "%d", i + 1); // default number
		
		int x_pos = i * (line_w + spacing) + line_w + 5; // x position (between line i and i+1)
		lv_obj_set_pos(lane_indicator.labels[i], x_pos, 0);
	}

	uint8_t data[3] = {0, 0, 0};
	lane_indicator_set_data(data); // hide all
}
// ----------------------------------------------------------------------------------------------------

// -------------------------------------------------- Speed Display --------------------------------------------------
// parameters
#define SPEED_ANIM_TIME		150 // animation time (ms)

#define STATE0_SPEED_X		(-40 + 10)
#define STATE0_SPEED_Y		(120 - 35)
#define STATE0_UNIT_X		(270 + 5)
#define STATE0_UNIT_Y		(120 + 40)
#define STATE1_SPEED_X		(-200 + 11)
#define STATE1_SPEED_Y		(240 - 60 - 60)
#define STATE1_UNIT_X		(20 + 5)
#define STATE1_UNIT_Y		(240 - 60)

typedef struct{
	lv_obj_t *label_speed; // speed value label
	lv_obj_t *label_unit; // speed unit label
	
	uint8_t state; // 0: Large size, 1: Small size
	bool is_animating; // is animation running
	lv_anim_t anim; // animation handle
} speed_display_t;
static speed_display_t speed_display;

static void speed_display_set_value(int16_t value){ // set value
	if(value < 0) lv_label_set_text(speed_display.label_speed, "---");
	else lv_label_set_text_fmt(speed_display.label_speed, "%d", value);
}

static void speed_display_set_state(uint8_t state){ // set state without animation
	speed_display.state = state;
	if(!state){ // state0
		lv_obj_set_style_text_font(speed_display.label_speed, &lv_font_MontserratBold_150, 0);
		lv_obj_set_pos(speed_display.label_speed, STATE0_SPEED_X, STATE0_SPEED_Y);

		lv_obj_set_pos(speed_display.label_unit, STATE0_UNIT_X, STATE0_UNIT_Y);
	}
	else{ // state1
		lv_obj_set_style_text_font(speed_display.label_speed, &lv_font_MontserratBold_60, 0);
		lv_obj_set_pos(speed_display.label_speed, STATE1_SPEED_X, STATE1_SPEED_Y);

		lv_obj_set_pos(speed_display.label_unit, STATE1_UNIT_X, STATE1_UNIT_Y);
	}
}

static void speed_display_set_state_anim(uint8_t target_state){ // set state with animation
	if(speed_display.state == target_state || speed_display.is_animating){
		return;
	}
	speed_display.is_animating = true; // animation running

	speed_display.anim.user_data = (void *)(uintptr_t)target_state; // record target state

	lv_anim_start(&speed_display.anim); // start

	// navigation_arrow_set_visible(target_state);
	navigation_arrow_set_visible_anim(target_state);
}

static void speed_anim_exec_cb(void *var, int32_t v){ // animation execution callback
	// v: 0 -> 1000 = animation progress: 0.0% -> 100.0%
	int16_t progress = (speed_display.state == 0) ? v : (1000 - v); // state progress (0 -> 1000 = state0 -> state1)

	// speed label font switching (10 stages)
	uint8_t font_idx = (progress >= 1000) ? 9 : (progress / 100); // range: [0, 9]
	lv_obj_set_style_text_font(speed_display.label_speed, font_list[font_idx], 0); // set font

	// speed label movement
	int32_t speed_x = STATE0_SPEED_X + (STATE1_SPEED_X - STATE0_SPEED_X) * progress / 1000;
	int32_t speed_y = STATE0_SPEED_Y + (STATE1_SPEED_Y - STATE0_SPEED_Y) * progress / 1000;
	lv_obj_set_pos(speed_display.label_speed, speed_x, speed_y);

	// unit label movement
	int32_t unit_x = STATE0_UNIT_X + (STATE1_UNIT_X - STATE0_UNIT_X) * progress / 1000;
	int32_t unit_y = STATE0_UNIT_Y + (STATE1_UNIT_Y - STATE0_UNIT_Y) * progress / 1000;
	lv_obj_set_pos(speed_display.label_unit, unit_x, unit_y);
}

static void speed_anim_ready_cb(lv_anim_t *a){ // animation finished callback
	speed_display.is_animating = false;
	speed_display_set_state((uint8_t)(uintptr_t)a->user_data); // get recorded target state and set it
}

static void speed_display_create(lv_obj_t *parent){
	// create label_speed
	speed_display.label_speed = lv_label_create(parent);
	lv_obj_set_style_text_color(speed_display.label_speed, lv_color_make(0x00, 0xFF, 0x00), 0);
	lv_obj_set_width(speed_display.label_speed, 300);
	lv_obj_set_style_text_align(speed_display.label_speed, LV_TEXT_ALIGN_RIGHT, 0);

	// create label_unit
	speed_display.label_unit = lv_label_create(parent);
	lv_obj_set_style_text_color(speed_display.label_unit, lv_color_make(0x00, 0xFF, 0x00), 0);
	lv_obj_set_style_text_font(speed_display.label_unit, &lv_font_RobotoBold_50, 0);
	lv_label_set_text(speed_display.label_unit, "kph");

	speed_display_set_value(-1); // defalut value for label_speed
	speed_display_set_state(0); // defalut state0

	// create animation
	lv_anim_init(&speed_display.anim);
	lv_anim_set_var(&speed_display.anim, &speed_display);
	lv_anim_set_values(&speed_display.anim, 0, 1000);
	lv_anim_set_time(&speed_display.anim, SPEED_ANIM_TIME);
	lv_anim_set_exec_cb(&speed_display.anim, speed_anim_exec_cb);
	lv_anim_set_ready_cb(&speed_display.anim, speed_anim_ready_cb);
	lv_anim_set_path_cb(&speed_display.anim, lv_anim_path_ease_in_out);
	speed_display.is_animating = false;
}
// ----------------------------------------------------------------------------------------------------

// -------------------------------------------------- Navigation Sign --------------------------------------------------
typedef struct{
	lv_obj_t *block;
	lv_obj_t *sign;
	lv_obj_t *dist;
}navigation_sign_t;
static navigation_sign_t navigation_sign;

static void navigation_sign_set_visible(uint8_t visible){
	if(visible){
		lv_obj_clear_flag(navigation_sign.block, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(navigation_sign.dist, LV_OBJ_FLAG_HIDDEN);
	}
	else{
		lv_obj_add_flag(navigation_sign.block, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(navigation_sign.dist, LV_OBJ_FLAG_HIDDEN);
	}
}

static void navigation_sign_set_sign(uint8_t sign){
	if(!sign || sign >= (sizeof(icon_list) / sizeof(icon_list[0]))){ // not in range
		navigation_sign_set_visible(0); // hide
	}
	else{
		navigation_sign_set_visible(1); // show
		lv_img_set_src(navigation_sign.sign, icon_list[sign]); // set img source
	}
}

static void navigation_sign_set_dist(uint16_t dist){ // set distance data
	if(!(dist & 0x8000)) lv_label_set_text_fmt(navigation_sign.dist, "%dm", dist);
	else lv_label_set_text_fmt(navigation_sign.dist, "%dkm", (dist & 0x7FFF));
}

static void navigation_sign_create(lv_obj_t *parent){
	navigation_sign.block = lv_obj_create(parent);
	lv_obj_set_size(navigation_sign.block, 100, 100); // size
	lv_obj_set_pos(navigation_sign.block, DISPLAY_W - 100, 0);
	lv_obj_set_style_bg_color(navigation_sign.block, lv_palette_main(LV_PALETTE_NONE), 0); // background
	lv_obj_set_style_radius(navigation_sign.block, 1, 0); // rounded corners 1px
	lv_obj_set_style_border_color(navigation_sign.block, lv_color_hex(0x00FFFF), LV_PART_MAIN);
	lv_obj_add_flag(navigation_sign.block, LV_OBJ_FLAG_SCROLL_ON_FOCUS); // disable scrolling
	lv_obj_clear_flag(navigation_sign.block, LV_OBJ_FLAG_SCROLLABLE); // turn off scrollbar attribute

	navigation_sign.sign = lv_img_create(navigation_sign.block); // create img
	lv_obj_center(navigation_sign.sign); // center
	lv_obj_set_style_img_recolor(navigation_sign.sign, lv_color_hex(0x00FFFF), LV_PART_MAIN); // set color
	lv_obj_set_style_img_recolor_opa(navigation_sign.sign, LV_OPA_COVER, LV_PART_MAIN); // set opacity

	navigation_sign.dist = lv_label_create(parent);
	lv_obj_set_style_text_color(navigation_sign.dist, lv_color_hex(0x00FFFF), 0);
	lv_obj_set_style_text_font(navigation_sign.dist, &lv_font_RobotoBold_50, 0);
	lv_obj_set_pos(navigation_sign.dist, DISPLAY_W - 140, 100);
	lv_obj_set_style_text_letter_space(navigation_sign.dist, -5, LV_PART_MAIN); // neg spacing
	lv_obj_set_style_text_line_space(navigation_sign.dist, -20, LV_PART_MAIN);
	lv_obj_set_width(navigation_sign.dist, 140); // text width
	lv_obj_set_style_text_align(navigation_sign.dist, LV_TEXT_ALIGN_RIGHT, 0); // right align

	navigation_sign_set_dist(0); // set distance data
	navigation_sign_set_sign(0); // hide
}
// ----------------------------------------------------------------------------------------------------

static lv_color_t lvgl_buf[DISPLAY_W * DISPLAY_H];
extern void lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);

static void lvgl_init(void){ // LVGL buf and driver init
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

static void lvgl_ui_init(void){
	// get current active screen
	lv_obj_t *scr = lv_scr_act();

	// ensure screen limits the display area
	lv_obj_add_flag(scr, LV_OBJ_FLAG_SCROLL_ON_FOCUS); // disable scrolling to prevent out of bounds auto scrolling
	lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE); // turn off the screen's scrollbar attribute

	// black background
	lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

	navigation_arrow_create(scr); // create navigation arrow

	navigation_sign_create(scr); // create navigation sign

	speed_display_create(scr); // create speed display

	speed_limit_sign_create(scr); // create speed limit sign in the upper right corner

	lane_indicator_create(scr); // create lane indicator at the top center
}

typedef struct{
	uint8_t valid;
	uint8_t speed_app;
	uint16_t arrow_angle;
	uint8_t sign_idx;
	uint16_t sign_dist;
	uint8_t speed_limit_app;
	uint8_t line_data[3];
}ble_packet_t;
static ble_packet_t ble_packet;

static void ui_use_data(void){
	const uint32_t ble_lost_timeout = 1000 * 1000; // 1s
	static uint32_t ble_last_time = -ble_lost_timeout;
	uint8_t data[BLE_MAX_RAW_DATA_LEN];
	uint8_t len = read_ble_in_buf(data); // read from ring buf
	if(len && data[0] == 1){ // valid packet
		// ESP_LOGI(TAG, "Received %d bytes:", len);
		// ESP_LOG_BUFFER_HEX(TAG, data, len);
		ble_last_time = GET_US(); // update time
		ble_packet.valid = 1; // ble data valid
		ble_packet.speed_app = data[1];
		ble_packet.arrow_angle = BIG_ENDIAN_16(data + 2);
		ble_packet.sign_idx = data[4];
		ble_packet.sign_dist = BIG_ENDIAN_16(data + 5);
		ble_packet.speed_limit_app = data[7];
		ble_packet.line_data[0] = data[8];
		ble_packet.line_data[1] = data[9];
		ble_packet.line_data[2] = data[10];
	}
	else if(ble_packet.valid){ // no valid packet and state is still valid
		uint32_t ble_lost_time = ((uint32_t)GET_US() - ble_last_time);
		if(ble_lost_time > ble_lost_timeout){ // timeout
			ESP_LOGW(TAG, "BLE connection lost.");
			// ble_packet defalut set
			memset(&ble_packet, 0, sizeof(ble_packet)); // ble data not valid
			ble_packet.speed_app = ble_packet.speed_limit_app = 255;
			ble_packet.arrow_angle = 0xFFFF;
		}
	}

	// speed
	uint8_t prior_speed = 0, prior_speed_limit = 0; // 0-local(OBD/CV) 1-BLE
	if(ble_packet.speed_app != 255 && 
		(prior_speed || !obd_valid)){ // use BLE speed
		speed_display_set_value(ble_packet.speed_app);
	}
	else if(obd_valid){ // use OBD speed
		speed_display_set_value(obd_speed);
	}
	else{ // no valid speed
		speed_display_set_value(-1);
	}

	// arrow angle
	uint8_t arrow_angle_valid = (ble_packet.arrow_angle <= 3600);
	if(arrow_angle_valid) navigation_arrow_set_angle(ble_packet.arrow_angle);
	// speed display state
	speed_display_set_state_anim(arrow_angle_valid);

	// navigation sign
	navigation_sign_set_sign(ble_packet.sign_idx);
	navigation_sign_set_dist(ble_packet.sign_dist);

	// speed limit
	if(ble_packet.speed_limit_app != 255 &&
		(prior_speed_limit || 1)){ // use BLE speed limit
		speed_limit_sign_set_value(ble_packet.speed_limit_app);
	}
	else if(!prior_speed_limit && 0){ // use CV speed limit
		speed_limit_sign_set_value(-1);
	}
	else{ // no valid speed limit
		speed_limit_sign_set_value(-1);
	}

	// line data
	lane_indicator_set_data(ble_packet.line_data);
}

void lvgl_task(void *pvParameters){ // LVGL FreeRTOS task
	SET_DISP(1); // enable screen first so it can receive RGB data before turning on backlight
	init_rgb(); // init RGB interface

	lvgl_init(); // LVGL init
	lvgl_ui_init(); // UI init

	vTaskDelay(pdMS_TO_TICKS(150)); // wait for the screen to be stable (ready)
	SET_PWM_LIGHT(100); // set backlight PWM

	ble_packet.valid = 1; // to trig ble_packet defalut set

	while(1){
		ui_use_data();

		uint32_t time_till_next = lv_timer_handler(); // Let LVGL handle rendering and timers
		// if(time_till_next < (uint32_t)-1) printf("time_till_next: %lu\n", time_till_next);
		if(time_till_next > 30) time_till_next = 30; // MAX
		else if(time_till_next < 5) time_till_next = 5; // MIN
		vTaskDelay(pdMS_TO_TICKS(time_till_next)); // wait for next rendering

		auto_backlight(128);

		// navigation_arrow_set_angle(lv_tick_get() % 3600);
		// speed_display_set_value(lv_tick_get() / 500 % 201);

		static uint8_t key_old = 0; // key old state
		static uint8_t cnt = 0;
		if(key_old != GET_KEY()){ // edge
			key_old = !key_old;
			if(key_old){ // press edge
				SET_PWM_LIGHT((cnt % 4 + 1) * 500);
				printf("KEY pressed.\n");
				// speed_limit_sign_set_value(cnt*10);
				// speed_limit_sign_set_visible(cnt % 2);
				// speed_display_set_state(cnt % 2);
				// speed_display_set_state_anim(cnt % 2);
				// navigation_sign_set_sign(cnt % 15);

				// uint8_t data[3] = {0x47, 0x88, 0xC9};
				// lane_indicator_set_data(data);
				cnt++;
			}
		}
	}
}





