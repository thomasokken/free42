/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "stm32l4xx_hal.h"

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
#define COL0_Pin GPIO_PIN_0
#define COL0_GPIO_Port GPIOC
#define COL0_EXTI_IRQn EXTI0_IRQn
#define COL1_Pin GPIO_PIN_1
#define COL1_GPIO_Port GPIOC
#define COL1_EXTI_IRQn EXTI1_IRQn
#define COL2_Pin GPIO_PIN_2
#define COL2_GPIO_Port GPIOC
#define COL2_EXTI_IRQn EXTI2_IRQn
#define COL3_Pin GPIO_PIN_3
#define COL3_GPIO_Port GPIOC
#define COL3_EXTI_IRQn EXTI3_IRQn
#define ROW0_Pin GPIO_PIN_0
#define ROW0_GPIO_Port GPIOA
#define ROW1_Pin GPIO_PIN_1
#define ROW1_GPIO_Port GPIOA
#define ROW2_Pin GPIO_PIN_2
#define ROW2_GPIO_Port GPIOA
#define ROW3_Pin GPIO_PIN_3
#define ROW3_GPIO_Port GPIOA
#define ROW4_Pin GPIO_PIN_4
#define ROW4_GPIO_Port GPIOA
#define COL4_Pin GPIO_PIN_4
#define COL4_GPIO_Port GPIOC
#define COL4_EXTI_IRQn EXTI4_IRQn
#define COL5_Pin GPIO_PIN_5
#define COL5_GPIO_Port GPIOC
#define COL5_EXTI_IRQn EXTI9_5_IRQn
#define LCD_A0_Pin GPIO_PIN_12
#define LCD_A0_GPIO_Port GPIOB
#define LCD_CS_Pin GPIO_PIN_13
#define LCD_CS_GPIO_Port GPIOB
#define LCD_RST_Pin GPIO_PIN_14
#define LCD_RST_GPIO_Port GPIOB
#define ROW5_Pin GPIO_PIN_8
#define ROW5_GPIO_Port GPIOA
#define ROW6_Pin GPIO_PIN_10
#define ROW6_GPIO_Port GPIOA
#define PWR_PERPH_Pin GPIO_PIN_8
#define PWR_PERPH_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
