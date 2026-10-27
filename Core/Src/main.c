/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bsp_led.h"
#include "stdio.h"
#include "math.h"
#include "st7735.h"
#include "fonts.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint32_t tick_counter = 0; // счетчик миллисекунд
uint16_t pwm_value = 0; // Текущая яркость
uint8_t led_state = 0;
uint8_t rx_buffer;// 0 = переход 1->2, 1 = переход 2->3, 2 = переход 3->1
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define PI 3.14159265f
void OnButtonPressed(void);
#ifdef __GNUC__
int __io_putchar(int ch)
{
	HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
	return ch;
}
#endif

uint8_t Read_Reg(uint16_t DevAddress, uint8_t RegAddress)
{
	uint8_t data = 0;
	// DevAddress уже должен быть сдвинут влево на 1 бит (0x68 << 1)
	HAL_I2C_Mem_Read(&hi2c1, DevAddress, RegAddress, 1, &data, 1, 100);
	return data;
}

void Write_Reg(uint16_t DevAddress, uint8_t RegAddress, uint8_t Data)
{
	HAL_I2C_Mem_Write(&hi2c1, DevAddress, RegAddress, 1, &Data, 1, 100);
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
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
	// MyBSP_LED_Init();
	// Запускаем ШИМ на всех трёх каналах
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);

	HAL_TIM_Base_Start_IT(&htim3); // Запускаем TIM3 с прерыванием (тик каждую 1 мс)

	printf("\r\n--- STM32 UART is ready! ---\r\n");

	//Запускаем приём 1 байта в прерывание
	//uint8_t rx_data;
	HAL_UART_Receive_IT(&huart2, &rx_buffer, 1);

	//Запускаем АЦП (без прерываний, просто в фоне)
	HAL_ADC_Start(&hadc1);

	/* Сканер датчика MPU6050 */

	printf("--- I2c Scanner ---\r\n");
	for (uint8_t addr = 1; addr < 128; addr++)
	{
		// Проверяем, отвечает ли датчик на этот адрес
		if(HAL_I2C_IsDeviceReady(&hi2c1, (addr << 1), 1, 10) == HAL_OK)
		{
			printf("Found I2C device at address: 0x%02X\r\n", addr);
		}

	}

	ST7735_Init();
	ST7735_FillScreen(ST7735_BLACK);

	//Вводим текст
	ST7735_WriteString(0,0, "INCLINOMETER", Font_11x18, ST7735_YELLOW, ST7735_BLACK);
	ST7735_WriteString(0,30, "Roll:", Font_11x18, ST7735_WHITE, ST7735_BLACK);
	ST7735_WriteString(0,60, "Pitch:", Font_11x18, ST7735_WHITE, ST7735_BLACK);
  /* USER CODE END 2 */

  /* Initialize leds */
  //BSP_LED_Init(LED2);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  //BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
			float comp_roll = 0.0f;
			float comp_pitch = 0.0f;
			uint32_t last_time = HAL_GetTick(); // Запоминаем время для интеграции
			char buf[32]; // Буфер для форматирования текста

	while (1) {


		uint8_t accel_buf[6];
		uint8_t gyro_buf[6];

		// 1. Читаем акселерометр (адрес 0x3B) и гироскоп (адрес 0x43)
		HAL_I2C_Mem_Read(&hi2c1, (0x68 << 1), 0x3B, 1, accel_buf, 6, 100);
		HAL_I2C_Mem_Read(&hi2c1, (0x68 << 1), 0x43, 1, gyro_buf, 6, 100);

		// 2. Склеиваем байты
		int16_t acc_x = (accel_buf[0] << 8) | accel_buf[1];
		int16_t acc_y = (accel_buf[2] << 8) | accel_buf[3];
		int16_t acc_z = (accel_buf[4] << 8) | accel_buf[5];

		int16_t gyr_x = (gyro_buf[0] << 8) | gyro_buf[1];
		int16_t gyr_y = (gyro_buf[2] << 8) | gyro_buf[3];

		// 3. Считаем время, прошедшее с прошлого цикла (dt в секундах)
		uint32_t now = HAL_GetTick();
		float dt = (now - last_time) / 1000.0f;
		last_time = now;

		//4. Считаем углы акселерометра
		float roll_acc = atan2(acc_y / 16384.0f, acc_z / 16384.0f) * (180.0f / PI);
		float pitch_acc = atan2(-acc_x / 16384.0f, sqrt((acc_y/16384.0f)*(acc_y/16384.0f) + (acc_z/16384.0f)*(acc_z/16384.0f))) * (180.0f / PI);

		//5. Считаем углы гироскопа (интегрируем скорость)
		//Делим на 131.0 для перевода в °/сек
		float roll_gyro = comp_roll + ((gyr_x / 131.0f) * dt);
		float pitch_gyro = comp_pitch + ((gyr_y / 131.0f) * dt);

		/* КОМПЛЕМЕНТАРНЫЙ ФИЛЬТР (MAGIC!) */
		// 98% доверия гироскопу (для быстрых движений) и 2% акселерометру (для устранения дрейфа)
		comp_roll = 0.98f * roll_gyro + 0.02f * roll_acc;
		comp_pitch = 0.98f * pitch_gyro + 0.02f * pitch_acc;

		/* ВЫВОД НА ДИСПЛЕЙ  */
		// Если угол будет превышать 30°, то строка будет КРАСНОЙ, если нет, то ЗЕЛЕНЫЙ
		uint16_t color_roll = (fabs(comp_roll) > 30.0f) ? ST7735_RED : ST7735_GREEN;
		uint16_t color_pitch = (fabs(comp_pitch) > 30.0f) ? ST7735_RED : ST7735_GREEN;

		// Формируем строку с пробелами в конце, чтобы затирать старые цифры
		sprintf(buf, "%6.1f ", comp_roll);
		ST7735_WriteString(60,30, buf, Font_11x18, color_roll, ST7735_BLACK); // 60 - отступ по Х

		sprintf(buf, "%6.1f ", comp_pitch);
		ST7735_WriteString(75,60, buf, Font_11x18, color_pitch, ST7735_BLACK); // 75 - отступ по Х

		HAL_Delay(20);

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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
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
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_144CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 179;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 179;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA8 PA9 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/*void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM3) {
		tick_counter++;

		//Каждые 5 миллисекунд (скорость анимации)
		if (tick_counter >= 5) {
			tick_counter = 0;

			// Увеличиваем яркость с шагом 10
			pwm_value += 10;
			if (pwm_value > 1000)
				pwm_value = 1000;

			// Логика перетекания
			if (led_state == 0) //переход 1->2
					{

				//__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 1000 - pwm_value);
				__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, pwm_value);
				__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 0);
			} else if (led_state == 1) // переход 2 -> 3
					{
				//__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
				__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 1000 - pwm_value);
				__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, pwm_value);
			} else if (led_state == 2) //переход 3 -> 1
					{
				//__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pwm_value);
				__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
				__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 1000 - pwm_value);
			}

			// Если дошли до максимума, переключаем на следующий переход
			if (pwm_value >= 1000) {
				pwm_value = 0;
				led_state++;
				if (led_state > 2)
					led_state = 0;
			}
		}
	}
}*/

/*void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

	static uint32_t last_time = 0;
	uint32_t now = HAL_GetTick();

	if (GPIO_Pin == BUTTON_USER_Pin && (now - last_time > 50)) {
		last_time = now;
	}


	if (GPIO_Pin == BUTTON_USER_Pin) {
		if (HAL_GPIO_ReadPin(BUTTON_USER_GPIO_Port, BUTTON_USER_Pin) == GPIO_PIN_RESET) {
			printf("Button: PRESSED\r\n");
			MyBSP_LED_On(LED_EXT);
		} else {
			printf("Button: RELEASED\r\n");
			MyBSP_LED_Off(LED_EXT);
		}
	}
}*/

//uint8_t rx_buffer;

/*void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART2)
	{

		//Return symbol
		HAL_UART_Transmit(&huart2, &rx_buffer, 1, 100);

		// rx_buffer содержит принятый символ
		if (rx_buffer == '1')
		{
			MyBSP_LED_On(LED_EXT);
			printf("LED ON\r\n");
		}
		else if (rx_buffer == '0')
		{
			MyBSP_LED_Off(LED_EXT);
			printf("LED OFF\r\n");
		}
		else
		{
			printf("Unknown command: %c\r\n", rx_buffer);
		}
		// ВАЖНО: снова запускаем приём следующего байта!!!
		HAL_UART_Receive_IT(&huart2, &rx_buffer, 1);
	}
}*/

void HAL__UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART2)
	{
		//Если ошибка - перезапускам приём
		HAL_UART_Receive_IT(&huart2, &rx_buffer, 1);

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
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
