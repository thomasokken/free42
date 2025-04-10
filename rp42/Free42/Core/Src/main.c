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

//extern uint32_t __attribute__((section("SYS_FUNC"))) (*sys_func)(uint32_t command, void* args);
//uint64_t (*system_call_data)(uint16_t, void*) = 0x20000000;

SystemCallData __attribute__((section(".SYS_CALL_DATA"))) systemCallData;

//int Scan_Keyboard(void);
//void Basic_Hardware_Test();
//uint8_t GetKey(uint16_t pin);

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
//#define KEY_QUEUE_SIZE 10
//uint8_t key_queue[KEY_QUEUE_SIZE];
//uint8_t kqri = 0;
//uint8_t kqwi = 0;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
//void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
bool enqueued = false;
int repeat = 0;

void update_lcd() {
	systemCallData.args = frame; // the frame array contains the bytes to draw to the LCD
	systemCallData.command = 0x0012; // 0x0012 = DRAW_LCD
	__asm__("SVC #0");
	//__asm__("SVC #0"); // perform the sys call
	// DRAW_LCD has no return value, so nothing is needed afterward
	//frame[0] ^= 0xff;
  /*if (true) {
	  frame_ready = false;
	  setAddress(0, 0);
	  sendData(frame, 132);
	  setAddress(1, 0);
	  sendData(frame + 132, 132);
	  setAddress(2, 0);
	  sendData(frame + 132 * 2, 132);
	  setAddress(3, 0);
	  sendData(frame + 132 * 3, 132);
  }*/
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
  //HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  //SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  //MX_GPIO_Init();
  //MX_SPI3_Init();
  /* USER CODE BEGIN 2 */

  // interrupt requests are disabled in the firmware so that no interrupts are triggered
  // while interrupt handlers are being copied into RAM
  //__enable_irq();

  core_init(0, 1, "", 0);
  update_lcd();
  //ROW0_GPIO_Port->BSRR = ROW0_Pin|ROW1_Pin|ROW2_Pin|ROW3_Pin|ROW4_Pin|ROW5_Pin|ROW6_Pin;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  bool keydown = false;
  bool* enqueued = (bool*) malloc(sizeof(bool));
  int* repeat = (int*) malloc(sizeof(int));
  while (1)
  {
	  //HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	  /*uint8_t key = 254;
	  if (kqri == kqwi) {
		  HAL_Delay(10);
		  key = Scan_Keyboard();
		  if (key == 255 && !keydown) {
			  key = 254;
			  continue;
		  }
	  }

	  if (key == 254) key = key_queue[kqri++];*/
	  //uint8_t key = (uint8_t) sys_func(0x0002, 0);

	  while (1) {
		  systemCallData.command = 0x0002;
		  __asm__("SVC #0");
		  uint8_t key = (uint8_t) systemCallData.result;
		  // read all keys before redrawing

		  if (key == 255) {
			  core_keyup();
		  } else {
			  core_keydown(key, enqueued, repeat);
		  }

		  //update_lcd();

	  }

	  uint8_t key = systemCallData.result;
	  if (key != 255) keydown = true;
	  if (key == 255) core_keyup();
	  else {
		  core_keydown(key, enqueued, repeat);
	  }

	  //frame_ready = true;
	  //update_lcd();

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
/*void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage

  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.

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
}*/
/*
/* USER CODE BEGIN 4
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
}*/

/**
 * Determines which key was pressed.
 * Input: pin corresponding to the column that was triggered
 * Returns: the key encoded in a uint8_t from 1 to 37 inclusive. Returns 255 if no key is pressed.
 */
/*
uint8_t __ramFunc GetKey(uint16_t pin)
{
	  // disable interrupts on the keys so that it doesn't keep triggering more interrupts
	  EXTI->IMR1 &= ~(COL0_Pin | COL1_Pin | COL2_Pin | COL3_Pin | COL4_Pin | COL5_Pin);

	  // switch pins to digital input instead of external interrupts
	  GPIO_InitTypeDef GPIO_InitStruct = {0};

	  GPIO_InitStruct.Pin = COL0_Pin|COL1_Pin|COL2_Pin|COL3_Pin
	                          |COL4_Pin|COL5_Pin;
	  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	  GPIO_InitStruct.Pull = GPIO_PULLDOWN;

	  // this function should be put in RAM since it is part of an interrupt routine
	  HAL_GPIO_Init(COL0_GPIO_Port, &GPIO_InitStruct);

	  // there's probably a better way to encode the row pins that an array
	uint16_t rows[] = { ROW0_Pin, ROW1_Pin, ROW2_Pin, ROW3_Pin, ROW4_Pin, ROW5_Pin, ROW6_Pin };

	// the key that was pressed, or 255 if no key was pressed.
	uint8_t key_press = 255;

	// reset all the row pins to 0 so that we can step through them 1 at a time
	ROW0_GPIO_Port->BRR = ROW0_Pin|ROW1_Pin|ROW2_Pin|ROW3_Pin|ROW4_Pin|ROW5_Pin|ROW6_Pin;
	for (uint8_t r = 0; r < 7; r++) {
		// set a row to high, then check if the column pin is high
		ROW0_GPIO_Port->BSRR = rows[r];
		GPIO_PinState state = (COL0_GPIO_Port->IDR & pin) != 0 ? GPIO_PIN_SET : GPIO_PIN_RESET;
		ROW0_GPIO_Port->BRR = rows[r];

		int c = -1; // get the column number
		if (state == GPIO_PIN_SET) {
			switch (pin) {
				case COL0_Pin: c = 0; break;
				case COL1_Pin: c = 1; break;
				case COL2_Pin: c = 2; break;
				case COL3_Pin: c = 3; break;
				case COL4_Pin: c = 4; break;
				case COL5_Pin: c = 5; break;
			}

			// do some math to convert into a number from 1 to 37
			int row = r == 8 ? 5 : (r == 10 ? 6 : r);
			key_press = row * 5 + c + 1;
			if (row > 0) key_press++;
			if (row > 1) key_press++;
			if (row > 2) key_press++;
			if (key_press > 13) key_press--;

			break;
		}
	}

	// set all pins back to high so that a rising interrupt will be triggered again when the button changes state
	ROW0_GPIO_Port->BSRR =  ROW0_Pin|ROW1_Pin|ROW2_Pin|ROW3_Pin|ROW4_Pin|ROW5_Pin|ROW6_Pin;

	// switch back to interrupt mode
		  GPIO_InitStruct.Pin = COL0_Pin|COL1_Pin|COL2_Pin|COL3_Pin
		                          |COL4_Pin|COL5_Pin;
		  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
		  GPIO_InitStruct.Pull = GPIO_PULLDOWN;

		  HAL_GPIO_Init(COL0_GPIO_Port, &GPIO_InitStruct);

		  // reset any pending external interrupts?
			EXTI->PR1 = (COL0_Pin | COL1_Pin | COL2_Pin | COL3_Pin | COL4_Pin | COL5_Pin);

			// renable external interrupts
	  EXTI->IMR1 |= (COL0_Pin | COL1_Pin | COL2_Pin | COL3_Pin | COL4_Pin | COL5_Pin);


	return key_press;
}

int Scan_Keyboard()
{
	  // disable interrupts on the keys so that it doesn't keep triggering more interrupts
	  EXTI->IMR1 &= ~(COL0_Pin | COL1_Pin | COL2_Pin | COL3_Pin | COL4_Pin | COL5_Pin);

	  // switch pins to digital input instead of external interrupts
	  GPIO_InitTypeDef GPIO_InitStruct = {0};

	  GPIO_InitStruct.Pin = COL0_Pin|COL1_Pin|COL2_Pin|COL3_Pin
	                          |COL4_Pin|COL5_Pin;
	  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	  GPIO_InitStruct.Pull = GPIO_PULLDOWN;

	  // this function should be put in RAM since it is part of an interrupt routine
	  HAL_GPIO_Init(COL0_GPIO_Port, &GPIO_InitStruct);

	  // there's probably a better way to encode the row pins that an array
	uint16_t rows[] = { ROW0_Pin, ROW1_Pin, ROW2_Pin, ROW3_Pin, ROW4_Pin, ROW5_Pin, ROW6_Pin };

	// the key that was pressed, or 255 if no key was pressed.
	uint8_t key_press = 255;

	// reset all the row pins to 0 so that we can step through them 1 at a time
	ROW0_GPIO_Port->BRR = ROW0_Pin|ROW1_Pin|ROW2_Pin|ROW3_Pin|ROW4_Pin|ROW5_Pin|ROW6_Pin;
	for (uint8_t r = 0; r < 7; r++) {
		// set a row to high, then check if the column pin is high
		ROW0_GPIO_Port->BSRR = rows[r];
		uint32_t column_values = COL0_GPIO_Port->IDR&(COL0_Pin|COL1_Pin|COL2_Pin|COL3_Pin|COL4_Pin|COL5_Pin);
		ROW0_GPIO_Port->BRR = rows[r];

		int c = -1; // get the column number
		if (column_values != 0) {
			if (column_values&COL0_Pin != 0) c = 0;
			else if (column_values & COL1_Pin != 0) c = 1;
			else if (column_values & COL2_Pin != 0) c = 2;
			else if (column_values & COL3_Pin != 0) c = 3;
			else if (column_values & COL4_Pin != 0) c = 4;
			else if (column_values & COL5_Pin != 0) c = 5;
			// do some math to convert into a number from 1 to 37
			int row = r == 8 ? 5 : (r == 10 ? 6 : r);
			key_press = row * 5 + c + 1;
			if (row > 0) key_press++;
			if (row > 1) key_press++;
			if (row > 2) key_press++;
			if (key_press > 13) key_press--;

			break;
		}
	}

	// set all pins back to high so that a rising interrupt will be triggered again when the button changes state
	ROW0_GPIO_Port->BSRR =  ROW0_Pin|ROW1_Pin|ROW2_Pin|ROW3_Pin|ROW4_Pin|ROW5_Pin|ROW6_Pin;

	// switch back to interrupt mode
		  GPIO_InitStruct.Pin = COL0_Pin|COL1_Pin|COL2_Pin|COL3_Pin
		                          |COL4_Pin|COL5_Pin;
		  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
		  GPIO_InitStruct.Pull = GPIO_PULLDOWN;

		  HAL_GPIO_Init(COL0_GPIO_Port, &GPIO_InitStruct);

		  // reset any pending external interrupts?
			EXTI->PR1 = (COL0_Pin | COL1_Pin | COL2_Pin | COL3_Pin | COL4_Pin | COL5_Pin);

			// renable external interrupts
	  EXTI->IMR1 |= (COL0_Pin | COL1_Pin | COL2_Pin | COL3_Pin | COL4_Pin | COL5_Pin);


	return key_press;


	/*
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
}*/
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
