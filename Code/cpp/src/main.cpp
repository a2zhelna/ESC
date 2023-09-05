/*
 * main.cpp
 *
 *  Created on: Jun 7, 2023
 *      Author: Anton
 */

#include "main.h"
#include <string.h>	//for strlen()
#include <stdio.h> //for sprintf()

#define EIGHT 13

#define ADC_BUF_LEN 4

uint16_t count = 0;

uint32_t burstData[5] = {96, 48, 96, 144, 0};

uint16_t dmaSignal[16] = {0};
uint16_t dmaSignalNormalized[16] = {0};
uint8_t newDmaSignal = 0;
uint16_t speed = 0;
uint16_t speedArr[3] = {900, 200, 500};
uint16_t testSpeed = 0;

uint16_t fc_duty = 144;

uint8_t Rx_data[4] = {0};
uint16_t uart_count = 0;

uint8_t phase_config = 0;

void all_off();
void delay_u(uint16_t delay);

extern "C" {
	//ADC_HandleTypeDef hadc;
	TIM_HandleTypeDef htim6;
	TIM_HandleTypeDef htim3;
	TIM_HandleTypeDef htim2;
	TIM_HandleTypeDef htim1;
}

//Apparently because the ADC is now tied with the DMA controller, it considers
//a complete conversion to be when the buffer that the DMA WRITES TO is FILLED.
//Not when an actual, single conversion is complete.
//void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
//	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
//}
//void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
//	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
//}

//ADC Buffer
uint16_t adc_buf[ADC_BUF_LEN];

//Enable timer3 DMA input capture after the specified amount of data was transfered to memory
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
	if (htim == &htim3 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
		newDmaSignal = 1;
	}
}

uint16_t divClosest(uint16_t a, uint16_t b) {
	return (a + b/2) / b;
}

void processDmaSignal() {
	newDmaSignal = 0;

	for (int i = 0; i < 15; i++) {
		dmaSignalNormalized[i] = divClosest((dmaSignal[i+1] - dmaSignal[i]),6) - 8;
	}

	//Now, find the large number (the spot between sets of pulses) to find the most recently
	//captured set of pulses.
	//Then, extract ten bits out of those pulses
	// - least recent contains four MSBs
	// - after that are four more bits
	// - after that are the two LSBs
	for (int i = 0; i <= 10; i++) {
		if (dmaSignalNormalized[i] < 100) {
			continue;
		}

		//Sometimes the large gap jumps around the array, resulting in a very large speed value
		//if that's the case, do not update the speed. This occurrence seems to always go away quickly
		testSpeed = dmaSignalNormalized[i+1]<<6 |
					dmaSignalNormalized[i+3]<<2 |
					dmaSignalNormalized[i+5]>>2;

		if (testSpeed < 500)
			speed = testSpeed;

		break;
	}
}

// ---------- Commutation section

//Safety timer - timer2
uint32_t it_config_r = 0;
uint32_t it_config_f = 0;
uint8_t consecutive = 0;

