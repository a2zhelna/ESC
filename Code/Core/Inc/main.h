/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void maincpp();
void TransferComplete();
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ZC_C_Pin GPIO_PIN_15
#define ZC_C_GPIO_Port GPIOC
#define ZC_C_EXTI_IRQn EXTI4_15_IRQn
#define ZC_A_Pin GPIO_PIN_0
#define ZC_A_GPIO_Port GPIOC
#define ZC_A_EXTI_IRQn EXTI0_1_IRQn
#define ZC_B_Pin GPIO_PIN_2
#define ZC_B_GPIO_Port GPIOC
#define ZC_B_EXTI_IRQn EXTI2_3_IRQn
#define PushButton_Pin GPIO_PIN_0
#define PushButton_GPIO_Port GPIOA
#define Timer1_OUT_Pin GPIO_PIN_5
#define Timer1_OUT_GPIO_Port GPIOA
#define Proshot_IN_Pin GPIO_PIN_6
#define Proshot_IN_GPIO_Port GPIOA
#define CLow_Pin GPIO_PIN_15
#define CLow_GPIO_Port GPIOB
#define BLow_Pin GPIO_PIN_6
#define BLow_GPIO_Port GPIOC
#define ALow_Pin GPIO_PIN_7
#define ALow_GPIO_Port GPIOC
#define LED_BLUE_Pin GPIO_PIN_8
#define LED_BLUE_GPIO_Port GPIOC
#define LED_GREEN_Pin GPIO_PIN_9
#define LED_GREEN_GPIO_Port GPIOC
#define AHigh_PWM_Pin GPIO_PIN_8
#define AHigh_PWM_GPIO_Port GPIOA
#define BHigh_PWM_Pin GPIO_PIN_9
#define BHigh_PWM_GPIO_Port GPIOA
#define CHigh_PWM_Pin GPIO_PIN_10
#define CHigh_PWM_GPIO_Port GPIOA
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
