/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, NRF_MOSI_Pin|NRF_CSN_Pin|NRF_SCK_Pin|LED_R_Pin
                          |MOTOR_B2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, NRF_CE_Pin|OLED_SCL_Pin|OLED_SDA_Pin|LED_B_Pin
                          |LED_G_Pin|MOTOR_B1_Pin|MOTOR_A2_Pin|MOTOR_A1_Pin
                          |VL53_SCL_Pin|VL53_SDA_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : BUZZER_Pin */
  GPIO_InitStruct.Pin = BUZZER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BUZZER_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : NRF_MOSI_Pin NRF_CSN_Pin NRF_SCK_Pin LED_R_Pin
                           MOTOR_B2_Pin */
  GPIO_InitStruct.Pin = NRF_MOSI_Pin|NRF_CSN_Pin|NRF_SCK_Pin|LED_R_Pin
                          |MOTOR_B2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : NRF_CE_Pin OLED_SCL_Pin OLED_SDA_Pin LED_B_Pin
                           LED_G_Pin MOTOR_B1_Pin MOTOR_A2_Pin MOTOR_A1_Pin */
  GPIO_InitStruct.Pin = NRF_CE_Pin|OLED_SCL_Pin|OLED_SDA_Pin|LED_B_Pin
                          |LED_G_Pin|MOTOR_B1_Pin|MOTOR_A2_Pin|MOTOR_A1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : NRF_IRQ_Pin */
  GPIO_InitStruct.Pin = NRF_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(NRF_IRQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : NRF_MISO_Pin */
  GPIO_InitStruct.Pin = NRF_MISO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(NRF_MISO_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : VL53_SCL_Pin VL53_SDA_Pin */
  GPIO_InitStruct.Pin = VL53_SCL_Pin|VL53_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
