#ifndef _LCDRGB_H
#define _LCDRGB_H

#include "BasicIO.h"

#include "driver/ledc.h"

// Display size
#define DISPLAY_W				400
#define DISPLAY_H				240
#define OFFSET_X				200
#define OFFSET_Y				120

// Mirror config
#define DISPLAY_MIRROR_X		1
#define DISPLAY_MIRROR_Y		0

// Screen size
#define LCD_H_RES				800
#define LCD_V_RES				480
#define LCD_FRAME_SIZE			(LCD_H_RES * LCD_V_RES * 1)

// RGB flow control pins
#define LCD_PIN_PCLK			GPIO_NUM_19
#define LCD_PIN_DE				GPIO_NUM_13
#define LCD_PIN_VSYNC			GPIO_NUM_46
#define LCD_PIN_HSYNC			GPIO_NUM_3

// RGB data pins (8-bit or 16-bit)
#define LCD_PIN_DATA0			GPIO_NUM_18
#define LCD_PIN_DATA1			GPIO_NUM_8
#define LCD_PIN_DATA2			GPIO_NUM_15
#define LCD_PIN_DATA3			GPIO_NUM_16
#define LCD_PIN_DATA4			GPIO_NUM_17
#define LCD_PIN_DATA5			GPIO_NUM_5
#define LCD_PIN_DATA6			GPIO_NUM_6
#define LCD_PIN_DATA7			GPIO_NUM_7

// Color Definition
#define COLOR_K			0x00 // Black
#define COLOR_A			0x6E // Gray
#define COLOR_W			0xFF // White
#define COLOR_R			0xE0 // Red
#define COLOR_G			0x1C // Green
#define COLOR_B			0x03 // Blue
#define COLOR_Y			0xF8 // Yellow
#define COLOR_C			0x1F // Cyan
#define COLOR_M			0xE3 // Magenta


void init_rgb(void);
void rgb_test(void);


#endif



