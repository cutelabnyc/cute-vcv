/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "math.h"
#include "pulsar.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUFFER_SIZE 1024
#define SAMPLE_RATE 48214
#define LED_PIN GPIO_PIN_7
#define LED_GPIO_PORT GPIOB
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

I2S_HandleTypeDef hi2s2;
DMA_HandleTypeDef hdma_spi2_tx;

/* USER CODE BEGIN PV */
__attribute__((aligned(32))) int16_t audioBuffer[BUFFER_SIZE];
volatile uint8_t halfBufferReady = 0;  // Flag for half buffer completion
volatile uint8_t fullBufferReady = 0;  // Flag for full buffer completion
volatile uint32_t readCounter = 0;
volatile uint32_t writeCounter = 0;
float phase = 0;
float frequency = 200;
ps_t pulsar;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2S2_Init(void);
/* USER CODE BEGIN PFP */
void enableCycleCounter();
uint32_t getCycleCount();
void fillNoise(int16_t *buffer, int size);
float fillSine(int16_t *buffer, int size, float freq, float phase,
               float sr); // returns the phase for the next call
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  pulsar_init(&pulsar, SAMPLE_RATE);
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */
  // enableCycleCounter();
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2S2_Init();
  /* USER CODE BEGIN 2 */

  HAL_StatusTypeDef status = HAL_I2S_Transmit_DMA(&hi2s2, audioBuffer, BUFFER_SIZE);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  float scaleFactor = (1.0 / 256.0) * 32768;

DWT->CYCCNT = 0;  // Reset the cycle counter
uint32_t number1 = 100;
uint32_t number2 = 100;
uint32_t number3 = number1 + number2;  // Perform addition
uint32_t elapsed_cycles = DWT->CYCCNT;  // Read elapsed cycles
printf("Elapsed cycles: %lu\n", elapsed_cycles);

  while (1) {

    if (halfBufferReady) {

      pulsar_configure(&pulsar, 400, 0.8, 5.0, 0.0, 0, 0, 0);
      for (int i = 0; i < BUFFER_SIZE / 2; i+=2) {
        // DWT->CYCCNT = 0;  // Reset the cycle counter
        audioBuffer[i] = scaleFactor * pulsar_process(&pulsar, 400, 0);
        audioBuffer[i + 1] = audioBuffer[i];
        // uint32_t endCycles = DWT->CYCCNT;
        // printf("Cycles: %lu\n", endCycles);
      }

      halfBufferReady = 0;
      // SCB_CleanDCache_by_Addr((uint32_t *)audioBuffer, (BUFFER_SIZE / 2) * sizeof(uint16_t));
      writeCounter++;
    }

    if (fullBufferReady) {
      pulsar_configure(&pulsar, 400, 0.8, 5.0, 0.0, 0, 0, 0);
      for (int i = BUFFER_SIZE / 2; i < BUFFER_SIZE; i+=2) {
        audioBuffer[i] = scaleFactor * pulsar_process(&pulsar, 400, 0);
        audioBuffer[i + 1] = audioBuffer[i];
      }
      // SCB_CleanDCache_by_Addr((uint32_t *) &audioBuffer[BUFFER_SIZE / 2], (BUFFER_SIZE / 2) * sizeof(uint16_t));
      fullBufferReady = 0;
    }

    // turn on the LED if the write counter falls behind the read counter
    if (readCounter - writeCounter > 1) {
      HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_SET);
    } else {
      HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_RESET);
    }

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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO2, RCC_MCO2SOURCE_PLLI2SCLK, RCC_MCODIV_1);
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_PLLI2S|RCC_PERIPHCLK_I2S;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 216;
  PeriphClkInitStruct.PLLI2S.PLLI2SP = RCC_PLLP_DIV2;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 7;
  PeriphClkInitStruct.PLLI2S.PLLI2SQ = 2;
  PeriphClkInitStruct.PLLI2SDivQ = 1;
  PeriphClkInitStruct.I2sClockSelection = RCC_I2SCLKSOURCE_PLLI2S;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2S2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S2_Init(void)
{

  /* USER CODE BEGIN I2S2_Init 0 */

  /* USER CODE END I2S2_Init 0 */

  /* USER CODE BEGIN I2S2_Init 1 */

  /* USER CODE END I2S2_Init 1 */
  hi2s2.Instance = SPI2;
  hi2s2.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s2.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s2.Init.DataFormat = I2S_DATAFORMAT_16B_EXTENDED;
  hi2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  hi2s2.Init.AudioFreq = I2S_AUDIOFREQ_22K;
  hi2s2.Init.CPOL = I2S_CPOL_LOW;
  hi2s2.Init.ClockSource = I2S_CLOCK_PLL;
  if (HAL_I2S_Init(&hi2s2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S2_Init 2 */

  // __HAL_LINKDMA(&hi2s2, hdmatx, hdma_spi2_tx);

  /* USER CODE END I2S2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void enableCycleCounter() {
    // // Enable the DWT unit
    // if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
    //     CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // Enable trace and debug block
    // }

    // // Reset and enable the cycle counter
    // DWT->CYCCNT = 0;               // Reset the cycle counter
    // DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; // Enable the cycle counter

    CoreDebug->DEMCR &= ~0x01000000;

    CoreDebug->DEMCR |=  0x01000000; // TRCENA

    DWT->LAR = 0xC5ACCE55; // unlock

    DWT->CYCCNT = 0; // reset the counter

    DWT->CTRL &= ~0x00000001;

    DWT->CTRL |=  0x00000001; // enable the counter
}

uint32_t getCycleCount() {
    return DWT->CYCCNT; // Return the current cycle count
}

void fillNoise(int16_t *buffer, int size) {}

float fillSine(int16_t *buffer, int size, float freq, float phase, float sr) {
  static float twopi = 2 * 3.14159;
  static float scaleFactor = (1.0 / 64.0) * 32768;
  float phaseIncrement = twopi * freq / sr;
  for (int i = 0; i < size; i += 2) {
    buffer[i] = scaleFactor * arm_sin_f32(phase);
    // buffer[i] = scaleFactor * sinf(phase);
    // arm_sin_f32(phase, &buffer[i]);
    buffer[i + 1] = buffer[i];
    phase += phaseIncrement;
  }

  if (phase > twopi) {
    phase -= twopi;
  }

  return phase;
}
// Callback for Half Transfer Complete
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
  if (hi2s->Instance==SPI2) {
    halfBufferReady = 1;
    // SCB_InvalidateDCache_by_Addr((uint32_t *)audioBuffer, (BUFFER_SIZE / 2) * sizeof(uint16_t));
    readCounter++;
  }
  // audioBufferPtr = audioBuffer;
  // phase = fillSine(audioBufferPtr, BUFFER_SIZE / 2, 2000, phase, SAMPLE_RATE);
  // dataReady = 1;
}

// Callback for Full Transfer Complete
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
  if (hi2s->Instance==SPI2) {
    // SCB_InvalidateDCache_by_Addr((uint32_t *) &audioBuffer[BUFFER_SIZE / 2], (BUFFER_SIZE / 2) * sizeof(uint16_t));
    fullBufferReady = 1;
  }
  // audioBufferPtr = audioBuffer + BUFFER_SIZE / 2;
  // audioBufferPtr = audioBuffer;
  // phase = fillSine(audioBufferPtr, BUFFER_SIZE / 2, 2000, phase, SAMPLE_RATE);
  // dataReady = 1;
}

void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
    uint32_t error = HAL_I2S_GetError(hi2s);
    // printf("I2S Error: %lu\n", error);
}
/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

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
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
