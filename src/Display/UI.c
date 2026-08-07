#include "UI.h"

#include "freertos/FreeRTOS.h"
#include "lvgl.h"

#include "LcdRgb.h"

// custom font
LV_FONT_DECLARE(lv_font_MontserratBold_100);
LV_FONT_DECLARE(lv_font_RobotoBold_50);


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

	// black background
	lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

	// // 1. Create a background color block (Container/Base Object)
	// lv_obj_t *bg_card = lv_obj_create(scr); // create on the current active screen
	// lv_obj_set_size(bg_card, 300, 100); // 200px x 100px
	// // lv_obj_align(bg_card, LV_ALIGN_CENTER, 0, 0); // centered display within the display area
	// lv_obj_set_pos(bg_card, 0, /*DISPLAY_H - 48-10*/DISPLAY_H - 100);
	// lv_obj_set_style_bg_color(bg_card, lv_palette_main(LV_PALETTE_BLUE), 0); // blue background
	// lv_obj_set_style_radius(bg_card, 10, 0); // rounded corners 10px

	// 2. Create a text label (Label) within the color block
	lv_obj_t *label = lv_label_create(scr); // parent node: the color block above
	lv_label_set_text(label, "Hello LVGL!\n"/*123 ABC"*/); // set text
	// lv_obj_set_style_text_color(label, lv_color_white(), 0); // color: white
	lv_obj_set_style_text_color(label, lv_color_make(0xFF, 0x00, 0x00), 0); // color: white
	lv_obj_center(label); // centered within the parent color block
	// lv_obj_set_style_transform_zoom(label, 512, 0);
	lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);


	lv_obj_t *label_meter = lv_label_create(scr);
	lv_obj_set_style_text_color(label_meter, lv_color_make(0xFF, 0xFF, 0xFF), 0);
	lv_obj_set_style_text_font(label_meter, &lv_font_RobotoBold_50, 0);
	lv_obj_set_pos(label_meter, 250, /*DISPLAY_H - 48-10*/0);
	lv_label_set_text(label_meter, "20 m");


	lv_obj_t *label_speed = lv_label_create(scr);
	lv_obj_set_style_text_color(label_speed, lv_color_make(0x00, 0xFF, 0x00), 0);
	lv_obj_set_style_text_font(label_speed, &lv_font_MontserratBold_100, 0);
	lv_obj_set_pos(label_speed, 0, /*DISPLAY_H - 48-10*/DISPLAY_H - 110);
	lv_label_set_text(label_speed, "123 kph");
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





