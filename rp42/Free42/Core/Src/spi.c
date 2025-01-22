/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    spi.c
  * @brief   This file provides code for the configuration
  *          of the SPI instances.
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
/* Includes ------------------------------------------------------------------*/
#include "spi.h"

/* USER CODE BEGIN 0 */
uint8_t characters[10][5] = {
	{
		0b00111110,
		0b01111111,
		0b01100011,
		0b00111110,
		0b00000000
	},
	{
		0b01100110,
		0b01111111,
		0b01111111,
		0b01100000,
		0b00000000
	},
	{
		0b01110110,
		0b01110011,
		0b01011111,
		0b01011110,
		0b00000000
	},
	{
		0b01101011,
		0b01101011,
		0b01111111,
		0b01111111,
		0b00000000
	},
	{
		0b00011111,
		0b00011000,
		0b01111111,
		0b01111111,
		0b00000000
	},
	{
		0b01101111,
		0b01101111,
		0b01111101,
		0b00111001,
		0b00000000
	},
	{
		0b01111111,
		0b01111111,
		0b01011001,
		0b01111011,
		0b00000000
	},
	{
		0b01100011,
		0b01110011,
		0b00111111,
		0b00001111,
		0b00000000
	},
	{
		0b01111111,
		0b01111111,
		0b01001001,
		0b01111111,
		0b00000000
	},
	{
		0b00011111,
		0b00011001,
		0b01111111,
		0b01111111,
		0b00000000
	}
};
/* USER CODE END 0 */

SPI_HandleTypeDef hspi3;

/* SPI3 init function */
void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 7;
  hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi3.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */
  uint8_t LCD_INIT_COMMANDS[] = {
  		  0x40,
  		  0xA1,
  		  0xC0,
  		  0xA6,
  		  0xA2,
  		  0x2F,
  		  0xF8,
  		  0x00,
  		  0x23,
  		  0x81,
  		  0x1F,
  		  0xAC,
  		  0x00,
  		  0xA0,
  		  0xC8,
  		  0xAF
  };
  uint16_t LCD_INIT_COMMAND_COUNT = 16;

  sendCommand(LCD_INIT_COMMANDS, LCD_INIT_COMMAND_COUNT);
  setAddress(0, 0);
  sendData(characters[0], 5);
  /* USER CODE END SPI3_Init 2 */

}

void HAL_SPI_MspInit(SPI_HandleTypeDef* spiHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(spiHandle->Instance==SPI3)
  {
  /* USER CODE BEGIN SPI3_MspInit 0 */

  /* USER CODE END SPI3_MspInit 0 */
    /* SPI3 clock enable */
    __HAL_RCC_SPI3_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**SPI3 GPIO Configuration
    PC10     ------> SPI3_SCK
    PC12     ------> SPI3_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN SPI3_MspInit 1 */

  /* USER CODE END SPI3_MspInit 1 */
  }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* spiHandle)
{

  if(spiHandle->Instance==SPI3)
  {
  /* USER CODE BEGIN SPI3_MspDeInit 0 */

  /* USER CODE END SPI3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI3_CLK_DISABLE();

    /**SPI3 GPIO Configuration
    PC10     ------> SPI3_SCK
    PC12     ------> SPI3_MOSI
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10|GPIO_PIN_12);

  /* USER CODE BEGIN SPI3_MspDeInit 1 */

  /* USER CODE END SPI3_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

void sendCommand(uint8_t* command, size_t count) {
	  HAL_GPIO_WritePin(LCD_A0_GPIO_Port, LCD_A0_Pin, RESET);
	  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, SET);
	  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, RESET);

	  HAL_SPI_Transmit(&hspi3, command, count, 10000);
}

void sendData(uint8_t* data, size_t count) {
	  HAL_GPIO_WritePin(LCD_A0_GPIO_Port, LCD_A0_Pin, SET);
	  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, SET);
	  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, RESET);

	HAL_SPI_Transmit(&hspi3, data, count, 10000);
}

void setAddress(int page, int column) {
	uint8_t commands[3] = {
			0b10110000 | page,
			0b00010000 | (column >> 4),
			0b00000000 | (column & 0x0F)
	};

	sendCommand(commands, 3);
}
/* USER CODE END 1 */