uint16_t run_motor = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	if (htim == &htim2) {
		if (run_motor >= 200) {
			if (EXTI->RTSR == it_config_r &&
				EXTI->FTSR == it_config_f) {
				consecutive++;

				if (consecutive >= 6) {
					all_off();
					run_motor = 0;
				}
			}
			else {
				consecutive = 0;
			}
			it_config_r = EXTI->RTSR;
			it_config_f = EXTI->FTSR;
		}
	}
}
//
void A_RISING() {
	//Configure specified interrupt's edge
	//0 - Rising/Falling trigger disabled, 1 - Rising/Falling trigger enabled
	EXTI->RTSR |=  (1<<0);
	EXTI->RTSR &= ~(1<<2);
	EXTI->RTSR &= ~(1<<15);

	EXTI->FTSR &= ~(1<<0);
	EXTI->FTSR &= ~(1<<2);
	EXTI->FTSR &= ~(1<<15);
	//Mask all but the specified interrupts
	//1 - nonmasked (ready to be used), 0 - masked (not being used)
	EXTI->IMR |=  (1<<0);
	EXTI->IMR &= ~(1<<2);
	EXTI->IMR &= ~(1<<15);

	EXTI->PR  &= ~(1<<0 | 1<<2 | 1<<15);
}
void A_FALLING() {
	//Configure specified interrupt's edge
	//0 - Rising/Falling trigger disabled, 1 - Rising/Falling trigger enabled
	EXTI->RTSR &= ~(1<<0);
	EXTI->RTSR &= ~(1<<2);
	EXTI->RTSR &= ~(1<<15);

	EXTI->FTSR |= (1<<0);
	EXTI->FTSR &= ~(1<<2);
	EXTI->FTSR &= ~(1<<15);
	//Mask all but the specified interrupts
	//1 - nonmasked (ready to be used), 0 - masked (not being used)
	EXTI->IMR |=  (1<<0);
	EXTI->IMR &= ~(1<<2);
	EXTI->IMR &= ~(1<<15);

	EXTI->PR  &= ~(1<<0 | 1<<2 | 1<<15);
}
void B_RISING() {
	//Configure specified interrupt's edge
	//0 - Rising/Falling trigger disabled, 1 - Rising/Falling trigger enabled
	EXTI->RTSR &= ~(1<<0);
	EXTI->RTSR |=  (1<<2);
	EXTI->RTSR &= ~(1<<15);

	EXTI->FTSR &= ~(1<<0);
	EXTI->FTSR &= ~(1<<2);
	EXTI->FTSR &= ~(1<<15);
	//Mask all but the specified interrupts
	//1 - nonmasked (ready to be used), 0 - masked (not being used)
	EXTI->IMR &= ~(1<<0);
	EXTI->IMR |=  (1<<2);
	EXTI->IMR &= ~(1<<15);

	EXTI->PR  &= ~(1<<0 | 1<<2 | 1<<15);
}
void B_FALLING() {
	//Configure specified interrupt's edge
	//0 - Rising/Falling trigger disabled, 1 - Rising/Falling trigger enabled
	EXTI->RTSR &= ~(1<<0);
	EXTI->RTSR &= ~(1<<2);
	EXTI->RTSR &= ~(1<<15);

	EXTI->FTSR &= ~(1<<0);
	EXTI->FTSR |=  (1<<2);
	EXTI->FTSR &= ~(1<<15);
	//Mask all but the specified interrupts
	//1 - nonmasked (ready to be used), 0 - masked (not being used)
	EXTI->IMR &= ~(1<<0);
	EXTI->IMR |=  (1<<2);
	EXTI->IMR &= ~(1<<15);

	EXTI->PR  &= ~(1<<0 | 1<<2 | 1<<15);
}
void C_RISING() {
	//Configure specified interrupt's edge
	//0 - Rising/Falling trigger disabled, 1 - Rising/Falling trigger enabled
	EXTI->RTSR &= ~(1<<0);
	EXTI->RTSR &= ~(1<<2);
	EXTI->RTSR |=  (1<<15);

	EXTI->FTSR &= ~(1<<0);
	EXTI->FTSR &= ~(1<<2);
	EXTI->FTSR &= ~(1<<15);
	//Mask all but the specified interrupts
	//1 - nonmasked (ready to be used), 0 - masked (not being used)
	EXTI->IMR &= ~(1<<0);
	EXTI->IMR &= ~(1<<2);
	EXTI->IMR |= (1<<15);

	EXTI->PR  &= ~(1<<0 | 1<<2 | 1<<15);
}
void C_FALLING() {
	//Configure specified interrupt's edge
	//0 - Rising/Falling trigger disabled, 1 - Rising/Falling trigger enabled
	EXTI->RTSR &= ~(1<<0);
	EXTI->RTSR &= ~(1<<2);
	EXTI->RTSR &= ~(1<<15);

	EXTI->FTSR &= ~(1<<0);
	EXTI->FTSR &= ~(1<<2);
	EXTI->FTSR |=  (1<<15);
	//Mask all but the specified interrupts
	//1 - nonmasked (ready to be used), 0 - masked (not being used)
	EXTI->IMR &= ~(1<<0);
	EXTI->IMR &= ~(1<<2);
	EXTI->IMR |=  (1<<15);

	EXTI->PR  &= ~(1<<0 | 1<<2 | 1<<15);
}

