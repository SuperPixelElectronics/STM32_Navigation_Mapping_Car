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
  HAL_GPIO_WritePin(GPIOA, NRF_MOSI_Pin|NRF_CSN_Pin|NRF_SCK_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, NRF_CE_Pin|LED_Y_Pin|LED_B_Pin|LED_G_Pin
                          |LED_R_Pin|OLED_SCL_Pin|OLED_SDA_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PUSH_KEY_Pin */
  GPIO_InitStruct.Pin = PUSH_KEY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(PUSH_KEY_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : NRF_MOSI_Pin NRF_CSN_Pin NRF_SCK_Pin */
  GPIO_InitStruct.Pin = NRF_MOSI_Pin|NRF_CSN_Pin|NRF_SCK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : TRACK_DOWN_Pin TRACK_RIGHT_Pin */
  GPIO_InitStruct.Pin = TRACK_DOWN_Pin|TRACK_RIGHT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : NRF_CE_Pin LED_Y_Pin LED_B_Pin LED_G_Pin
                           LED_R_Pin OLED_SCL_Pin OLED_SDA_Pin */
  GPIO_InitStruct.Pin = NRF_CE_Pin|LED_Y_Pin|LED_B_Pin|LED_G_Pin
                          |LED_R_Pin|OLED_SCL_Pin|OLED_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : NRF_IRQ_Pin */
  GPIO_InitStruct.Pin = NRF_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(NRF_IRQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : NRF_MISO_Pin EDGE1_KEY_Pin EDGE2_KEY_Pin DOWN_KEY_Pin
                           RIGHT_KEY_Pin TRACK_KEY_Pin */
  GPIO_InitStruct.Pin = NRF_MISO_Pin|EDGE1_KEY_Pin|EDGE2_KEY_Pin|DOWN_KEY_Pin
                          |RIGHT_KEY_Pin|TRACK_KEY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : UP_KEY_Pin LEFT_KEY_Pin */
  GPIO_InitStruct.Pin = UP_KEY_Pin|LEFT_KEY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : TRACK_UP_Pin TRACK_LEFT_Pin */
  GPIO_InitStruct.Pin = TRACK_UP_Pin|TRACK_LEFT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
