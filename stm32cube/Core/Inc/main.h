/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define UI_LED_Pin GPIO_PIN_0
#define UI_LED_GPIO_Port GPIOA
#define ADC1_BATT_MEAS_Pin GPIO_PIN_1
#define ADC1_BATT_MEAS_GPIO_Port GPIOA
#define UI_B_Pin GPIO_PIN_2
#define UI_B_GPIO_Port GPIOA
#define UI_A_Pin GPIO_PIN_3
#define UI_A_GPIO_Port GPIOA
#define TIM14_CH1_BUZZER_PWM_Pin GPIO_PIN_4
#define TIM14_CH1_BUZZER_PWM_GPIO_Port GPIOA
#define UI_C_Pin GPIO_PIN_5
#define UI_C_GPIO_Port GPIOA
#define nJACK_CONNECTED_Pin GPIO_PIN_7
#define nJACK_CONNECTED_GPIO_Port GPIOA
#define ADC9_TEMP_MEAS_Pin GPIO_PIN_1
#define ADC9_TEMP_MEAS_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */