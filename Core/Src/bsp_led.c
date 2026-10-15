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
		[LED_EXT] = GPIOA, /* внешний LD */
		[LED_BLUE] = GPIOA /* внешний LD голубой */
};

static const uint16_t led_pin[] = {
		[LED_GREEN] = GPIO_PIN_5, /* PA5 */
	    [LED_EXT] = GPIO_PIN_6, /* PA6 */
		[LED_BLUE] = GPIO_PIN_7 /* PA7 */
};

void MyBSP_LED_Init (void) {
	/*Настройка PA5 как выход */
	GPIOA->MODER &= ~(3U << (5 * 2));
	GPIOA->MODER |= (1U << (5 * 2));  /* 01 = Output */

	/* Настройка PA6 как выход */
	GPIOA->MODER &= ~(3U << (6 * 2));
	GPIOA->MODER |= (1U << (6 * 2));  /* 01 = Output */

	/* Настройка PA7 как выход */
	GPIOA->MODER &= ~(3U << (7 * 2));
	GPIOA->MODER |= (1U << (7 * 2));  /* 01 = Output */


	/*Гарантировано погасить оба светодиода при старте */
	GPIOA->BSRR = (1U << (5 + 16)) | (1U << (6 + 16)) | (1U << (7 + 16));
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

void MyBSP_LED_Blink(BSP_LED_t led, uint32_t period_ms){

	led_port[led]->BSRR = led_pin[led];
	HAL_Delay(period_ms);
	led_port[led]->BSRR = (led_pin[led] << 16);
	HAL_Delay(period_ms);
}

void MyBSP_LED_SetMask(uint16_t mask){
	for (BSP_LED_t led = LED_GREEN; led <= LED_BLUE; led++)
	{
		if(mask & (1U << led)){
			MyBSP_LED_On(led);
		}
		else{
			MyBSP_LED_Off(led);
		}
	}
}

