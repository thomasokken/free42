/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>

#include "core_main.h"
#include "shell_main.h"

#define __ramFunc __attribute__((section(".RamFunc")))

int Scan_Keyboard(void);
void Basic_Hardware_Test();
uint8_t GetKey(uint16_t pin);

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define KEY_QUEUE_SIZE 10
uint8_t key_queue[KEY_QUEUE_SIZE];
uint8_t kqri = 0;
uint8_t kqwi = 0;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
bool enqueued = false;
int repeat = 0;

void __ramFunc update_lcd() {
	//frame[0] ^= 0xff;
  if (true) {
	  frame_ready = false;
	  setAddress(0, 0);
	  sendData(frame, 132);
	  setAddress(1, 0);
	  sendData(frame + 132, 132);
	  setAddress(2, 0);
	  sendData(frame + 132 * 2, 132);
	  setAddress(3, 0);
	  sendData(frame + 132 * 3, 132);
  }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  //SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI3_Init();
  /* USER CODE BEGIN 2 */

  // interrupt requets are disabled in the firmware so that no interrupts are triggered
  // while interrupt handlers are being copied into RAM
  __enable_irq();

  core_init(0, 1, "", 0);
  frame_ready = true;
  update_lcd();
  ROW0_GPIO_Port->BSRR = ROW0_Pin|ROW1_Pin|ROW2_Pin|ROW3_Pin|ROW4_Pin|ROW5_Pin|ROW6_Pin;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

	  if (kqri == kqwi) continue;
	  uint8_t key = key_queue[kqri++];
	  if (kqri == KEY_QUEUE_SIZE) kqri = 0;
	  if (key == 255) core_keyup();
	  else {
		  bool* enqued = false;
		  int* repeat = 0;
		  core_keydown(key, enqued, repeat);
	  }

	  frame_ready = true;
	  update_lcd();

	  /**
	   *
	   *     core_init(0, 1, "", 0);

    while (true) {
        int input;
        std::cin >> input;
        bool* enqued = new bool;
        int* repeat = new int;
        core_keydown(input, enqued, repeat);
    }
	   */
	  /*int key = Scan_Keyboard();
	  if (key == 255) {
		  continue;
	  }

	  core_keydown(key, &enqueued, &repeat);
	  frame_ready = true;

	  update_lcd();

	  for (int i = 0; i < 100; i++)
		  while (Scan_Keyboard() != 255) continue;

	  if (!enqueued)
		  core_keyup();
	  frame_ready = true;
	  update_lcd();*/

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
uint32_t last_key_time = 0;
uint8_t last_key = 0;
void __ramFunc HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (last_key_time != 0 && uwTick - last_key_time < 20) {
		EXTI->PR1 = (COL0_Pin | COL1_Pin | COL2_Pin | COL3_Pin | COL4_Pin | COL5_Pin);
		return;
	}
	last_key_time = uwTick;// HAL_GetTick();

	uint8_t key = GetKey(GPIO_Pin);

	if (key == last_key) return;

	if (key == 255) {
		last_key = 0;

		key_queue[kqwi++] = key;
		if (kqwi == KEY_QUEUE_SIZE) kqwi = 0;

		return;
	}

	last_key = key;
	key_queue[kqwi++] = key;
	if (kqwi == KEY_QUEUE_SIZE) kqwi = 0;
	EXTI->PR1 = (COL0_Pin | COL1_Pin | COL2_Pin | COL3_Pin | COL4_Pin | COL5_Pin);
}


uint8_t __ramFunc GetKey(uint16_t pin)
{
	  GPIO_InitTypeDef GPIO_InitStruct = {0};

	  GPIO_InitStruct.Pin = COL0_Pin|COL1_Pin|COL2_Pin|COL3_Pin
	                          |COL4_Pin|COL5_Pin;
	  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	  HAL_GPIO_Init(COL0_GPIO_Port, &GPIO_InitStruct);
	  EXTI->IMR1 &= ~(COL0_Pin | COL1_Pin | COL2_Pin | COL3_Pin | COL4_Pin | COL5_Pin);

	uint16_t rows[] = { ROW0_Pin, ROW1_Pin, ROW2_Pin, ROW3_Pin, ROW4_Pin, ROW5_Pin, ROW6_Pin };
	uint8_t key_press = 255;

	ROW0_GPIO_Port->BRR = ROW0_Pin|ROW1_Pin|ROW2_Pin|ROW3_Pin|ROW4_Pin|ROW5_Pin|ROW6_Pin;
	//HAL_GPIO_WritePin(ROW0_GPIO_Port, ROW0_Pin|ROW1_Pin|ROW2_Pin|ROW3_Pin|ROW4_Pin|ROW5_Pin|ROW6_Pin, GPIO_PIN_RESET);
	for (uint8_t r = 0; r < 7; r++) {
		/*
		 *
		 *   if(PinState != GPIO_PIN_RESET)
  {
    GPIOx->BSRR = (uint32_t)GPIO_Pin;
  }
  else
  {
    GPIOx->BRR = (uint32_t)GPIO_Pin;
  }
		 */
		ROW0_GPIO_Port->BSRR = rows[r];
		GPIO_PinState state = (COL0_GPIO_Port->IDR & pin) != 0 ? GPIO_PIN_SET : GPIO_PIN_RESET;
		ROW0_GPIO_Port->BRR = rows[r];

		int c = -1;
		if (state == GPIO_PIN_SET) {
			switch (pin) {
				case COL0_Pin: c = 0; break;
				case COL1_Pin: c = 1; break;
				case COL2_Pin: c = 2; break;
				case COL3_Pin: c = 3; break;
				case COL4_Pin: c = 4; break;
				case COL5_Pin: c = 5; break;
			}

			int row = r == 8 ? 5 : (r == 10 ? 6 : r);
			key_press = row * 5 + c + 1;
			if (row > 0) key_press++;
			if (row > 1) key_press++;
			if (row > 2) key_press++;
			if (key_press > 13) key_press--;

			break;
		}
	}

	ROW0_GPIO_Port->BSRR =  ROW0_Pin|ROW1_Pin|ROW2_Pin|ROW3_Pin|ROW4_Pin|ROW5_Pin|ROW6_Pin;
	//HAL_GPIO_WritePin(ROW0_GPIO_Port, ROW0_Pin|ROW1_Pin|ROW2_Pin|ROW3_Pin|ROW4_Pin|ROW5_Pin|ROW6_Pin, GPIO_PIN_SET);

		  GPIO_InitStruct.Pin = COL0_Pin|COL1_Pin|COL2_Pin|COL3_Pin
		                          |COL4_Pin|COL5_Pin;
		  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
		  GPIO_InitStruct.Pull = GPIO_PULLDOWN;

		  HAL_GPIO_Init(COL0_GPIO_Port, &GPIO_InitStruct);
	  EXTI->IMR1 |= (COL0_Pin | COL1_Pin | COL2_Pin | COL3_Pin | COL4_Pin | COL5_Pin);
		EXTI->PR1 = (COL0_Pin | COL1_Pin | COL2_Pin | COL3_Pin | COL4_Pin | COL5_Pin);


	return key_press;
}

int Scan_Keyboard()
{
	uint16_t rows = 0b10100011111;
	uint16_t columns = 0b111111;

	int key_press = 255;

	for (int r = 0; r < 11; r++) {
		if ((rows & (1 << r)) == 0) continue;
		ROW0_GPIO_Port->ODR |= (1 << r);

		int c = -1;

		if (HAL_GPIO_ReadPin(COL0_GPIO_Port, COL0_Pin) == GPIO_PIN_SET)
			c = 0;
		else 		if (HAL_GPIO_ReadPin(COL1_GPIO_Port, COL1_Pin) == GPIO_PIN_SET)
			c = 1;
		else		if (HAL_GPIO_ReadPin(COL2_GPIO_Port, COL2_Pin) == GPIO_PIN_SET)
			c = 2;
		else 		if (HAL_GPIO_ReadPin(COL3_GPIO_Port, COL3_Pin) == GPIO_PIN_SET)
			c = 3;
		else		if (HAL_GPIO_ReadPin(COL4_GPIO_Port, COL4_Pin) == GPIO_PIN_SET)
			c = 4;
		else 		if (HAL_GPIO_ReadPin(COL5_GPIO_Port, COL5_Pin) == GPIO_PIN_SET)
			c = 5;

		if (c != -1) {
			int row = r == 8 ? 5 : (r == 10 ? 6 : r);
			key_press = row * 5 + c + 1;
			if (row > 0) key_press++;
			if (row > 1) key_press++;
			if (row > 2) key_press++;
			if (key_press > 13) key_press--;
		}

		ROW0_GPIO_Port->ODR &= ~(1 << r);

		if (key_press != 255)
			break;

	}

	return key_press;
}


void Basic_Hardware_Test() {
	  unsigned int page = 0;
	  unsigned int column = 0;
	  char zero[132] = {0};
	  for (int p = 0; p < 4; p++) {
		  setAddress(p, 0);
		  sendData(zero, 132);
	  }

	  while (1)
	  {
		  int key = Scan_Keyboard();
		  if (key == 255) continue;

		  uint8_t ten = key / 10;
		  uint8_t one = key % 10;

		  uint8_t key_string[] = { ten + '0', one + '0', '\0' };

		  setAddress(page, column);
		  sendData(characters[ten], 5);

		  column += 5;
		  if (column > 131 - 5) {
			  column = 0;
			  page++;
			  if (page > 3) {
				  page = 0;
			  }
		  }

		  setAddress(page, column);
		  sendData(characters[one], 5);

		  column += 8;
		  if (column > 131 - 5) {
			  column = 0;
			  page++;
			  if (page > 3) {
				  page = 0;
			  }
		  }

		  for (int i = 0; i < 20; i++) {
			  while (Scan_Keyboard() == key) continue;
			  HAL_Delay(2);
		  }
	  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
