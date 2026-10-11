/*
 * bsp_led.c
 *
 *  Created on: 11 окт. 2026 г.
 *      Author: Dell
 */

#include "bsp_led.h"

/* Приватная таблица: какой пин соотвествует какому светодиоду */
static GPIO_TypeDef * const led_port[] = {
		[LED_GREEN] = GPIOA, /* LD2 */
		[LED_EXT] = GPIOA /* внешний LD */
};

static const uint16_t led_pin[] = {
		[LED_GREEN] = GPIO_PIN_5, /* PA5 */
	    [LED_EXT] = GPIO_PIN_6 /* PA6 */
};

void MyBSP_LED_Init (void) {
	/*Настройка PA5 как выход */
	GPIOA->MODER &= ~(3U << (5 * 2));
	GPIOA->MODER |= (1U << (5 * 2));  /* 01 = Output */

	/* Настройка PA6 как выход */
	GPIOA->MODER &= ~(3U << (6 * 2));
	GPIOA->MODER |= (1U << (6 * 2));  /* 01 = Output */


	/*Гарантировано погасить оба светодиода при старте */
	GPIOA->BSRR = (1U << (5 + 16)) | (1U << (6 + 16));
}

void MyBSP_LED_On (BSP_LED_t led) {

	led_port[led]->BSRR = led_pin[led];
}

void MyBSP_LED_Off (BSP_LED_t led) {

	led_port[led]->BSRR = (led_pin[led] << 16);
}

void MyBSP_LED_Toggle (BSP_LED_t led) {

	led_port[led]->ODR ^= (1U << led_pin[led]);
}
