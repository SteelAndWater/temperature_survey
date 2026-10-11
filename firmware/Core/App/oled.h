#ifndef __OLED_H
#define __OLED_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

/* 通用区：STM32F1 HAL + 128x64 SSD1306 兼容 I2C OLED。 */
void OLED_Init(I2C_HandleTypeDef *i2c, uint8_t address_7bit);
void OLED_Clear(void);
void OLED_Update(void);
void OLED_ShowString(uint8_t x, uint8_t y_page, const char *text);
void OLED_Show8x16Char(uint8_t x, uint8_t y_page, const uint8_t *data);
void OLED_Show16x16Char(uint8_t x, uint8_t y_page, const uint8_t *data);
extern volatile uint32_t oled_i2c_errors;

/* ===== 项目专属扩展区：换项目时修改这里的标签声明。===== */
typedef enum {
    OLED_LABEL_NORMAL,
    OLED_LABEL_ALARM,
    OLED_LABEL_TEMP_OUTPUT,
    OLED_LABEL_LIGHT_OUTPUT
} OLED_Label;

void OLED_ShowLabel(uint8_t x, uint8_t y_page, OLED_Label label);
void OLED_ShowSurveyScreen(uint8_t alarm_test, uint8_t temp_output,
                           float light_voltage);

#endif
