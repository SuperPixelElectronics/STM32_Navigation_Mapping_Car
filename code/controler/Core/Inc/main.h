/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32f1xx_hal.h"

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
#define PUSH_KEY_Pin GPIO_PIN_13
#define PUSH_KEY_GPIO_Port GPIOC
#define NRF_MOSI_Pin GPIO_PIN_1
#define NRF_MOSI_GPIO_Port GPIOA
#define NRF_CSN_Pin GPIO_PIN_4
#define NRF_CSN_GPIO_Port GPIOA
#define NRF_SCK_Pin GPIO_PIN_5
#define NRF_SCK_GPIO_Port GPIOA
#define TRACK_DOWN_Pin GPIO_PIN_6
#define TRACK_DOWN_GPIO_Port GPIOA
#define TRACK_RIGHT_Pin GPIO_PIN_7
#define TRACK_RIGHT_GPIO_Port GPIOA
#define NRF_CE_Pin GPIO_PIN_0
#define NRF_CE_GPIO_Port GPIOB
#define NRF_IRQ_Pin GPIO_PIN_1
#define NRF_IRQ_GPIO_Port GPIOB
#define NRF_IRQ_EXTI_IRQn EXTI1_IRQn
#define NRF_MISO_Pin GPIO_PIN_2
#define NRF_MISO_GPIO_Port GPIOB
#define LED_Y_Pin GPIO_PIN_10
#define LED_Y_GPIO_Port GPIOB
#define LED_B_Pin GPIO_PIN_11
#define LED_B_GPIO_Port GPIOB
#define EDGE1_KEY_Pin GPIO_PIN_12
#define EDGE1_KEY_GPIO_Port GPIOB
#define EDGE2_KEY_Pin GPIO_PIN_13
#define EDGE2_KEY_GPIO_Port GPIOB
#define DOWN_KEY_Pin GPIO_PIN_14
#define DOWN_KEY_GPIO_Port GPIOB
#define RIGHT_KEY_Pin GPIO_PIN_15
#define RIGHT_KEY_GPIO_Port GPIOB
#define UP_KEY_Pin GPIO_PIN_8
#define UP_KEY_GPIO_Port GPIOA
#define BUZZER_Pin GPIO_PIN_11
#define BUZZER_GPIO_Port GPIOA
#define LEFT_KEY_Pin GPIO_PIN_15
#define LEFT_KEY_GPIO_Port GPIOA
#define LED_G_Pin GPIO_PIN_3
#define LED_G_GPIO_Port GPIOB
#define LED_R_Pin GPIO_PIN_4
#define LED_R_GPIO_Port GPIOB
#define TRACK_KEY_Pin GPIO_PIN_5
#define TRACK_KEY_GPIO_Port GPIOB
#define TRACK_UP_Pin GPIO_PIN_6
#define TRACK_UP_GPIO_Port GPIOB
#define TRACK_LEFT_Pin GPIO_PIN_7
#define TRACK_LEFT_GPIO_Port GPIOB
#define OLED_SCL_Pin GPIO_PIN_8
#define OLED_SCL_GPIO_Port GPIOB
#define OLED_SDA_Pin GPIO_PIN_9
#define OLED_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