volatile uint8_t bldc_state = 0;
volatile uint16_t nextITPin = 0;

uint16_t PWM_CCER_ONMASK[6] {
	( 1 ),
	( 1 ),
	( (1<<4) ),
	( (1<<4) ),
	( (1<<8) ),
	( (1<<8) )
};

uint16_t PWM_CCER_OFFMASK[6] {
	( (1<<4) | (1<<8) ),
	( (1<<4) | (1<<8) ),
	( 1 | (1<<8) ),
	( 1 | (1<<8) ),
	( 1 | (1<<4) ),
	( 1 | (1<<4) )
};

void bldc_move() {
	bldc_state++;
	bldc_state %= 6;

	EXTI->IMR  &= ~(1<<0 | 1<<2 | 1<<15);
	EXTI->RTSR  &= ~(1<<0 | 1<<2 | 1<<15);
	EXTI->FTSR  &= ~(1<<0 | 1<<2 | 1<<15);
	EXTI->PR  &= ~(1<<0 | 1<<2 | 1<<15);

	switch(bldc_state) {
		case 0:
			//A high, B low, C off
			nextITPin = ZC_C_Pin;
//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_2);
//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_3);
			TIM1->CCER &= ~PWM_CCER_OFFMASK[bldc_state];
			HAL_GPIO_WritePin(CLow_GPIO_Port, CLow_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(ALow_GPIO_Port, ALow_Pin, GPIO_PIN_SET);

			HAL_GPIO_WritePin(BLow_GPIO_Port, BLow_Pin, GPIO_PIN_RESET);
//			HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t*)&speed, 1);
			TIM1->CCER |= PWM_CCER_ONMASK[bldc_state];

			delay_u(2);

			C_FALLING();
			break;
//
		case 1:
			//A high, B off, C low
			nextITPin = ZC_B_Pin;

//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_2);
//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_3);
			TIM1->CCER &= ~PWM_CCER_OFFMASK[bldc_state];
			HAL_GPIO_WritePin(ALow_GPIO_Port, ALow_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(BLow_GPIO_Port, BLow_Pin, GPIO_PIN_SET);

			HAL_GPIO_WritePin(CLow_GPIO_Port, CLow_Pin, GPIO_PIN_RESET);
//			HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t*)&speed, 1);
			TIM1->CCER |= PWM_CCER_ONMASK[bldc_state];

			delay_u(2);

			B_RISING();
			break;

		case 2:
			//A off, B high, C low
			nextITPin = ZC_A_Pin;

//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_3);
			TIM1->CCER &= ~PWM_CCER_OFFMASK[bldc_state];
			HAL_GPIO_WritePin(ALow_GPIO_Port, ALow_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(BLow_GPIO_Port, BLow_Pin, GPIO_PIN_SET);

			HAL_GPIO_WritePin(CLow_GPIO_Port, CLow_Pin, GPIO_PIN_RESET);
//			HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_2, (uint32_t*)&speed, 1);
			TIM1->CCER |= PWM_CCER_ONMASK[bldc_state];

			delay_u(2);

			A_FALLING();
			break;

		case 3:

			//A low, B high, C off
			nextITPin = ZC_C_Pin;

//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_3);
			TIM1->CCER &= ~PWM_CCER_OFFMASK[bldc_state];
			HAL_GPIO_WritePin(BLow_GPIO_Port, BLow_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(CLow_GPIO_Port, CLow_Pin, GPIO_PIN_SET);

			HAL_GPIO_WritePin(ALow_GPIO_Port, ALow_Pin, GPIO_PIN_RESET);
//			HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_2, (uint32_t*)&speed, 1);
			TIM1->CCER |= PWM_CCER_ONMASK[bldc_state];

			delay_u(2);

			C_RISING();
			break;

		case 4:
			//A low, B off, C high
			nextITPin = ZC_B_Pin;

//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_2);
			TIM1->CCER &= ~PWM_CCER_OFFMASK[bldc_state];
			HAL_GPIO_WritePin(BLow_GPIO_Port, BLow_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(CLow_GPIO_Port, CLow_Pin, GPIO_PIN_SET);

			HAL_GPIO_WritePin(ALow_GPIO_Port, ALow_Pin, GPIO_PIN_RESET);
//			HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_3, (uint32_t*)&speed, 1);
			TIM1->CCER |= PWM_CCER_ONMASK[bldc_state];

			delay_u(2);

			B_FALLING();
			break;

		case 5:
			//A off, B low, C high
			nextITPin = ZC_A_Pin;

//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_2);
			TIM1->CCER &= ~PWM_CCER_OFFMASK[bldc_state];
			HAL_GPIO_WritePin(ALow_GPIO_Port, ALow_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(CLow_GPIO_Port, CLow_Pin, GPIO_PIN_SET);

			HAL_GPIO_WritePin(BLow_GPIO_Port, BLow_Pin, GPIO_PIN_RESET);
//			HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_3, (uint32_t*)&speed, 1);
			TIM1->CCER |= PWM_CCER_ONMASK[bldc_state];

			delay_u(2);

			A_RISING();
			break;

	}
}

void bldc_move_ol() {
	switch(bldc_state) {
		case 0:
			//A high, B low, C off
			//HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_2);
			//HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_3);
			TIM1->CCER &= ~PWM_CCER_OFFMASK[bldc_state];
			HAL_GPIO_WritePin(CLow_GPIO_Port, CLow_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(ALow_GPIO_Port, ALow_Pin, GPIO_PIN_SET);

			HAL_GPIO_WritePin(BLow_GPIO_Port, BLow_Pin, GPIO_PIN_RESET);
			//HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t*)&speed, 1);
			TIM1->CCER |= PWM_CCER_ONMASK[bldc_state];
			break;
	//
		case 1:
			//A high, B off, C low
//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_2);
//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_3);
			TIM1->CCER &= ~PWM_CCER_OFFMASK[bldc_state];
			HAL_GPIO_WritePin(ALow_GPIO_Port, ALow_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(BLow_GPIO_Port, BLow_Pin, GPIO_PIN_SET);

			HAL_GPIO_WritePin(CLow_GPIO_Port, CLow_Pin, GPIO_PIN_RESET);
			//HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t*)&speed, 1);
			TIM1->CCER |= PWM_CCER_ONMASK[bldc_state];
			break;

		case 2:
			//A off, B high, C low
//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_3);
			TIM1->CCER &= ~PWM_CCER_OFFMASK[bldc_state];
			HAL_GPIO_WritePin(ALow_GPIO_Port, ALow_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(BLow_GPIO_Port, BLow_Pin, GPIO_PIN_SET);

			HAL_GPIO_WritePin(CLow_GPIO_Port, CLow_Pin, GPIO_PIN_RESET);
			//HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_2, (uint32_t*)&speed, 1);
			TIM1->CCER |= PWM_CCER_ONMASK[bldc_state];
			break;

		case 3:
			//A low, B high, C off
//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_3);
			TIM1->CCER &= ~PWM_CCER_OFFMASK[bldc_state];
			HAL_GPIO_WritePin(BLow_GPIO_Port, BLow_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(CLow_GPIO_Port, CLow_Pin, GPIO_PIN_SET);

			HAL_GPIO_WritePin(ALow_GPIO_Port, ALow_Pin, GPIO_PIN_RESET);
			//HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_2, (uint32_t*)&speed, 1);
			TIM1->CCER |= PWM_CCER_ONMASK[bldc_state];
			break;

		case 4:
			//A low, B off, C high
//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_2);
			TIM1->CCER &= ~PWM_CCER_OFFMASK[bldc_state];
			HAL_GPIO_WritePin(BLow_GPIO_Port, BLow_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(CLow_GPIO_Port, CLow_Pin, GPIO_PIN_SET);

			HAL_GPIO_WritePin(ALow_GPIO_Port, ALow_Pin, GPIO_PIN_RESET);
			//HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_3, (uint32_t*)&speed, 1);
			TIM1->CCER |= PWM_CCER_ONMASK[bldc_state];
			break;

		case 5:
			//A off, B low, C high
//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
//			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_2);
			TIM1->CCER &= ~PWM_CCER_OFFMASK[bldc_state];
			HAL_GPIO_WritePin(ALow_GPIO_Port, ALow_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(CLow_GPIO_Port, CLow_Pin, GPIO_PIN_SET);

			HAL_GPIO_WritePin(BLow_GPIO_Port, BLow_Pin, GPIO_PIN_RESET);
			//HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_3, (uint32_t*)&speed, 1);
			TIM1->CCER |= PWM_CCER_ONMASK[bldc_state];
			break;

	}
}

void all_off() {
	//HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
	//HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_2);
	//HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_3);
	TIM1->CCER &= ~( 1 | 1<<4 | 1<<8 );

	HAL_GPIO_WritePin(ALow_GPIO_Port, ALow_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(BLow_GPIO_Port, BLow_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(CLow_GPIO_Port, CLow_Pin, GPIO_PIN_SET);

	EXTI->RTSR &= ~(1<<0);
	EXTI->RTSR &= ~(1<<2);
	EXTI->RTSR &= ~(1<<15);

	EXTI->FTSR &= ~(1<<0);
	EXTI->FTSR &= ~(1<<2);
	EXTI->FTSR &= ~(1<<15);
	//Mask all but the specified interrupts
	//1 - nonmasked (ready to be used), 0 - masked (not being used)
	EXTI->IMR &= ~(1<<0);
	EXTI->IMR &= ~(1<<2);
	EXTI->IMR &= ~(1<<15);
}

//The entries are ports on which an interrupt is expected for a bldc_state corresponding to the index
const GPIO_TypeDef* zcPortOff[6] = {
		(GPIO_TypeDef*)ZC_C_GPIO_Port,
		(GPIO_TypeDef*)ZC_B_GPIO_Port,
		(GPIO_TypeDef*)ZC_A_GPIO_Port,
		(GPIO_TypeDef*)ZC_C_GPIO_Port,
		(GPIO_TypeDef*)ZC_B_GPIO_Port,
		(GPIO_TypeDef*)ZC_A_GPIO_Port
};

//The faster the motor spins, the less time the period should last for which the ZC interrupt is de-bounced.
//Otherwise, fast (and expected) ZC transitions will be considered noise... which is bad
uint16_t speed_to_delay() {
	if (speed < 150) {
		return 400 - speed/2;
	}
	else if (speed < 300) {
		return 200 - speed/2;
	}
	else {
		return 50 - speed/10;
	}
}
uint16_t my_delay = 500;
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	//Callback is called only by the specified edge of the specified pin's interrupt
	//It's called when ZC occurs, which lets us know to go to the next phase configuration (bldc_state)
	if (GPIO_Pin == nextITPin) {	//Additional check for ensuring expected interrupt is called
		__HAL_TIM_SET_COUNTER(&htim6, 0);
		//my_delay = speed_to_delay();
		//guser suggested to use 768+48 as the delay
		while (__HAL_TIM_GET_COUNTER(&htim6) < (400+48-(speed/2))) {
			//Compare the state of the active interrupt line to its expected state
			//If it doesn't match, then it is probably noise

			//If pin is low on active rising edge interrupt line... BAD!
			//If pin is high on active falling edge IT line... BAD!
			//THEN BAD!
			if (EXTI->RTSR & 0xFFFF != 0) {
				if ((zcPortOff[bldc_state]->IDR & (EXTI->RTSR & 0xFFFF)) == 0) {
					return;
				}
			}
			else if (EXTI->FTSR & 0xFFFF != 0) {
				if (zcPortOff[bldc_state]->IDR & (EXTI->RTSR & 0xFFFF)) {
					return;
				}
			}
		}
		bldc_move();
		//Though all but the expected interrupt is disabled, sometimes strange pending interrupts appear, which ain't good
	}
}

void setup() {
	//Ensure all motor control signals and interrupts aren't active
	all_off();
	//Start safety timer
	if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK) {
		Error_Handler();
	}
	//Control CAPTURE timer:
	HAL_TIM_IC_Start_DMA(&htim3, TIM_CHANNEL_1, (uint32_t*)dmaSignal, 16);

	//Driver High pin PWM outputs
	if (HAL_TIM_Base_Start(&htim1) != HAL_OK) {
		Error_Handler();
	}
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t*)&speed, 1);
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_2, (uint32_t*)&speed, 1);
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_3, (uint32_t*)&speed, 1);

	//Turn off the PWM signals (now we can control them using the CCER register)
	TIM1->CCER &= ~(1);
	TIM1->CCER &= ~(1<<4);
	TIM1->CCER &= ~(1<<8);

	//Start delay timer
	if (HAL_TIM_Base_Start(&htim6) != HAL_OK) {
			Error_Handler();
	}

	//The following steps were said to be necessary to properly set up the DMA with the PWM timers
	//Explained in https://www.youtube.com/watch?v=crZn8HKVJss&t=1371s @ 55:40

	// Set the pulse widths to zero
//	TIM3->CCR1 = 0;
//	TIM3->CCR2 = 0;
//	TIM3->CCR3 = 0;
//
//	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
//	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
//	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
//
//	HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
//	HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_2);
//	HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_3);
	
	//HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t*)speedArr, 3);
//	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_2, (uint32_t*)speed, 1);
//	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_3, (uint32_t*)speed, 1);
	//HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, pData, Length)

	HAL_TIM_Base_Start(&htim1);
}




uint8_t it_count = 0;
uint16_t tim2cnt = 0;
void delay_u(uint16_t delay) {
	__HAL_TIM_SET_COUNTER(&htim6, 0);
	while (__HAL_TIM_GET_COUNTER(&htim6) < delay*48);
}

void maincpp() {
	setup();

	while(1) {
		if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
			HAL_Delay(200);
			//HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
			all_off();
			run_motor = run_motor ? 0 : 1;

		}
		else {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
			//Open-loop commutation

		}

		if (run_motor) {
			if (run_motor<200) {
				int16_t i = 1000;
				speed = 150;
				while (i > 100) {
					delay_u(i);
					delay_u(i);
					delay_u(i);
					delay_u(i);
					delay_u(i);
					bldc_state++;
					bldc_state %= 6;
					bldc_move_ol();

					i=i-4;
				}
				i = 0;
				run_motor = 200;
			}
			else {
				//speed=20;
				if (run_motor==200) {
					run_motor++;
					switch(bldc_state) {
						case 0:
							nextITPin = ZC_C_Pin;
							C_FALLING();
							break;
						case 1:
							nextITPin = ZC_B_Pin;
							B_RISING();
							break;
						case 2:
							nextITPin = ZC_A_Pin;
							A_FALLING();
							break;
						case 3:
							nextITPin = ZC_C_Pin;
							C_RISING();
							break;
						case 4:
							nextITPin = ZC_B_Pin;
							B_FALLING();
							break;
						case 5:
							nextITPin = ZC_A_Pin;
							A_RISING();
							break;
					}
					//bldc_state=0;
					//nextITPin = ZC_C_Pin;
					//C_FALLING();
				}
			}

		}

		if (newDmaSignal) {
			//Begin to transfer data obtained from timer to memory
			HAL_TIM_IC_Start_DMA(&htim3, TIM_CHANNEL_1, (uint32_t*)dmaSignal, 16);
			//Process this data (the times of different rising and falling edges being detected
			processDmaSignal();
		}

	}
}


