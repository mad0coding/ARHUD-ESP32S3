#ifndef _LCDRGB_H
#define _LCDRGB_H

#include "BasicIO.h"


// 屏幕尺寸
#define LCD_H_RES				800
#define LCD_V_RES				480
#define LCD_FRAME_SIZE			(LCD_H_RES * LCD_V_RES * 1)

// RGB 流控引脚
#define LCD_PIN_PCLK			GPIO_NUM_19
#define LCD_PIN_DE				GPIO_NUM_13
#define LCD_PIN_VSYNC			GPIO_NUM_46
#define LCD_PIN_HSYNC			GPIO_NUM_3

// RGB 数据引脚 (8位或16位)
#define LCD_PIN_DATA0			GPIO_NUM_18
#define LCD_PIN_DATA1			GPIO_NUM_8
#define LCD_PIN_DATA2			GPIO_NUM_15
#define LCD_PIN_DATA3			GPIO_NUM_16
#define LCD_PIN_DATA4			GPIO_NUM_17
#define LCD_PIN_DATA5			GPIO_NUM_5
#define LCD_PIN_DATA6			GPIO_NUM_6
#define LCD_PIN_DATA7			GPIO_NUM_7



void rgb_main(void);



#endif



