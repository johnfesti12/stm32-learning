/*
 * bsp_led.h
 *
 *  Created on: 11 окт. 2026 г.
 *      Author: Dell
 */

#ifndef INC_BSP_LED_H_
#define INC_BSP_LED_H_

#include "stm32f4xx_hal.h"

/*Публичные идентификаторы светодиодов */
typedef enum {
	LED_GREEN = 0, /* LD2, PA5, встроенный */
	LED_EXT, /* PA6, внешний */
	LED_BLUE = 2
} BSP_LED_t;

/* Инициализация всех светодиодов */
void MyBSP_LED_Init(void);

/* Включить светодиод */
void MyBSP_LED_On(BSP_LED_t led);

/* Выключить светодиод */
void MyBSP_LED_Off(BSP_LED_t led);

/* Переключить светодиод */
void MyBSP_LED_Toggle(BSP_LED_t led);

/* Мигание светодиодом */
void MyBSP_LED_Blink(BSP_LED_t led, uint32_t period_ms);

#endif /* INC_BSP_LED_H_ */
