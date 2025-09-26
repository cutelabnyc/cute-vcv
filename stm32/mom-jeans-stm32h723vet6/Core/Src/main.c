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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "pulsar.h"
#include "math.h"
#include "pulsar_lut.h"
#include "pulsar_calibration.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
__attribute__((aligned(4)))
typedef struct {
    uint32_t magic_number;                // Verification magic
    mom_jeans_calibration_t calibration;  // Calibration data
    uint32_t version;                     // Data structure version
    uint32_t checksum;                    // Simple checksum for integrity
} calibration_data_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUFFER_SIZE 64
#define SAMPLE_RATE 44070.512820513
#define DEBUG_LED_PIN GPIO_PIN_2
#define DEBUG_LED_GPIO_PORT GPIOE

#define INDEX_DETECT_PORT GPIOB
#define INDEX_DETECT_PIN GPIO_PIN_12

#define LED_GPIO_PORT GPIOC
#define CADENCE_LED_PIN GPIO_PIN_6
#define PITCH_LED_PIN GPIO_PIN_7
#define CADENCE_PWM_CHANNEL TIM_CHANNEL_1
#define PITCH_PWM_CHANNEL TIM_CHANNEL_2

#define MUX_SEL_GPIO_PORT GPIOD
#define MUX_S0_PIN GPIO_PIN_12
#define MUX_S1_PIN GPIO_PIN_13
#define MUX_S2_PIN GPIO_PIN_14
#define MUX_COM_GPIO_PORT GPIOC
#define MUX_COM_PIN GPIO_PIN_4
#define MUX2_COM_PIN GPIO_PIN_5

#define MUX_ADC_CHANNELS 2
#define DIRECT_ADC_CHANNELS 2
#define AUDIO_ADC_CHANNELS 2
#define AUDIO_ADC_BUFFER_SIZE (BUFFER_SIZE * AUDIO_ADC_CHANNELS / 2) // remember that the buffer has two channels

#define CALIBRATION_MAGIC       0xCAFEBABE
#define CALIBRATION_VERSION     1
#define CALIBRATION_SECTOR      FLASH_SECTOR_3        // Last sector for 512KB part
#define CALIBRATION_FLASH_ADDR  0x08060000            // Start of last sector (384KB offset)
#define CALIBRATION_SECTOR_SIZE 0x20000               // 128KB sector size
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
ADC_HandleTypeDef hadc3;
DMA_HandleTypeDef hdma_adc1;
DMA_HandleTypeDef hdma_adc2;
DMA_HandleTypeDef hdma_adc3;

I2S_HandleTypeDef hi2s1;
DMA_HandleTypeDef hdma_spi1_tx;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim8;

/* USER CODE BEGIN PV */
int ledState = 0;
int counter = 0;
float debug_value;
__attribute__((section(".sram1"), aligned(32))) int16_t audioBuffer[BUFFER_SIZE];
__attribute__((section(".sram1"), aligned(32))) uint16_t muxBuffer[MUX_ADC_CHANNELS];  // DMA buffer for potentiometer ADC data
__attribute__((section(".sram1"), aligned(32))) uint16_t adcBuffer[DIRECT_ADC_CHANNELS];  // DMA buffer for direct ADC data
__attribute__((section(".sram1"), aligned(32))) uint16_t audioInputBuffer[AUDIO_ADC_BUFFER_SIZE];  // DMA buffer for direct ADC data
uint8_t muxChannel = 0;  // Current MUX channel being read
uint16_t mux1Data[8];  // Buffer for MUX1 data
uint16_t mux2Data[8];  // Buffer for MUX2 data
volatile uint8_t halfBufferReady = 0;  // Flag for half buffer completion
volatile uint8_t fullBufferReady = 0;  // Flag for full buffer completion
volatile uint32_t readCounter = 0;
volatile uint32_t writeCounter = 0;
volatile uint32_t adc1ReadCounter = 0;
volatile uint32_t adc2ReadCounter = 0;
volatile uint32_t adc3ReadCounter = 0;
ps_t ps; // pulsar object
pulsar_state_t pulsar_state; // pulsar state
mom_jeans_calibration_t calibration; // pulsar calibration structure
mom_jeans_reading_t reading; // pulsar reading structure
uint8_t calibrating = 1;
uint8_t calibrationComplete = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC1_Init(void);
static void MX_ADC3_Init(void);
static void MX_TIM1_Init(void);
static void MX_ADC2_Init(void);
static void MX_I2S1_Init(void);
static void MX_TIM8_Init(void);
/* USER CODE BEGIN PFP */
void enableCycleCounter();
uint32_t getCycleCount();
void setMultiplexerChannel(uint8_t channel);
float fillSine(int16_t *buffer, int size, float freq, float phase, float sr);
float makePulseFrequency(float pitch, float v_oct, float linear_fm, float fm_index, float *previous_value);
float hysteresis(float value, float hysteresis, float *previous_value);
void SetLEDIntensity(uint16_t intensity, uint32_t channel);
void copyReadings(mom_jeans_reading_t *reading);
void appendCalibration(mom_jeans_reading_t *reading, mom_jeans_calibration_t *calibration);
void fillPulsarState(mom_jeans_reading_t *reading, mom_jeans_calibration_t *calibration, pulsar_state_t *state);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static float scaleLin(float x, float a, float b, float c, float d) {
	return (x - a) / (b - a) * (d - c) + c;
}

static float clip(float value, float min, float max) {
  if (value < min) {
    return min;
  } else if (value > max) {
    return max;
  } else {
    return value;
  }
}

static float scaleJackInput(uint16_t x, uint16_t in_min, uint16_t in_zero, uint16_t in_max, float out_low, float out_high) {
  if (x < in_zero) {
    return scaleLin(x, in_min, in_zero, out_low, 0.0f);
  } else if (x > in_zero) {
    return scaleLin(x, in_zero, in_max, 0.0f, out_high);
  } else {
    return 0.0f; // If the value is exactly zero
  }
}

/**
 * @brief Calculate simple checksum for calibration data
 */
static uint32_t calculate_checksum(const mom_jeans_calibration_t* data) {
    uint32_t checksum = 0;
    const uint32_t* ptr = (const uint32_t*)data;
    size_t words = (sizeof(mom_jeans_calibration_t) - sizeof(uint32_t)) / sizeof(uint32_t);

    for (size_t i = 0; i < words; i++) {
        checksum ^= ptr[i];
    }
    return checksum;
}

/**
 * @brief Erase calibration sector
 * @return HAL_OK on success, error code on failure
 */
HAL_StatusTypeDef calibration_erase_sector(void) {
    HAL_StatusTypeDef status;
    
    // Unlock flash for erase operation
    HAL_FLASH_Unlock();
    
    // Configure erase parameters
    FLASH_EraseInitTypeDef erase_init;
    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.Sector = CALIBRATION_SECTOR;          // FLASH_SECTOR_3
    erase_init.NbSectors = 1;                        // Erase only one sector
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3; // 2.7V to 3.6V
    
    uint32_t sector_error = 0;
    
    // Perform the erase operation
    status = HAL_FLASHEx_Erase(&erase_init, &sector_error);
    
    // Lock flash after operation
    HAL_FLASH_Lock();
    
    // Check if erase was successful
    if (status != HAL_OK) {
        // Optional: You can check sector_error for more details
        // sector_error will contain the sector number that failed (if any)
        return status;
    }
    
    return HAL_OK;
}

/**
 * @brief Read calibration data from flash
 * @return 1 if valid calibration data found, 0 otherwise
 */
uint8_t calibration_read_from_flash(mom_jeans_calibration_t *calibration) {
    // Read data directly from flash memory
    const calibration_data_t* flash_data = (const calibration_data_t*)CALIBRATION_FLASH_ADDR;
    const calibration_data_t calibration_copy;
    
    // Check magic number
    if (flash_data->magic_number != CALIBRATION_MAGIC) {
        return 0;
    }
    
    // Check version compatibility
    if (flash_data->version != CALIBRATION_VERSION) {
        return 0;
    }
    
    // Copy data to RAM for checksum calculation
    memcpy(&calibration_copy, flash_data, sizeof(calibration_data_t));
    
    // Verify checksum
    uint32_t calculated_checksum = calculate_checksum(&calibration_copy.calibration);
    if (calibration_copy.checksum != calculated_checksum) {
        return 0;
    }
    
    memcpy(calibration, &calibration_copy.calibration, sizeof(mom_jeans_calibration_t));
    return 1;
}

/**
 * @brief Write calibration data to flash
 * @return HAL_OK on success, error code on failure
 */
HAL_StatusTypeDef calibration_write_to_flash(mom_jeans_calibration_t *calibration) {
    HAL_StatusTypeDef status;
    
    calibration_data_t flash_calibration;
    flash_calibration.magic_number = CALIBRATION_MAGIC;
    flash_calibration.version = CALIBRATION_VERSION;

    // Update checksum before writing
    flash_calibration.checksum = calculate_checksum(calibration);
    memcpy(&flash_calibration.calibration, calibration, sizeof(mom_jeans_calibration_t));

    // Unlock flash
    HAL_FLASH_Unlock();
    
    // Erase the sector
    FLASH_EraseInitTypeDef erase_init;
    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.Sector = CALIBRATION_SECTOR;
    erase_init.NbSectors = 1;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;  // 2.7V to 3.6V
    
    uint32_t sector_error;
    status = HAL_FLASHEx_Erase(&erase_init, &sector_error);
    if (status != HAL_OK) {
        HAL_FLASH_Lock();
        return status;
    }
    
    // Write data word by word (STM32H7 requires 256-bit writes, but HAL handles this)
  uint32_t data_ptr = (uint32_t)&flash_calibration;
  uint32_t address = CALIBRATION_FLASH_ADDR;
  size_t struct_size = sizeof(calibration_data_t);
  size_t chunk_size = 32; // FLASHWORD writes 32 bytes

  for (size_t offset = 0; offset < struct_size; offset += chunk_size) {
      status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, address, data_ptr);
      if (status != HAL_OK) {
          HAL_FLASH_Lock();
          return status;
      }
      address += chunk_size;
      data_ptr += chunk_size;
  }
    
    // Lock flash
    HAL_FLASH_Lock();
    
    return HAL_OK;
}

void _processAudioBlock(int offset, uint8_t *lastResync, float *lastPitch)
{
  static const float scaleFactor = (-1.0 / 2.0) * 32768;
  static uint32_t triggerOutputCounter = 0;

  copyReadings(&reading);
  fillPulsarState(&reading, &calibration, &pulsar_state);
  
  float trigger_output = 0.0f;
  float internal_lfo_phase = 0.0f;

  for (int i = offset; i < BUFFER_SIZE / 2 + offset; i+=2) {
    int idx = i >> 1;
    int inputIdx = idx * AUDIO_ADC_CHANNELS + 1; // sync input
    uint8_t resync = audioInputBuffer[inputIdx] < 1800 ? 1 : 0; // resync if the input is below a threshold

    uint16_t linear_fm_jack = audioInputBuffer[inputIdx - 1]; // linear fm input
    pulsar_state.linear_fm = jack_to_float(
      linear_fm_jack,
      &calibration.linear_fm_jack,
      1.0f,
      -1.0f
    );

    float finalPitch = makePulseFrequency(
      pulsar_state.pitch,
      pulsar_state.v_oct,
      pulsar_state.linear_fm,
      1.0,
      lastPitch
    ); // Convert to frequency

    if (i == offset) {
      pulsar_configure(
        &ps,
        finalPitch,
        pulsar_state.density,
        pulsar_state.cadence,
        pulsar_state.torque,
        pulsar_state.waveform,
        pulsar_state.quantization,
        pulsar_state.coupling
      );
    }

    audioBuffer[i] = scaleFactor * pulsar_process(&ps, finalPitch, resync > *lastResync, &debug_value);
    trigger_output = pulsar_get_sync_output(&ps); // Get the sync output from pulsar

    if (trigger_output > 0.5f) {
      triggerOutputCounter = SAMPLE_RATE * 0.5 / (finalPitch + 1.0f); // Set the trigger output counter to 1/10th of a second
    } else {
      if (triggerOutputCounter > 0) {
        triggerOutputCounter--;
      }
    }

    audioBuffer[i + 1] = (triggerOutputCounter > 0 ? scaleFactor : 0);
    *lastResync = resync;
  }
  
  int16_t ledIntensity = 0;
  if (audioBuffer[0] > 0) {
    ledIntensity = triggerOutputCounter > 0 ? 999 : 0; // If trigger output is high, set LED intensity to 999
  }
  SetLEDIntensity(ledIntensity, PITCH_PWM_CHANNEL);
  
  ledIntensity = 0;
  internal_lfo_phase = pulsar_get_internal_lfo_phase(&ps); // Get the internal LFO phase
  float lfoIntensity = arm_sin_f32(internal_lfo_phase * PI * 2);
  if (lfoIntensity < 0) {
    ledIntensity = scaleLin(lfoIntensity, 0.0, -1.0, 0, 999); // Scale the intensity from 0 to 999
  }
  SetLEDIntensity(ledIntensity, CADENCE_PWM_CHANNEL);
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

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM3_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  MX_ADC3_Init();
  MX_TIM1_Init();
  MX_ADC2_Init();
  MX_I2S1_Init();
  MX_TIM8_Init();
  /* USER CODE BEGIN 2 */
  HAL_StatusTypeDef status;
  enableCycleCounter();

  // uint32_t timer_clock = HAL_RCC_GetPCLK2Freq(); // or GetPCLK1Freq()
  // uint32_t prescaler = htim1.Init.Prescaler;
  // uint32_t period = htim1.Init.Period; 
  // uint32_t repetition = htim1.Init.RepetitionCounter;
  // uint32_t actual_freq = timer_clock / ((prescaler + 1) * (period + 1) * (repetition + 1));

  // ADC1
  status = HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
  status = HAL_ADC_Start_DMA(&hadc1, (uint32_t *) muxBuffer, MUX_ADC_CHANNELS);
  HAL_TIM_Base_Start_IT(&htim2);

  // ADC2
  status = HAL_ADCEx_Calibration_Start(&hadc2, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
  status = HAL_ADC_Start_DMA(&hadc2, (uint32_t *) adcBuffer, DIRECT_ADC_CHANNELS);
  HAL_TIM_Base_Start(&htim3);

  // ADC3
  for (int i = 0; i < AUDIO_ADC_BUFFER_SIZE; i++) {
    audioInputBuffer[i] = 0; // Initialize the audio input buffer to zero
  }
  status = HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
  status = HAL_ADC_Start_DMA(&hadc3, (uint32_t *) audioInputBuffer, AUDIO_ADC_BUFFER_SIZE);
  HAL_TIM_Base_Start(&htim1);  

  // TIM8 PWM
  HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);

  if ((DMA1_Stream0->CR & DMA_SxCR_EN) == 0) {
    status = 1;
  }

  if ((DMA1_Stream1->CR & DMA_SxCR_EN) == 0) {
    status = 1;
  }

  if ((DMA2_Stream0->CR & DMA_SxCR_EN) == 0) {
    status = 1;
  }

  HAL_GPIO_WritePin(MUX_SEL_GPIO_PORT, MUX_S0_PIN | MUX_S1_PIN | MUX_S2_PIN, GPIO_PIN_RESET);

  fillSine(audioBuffer, BUFFER_SIZE, 200, 0, SAMPLE_RATE);
  status = HAL_I2S_Transmit_DMA(&hi2s1, (const uint16_t *) audioBuffer, BUFFER_SIZE);

  pulsar_init(&ps, SAMPLE_RATE);

  // This is where you'd load calibration data if you had any

  if (0) {
    calibration_erase_sector();
  }

  uint8_t calibration_status = calibration_read_from_flash(&calibration);
  if (!calibration_status) {
    calibrating = 1;
    calibrationComplete = 0;
    mom_jeans_zero_calibration(&calibration);
  } else {
    calibrating = 0;
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint8_t lastResync = 0;
  float lastPitch = 0.0f; // Initialize last pitch for makePulseFrequency

  while (1)
  {
    if (halfBufferReady) {
      if (!calibrating) {
        _processAudioBlock(0, &lastResync, &lastPitch);
      }
      halfBufferReady = 0;
      writeCounter++;

      if (calibrating && adc1ReadCounter > 8) {
        copyReadings(&reading);
        appendCalibration(&reading, &calibration);
        if (mom_jeans_is_calibrated(&calibration)) {
          calibrating = 0;
          calibrationComplete = 1;
          calibration_write_to_flash(&calibration);
        }
      }
    }

    if (fullBufferReady) {
      if (!calibrating) {
        _processAudioBlock(BUFFER_SIZE / 2, &lastResync, &lastPitch);
      }
      fullBufferReady = 0;
    }

    // turn on the LED if the write counter falls behind the read counter
    if (readCounter - writeCounter > 1) {
      HAL_GPIO_WritePin(DEBUG_LED_GPIO_PORT, DEBUG_LED_PIN, GPIO_PIN_RESET);
    } else {
      HAL_GPIO_WritePin(DEBUG_LED_GPIO_PORT, DEBUG_LED_PIN, GPIO_PIN_SET);
    }
    // HAL_GPIO_WritePin(DEBUG_LED_GPIO_PORT, DEBUG_LED_PIN, GPIO_PIN_RESET);
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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = 64;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 34;
  RCC_OscInitStruct.PLL.PLLP = 1;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 3072;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
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
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInitStruct.PLL2.PLL2M = 4;
  PeriphClkInitStruct.PLL2.PLL2N = 12;
  PeriphClkInitStruct.PLL2.PLL2P = 2;
  PeriphClkInitStruct.PLL2.PLL2Q = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 2;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
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

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_16B;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 2;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIG_T2_TRGO;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  hadc1.Init.OversamplingMode = ENABLE;
  hadc1.Init.Oversampling.Ratio = 4;
  hadc1.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_2;
  hadc1.Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
  hadc1.Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_CONTINUED_MODE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  sConfig.OffsetSignedSaturation = DISABLE;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */

  /** Common config
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc2.Init.Resolution = ADC_RESOLUTION_16B;
  hadc2.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc2.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc2.Init.LowPowerAutoWait = DISABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.NbrOfConversion = 2;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv = ADC_EXTERNALTRIG_T3_TRGO;
  hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc2.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;
  hadc2.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc2.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  hadc2.Init.OversamplingMode = ENABLE;
  hadc2.Init.Oversampling.Ratio = 16;
  hadc2.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_4;
  hadc2.Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
  hadc2.Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_CONTINUED_MODE;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_10;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_387CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  sConfig.OffsetSignedSaturation = DISABLE;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_11;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

}

/**
  * @brief ADC3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC3_Init(void)
{

  /* USER CODE BEGIN ADC3_Init 0 */

  /* USER CODE END ADC3_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC3_Init 1 */

  /* USER CODE END ADC3_Init 1 */

  /** Common config
  */
  hadc3.Instance = ADC3;
  hadc3.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc3.Init.Resolution = ADC_RESOLUTION_12B;
  hadc3.Init.DataAlign = ADC3_DATAALIGN_RIGHT;
  hadc3.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc3.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc3.Init.LowPowerAutoWait = DISABLE;
  hadc3.Init.ContinuousConvMode = DISABLE;
  hadc3.Init.NbrOfConversion = 2;
  hadc3.Init.DiscontinuousConvMode = DISABLE;
  hadc3.Init.ExternalTrigConv = ADC_EXTERNALTRIG_T1_TRGO;
  hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc3.Init.DMAContinuousRequests = ENABLE;
  hadc3.Init.SamplingMode = ADC_SAMPLING_MODE_NORMAL;
  hadc3.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;
  hadc3.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc3.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  hadc3.Init.OversamplingMode = ENABLE;
  hadc3.Init.Oversampling.Ratio = ADC3_OVERSAMPLING_RATIO_16;
  hadc3.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_4;
  hadc3.Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
  hadc3.Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_CONTINUED_MODE;
  if (HAL_ADC_Init(&hadc3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC3_SAMPLETIME_47CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  sConfig.OffsetSign = ADC3_OFFSET_SIGN_NEGATIVE;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC3_Init 2 */

  /* USER CODE END ADC3_Init 2 */

}

/**
  * @brief I2S1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S1_Init(void)
{

  /* USER CODE BEGIN I2S1_Init 0 */

  /* USER CODE END I2S1_Init 0 */

  /* USER CODE BEGIN I2S1_Init 1 */

  /* USER CODE END I2S1_Init 1 */
  hi2s1.Instance = SPI1;
  hi2s1.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s1.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s1.Init.DataFormat = I2S_DATAFORMAT_16B_EXTENDED;
  hi2s1.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
  hi2s1.Init.AudioFreq = I2S_AUDIOFREQ_44K;
  hi2s1.Init.CPOL = I2S_CPOL_LOW;
  hi2s1.Init.FirstBit = I2S_FIRSTBIT_MSB;
  hi2s1.Init.WSInversion = I2S_WS_INVERSION_DISABLE;
  hi2s1.Init.Data24BitAlignment = I2S_DATA_24BIT_ALIGNMENT_RIGHT;
  hi2s1.Init.MasterKeepIOState = I2S_MASTER_KEEP_IO_STATE_DISABLE;
  if (HAL_I2S_Init(&hi2s1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S1_Init 2 */
  /* USER CODE END I2S1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 79;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 77;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

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
  htim2.Init.Prescaler = 1920- 1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 100 - 1;
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
  if (HAL_TIM_OC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC1;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 50;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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
  htim3.Init.Prescaler = 24-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 256-1;
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
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM8_Init(void)
{

  /* USER CODE BEGIN TIM8_Init 0 */

  /* USER CODE END TIM8_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM8_Init 1 */

  /* USER CODE END TIM8_Init 1 */
  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 275;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = 999;
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim8, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim8, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM8_Init 2 */

  /* USER CODE END TIM8_Init 2 */
  HAL_TIM_MspPostInit(&htim8);

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
  /* DMA1_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
  /* DMA1_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pin : PE2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB14 */
  GPIO_InitStruct.Pin = GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PD12 PD13 PD14 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void enableCycleCounter() {
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

void setMultiplexerChannel(uint8_t channel) {
  HAL_GPIO_WritePin(MUX_SEL_GPIO_PORT, GPIO_PIN_12, (channel & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MUX_SEL_GPIO_PORT, GPIO_PIN_13, (channel & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MUX_SEL_GPIO_PORT, GPIO_PIN_14, (channel & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

float fillSine(int16_t *buffer, int size, float freq, float phase, float sr) {
  static float twopi = 2 * 3.14159;
  static float scaleFactor = (1.0 / 4.0) * 32768;
  // float phaseIncrement = twopi * freq / sr;
  float phaseIncrement = twopi * 16.0f / size;
  for (int i = 0; i < size; i += 2) {
    // buffer[i] = scaleFactor * arm_sin_f32(phase);
    buffer[i] = scaleFactor * sinf(phase);
    // arm_sin_f32(phase, &buffer[i]);
    buffer[i + 1] = buffer[i];
    buffer[i + 1] = 0;
    phase += phaseIncrement;
  }

  if (phase > twopi) {
    phase -= twopi;
  }

  return phase;
}

float makePulseFrequency(float pitch, float v_oct, float linear_fm, float fm_index, float *previous_value)
{
  const float lfo_cutoff = 0.0f; // LFO cutoff frequency
  float pulse_frequency;

  if (pitch < lfo_cutoff) {
    pitch = scaleLin(pitch, 0.0, lfo_cutoff, -2.f, 4.78135971352f);
    pitch += v_oct + linear_fm * fm_index / 5.f;
    pulse_frequency = powf(2.f, pitch);
  } else {
    pitch = scaleLin(pitch, lfo_cutoff, 1.f, 0, 84);
    pitch = hysteresis(pitch, 0.5f, previous_value);
    pitch = roundf(pitch);
    pitch += (v_oct + linear_fm * fm_index / 5.f) * 12.0f;
    pulse_frequency = 27.5 * powf(2.f, pitch / 12.f);
  }

  return pulse_frequency;
}

float hysteresis(float value, float hysteresis, float *previous_value)
{
  if (fabsf(value - *previous_value) > hysteresis) {
    *previous_value = value;
  } else {
    value = *previous_value;
  }

  return value;
}

void SetLEDIntensity(uint16_t intensity, uint32_t channel) {
    // intensity: 0-999 for LED on given channel
    __HAL_TIM_SET_COMPARE(&htim8, channel, intensity);
}

void copyReadings(mom_jeans_reading_t *reading)
{
  reading->pitch_pot = mux1Data[2];
  reading->density_pot = mux1Data[1];
  reading->cadence_pot = mux1Data[3];
  reading->torque_pot = mux1Data[0];
  reading->waveform_pot = mux2Data[2];

  reading->density_jack = adcBuffer[0];
  reading->cadence_jack = mux2Data[7];
  reading->torque_jack = mux2Data[6];
  reading->waveform_jack = mux2Data[5];
  reading->voct_jack = adcBuffer[1];
  reading->linear_fm_jack = audioInputBuffer[1];
  reading->fm_index_jack = mux2Data[4];
  reading->sync_jack = audioInputBuffer[0];

  reading->coupling_switch = mux1Data[7];
  reading->quant_switch = mux1Data[4];
}

void appendCalibration(mom_jeans_reading_t *reading, mom_jeans_calibration_t *calibration)
{
  mom_jeans_append_calibration_reading(reading, calibration);
}

void fillPulsarState(mom_jeans_reading_t *reading, mom_jeans_calibration_t *calibration, pulsar_state_t *state)
{

//   float pitch;
//   float density;
//   float cadence;
//   float torque;
//   float waveform;

//   float v_oct;
//   float linear_fm;
//   float fm_index;

//   uint8_t sync;
//   uint8_t coupling;
//   uint8_t quantization;

// static inline float pot_and_jack_to_float(
//     uint16_t pot_value,
//     uint16_t jack_value,
//     potentiometer16u_t *pot,
//     jack16u_t *jack,
//     float min_value,
//     float max_value


  // float density_knob = scaleLin(mux1Data[1], 65535, 0, -1.0, 1.0); // Scale the density from 0 to 1
  // float waveform_knob = scaleLin(mux2Data[2], 65535, 0, 0.0, 5.0); // Scale the wave shape from 0 to 6
  // float ratio_knob = scaleLin(mux1Data[3], 65535, 0, 0.0, 1.0); // Scale the ratio from 0 to 1
  // float depth_knob = scaleLin(mux1Data[0], 65535, 0, 0.0, 1.0); // Scale the depth from 0 to 1
  // float pitch = scaleLin(mux1Data[2], 65535, 0, 0.0, 1.0); // Scale the pitch from 0 to 1

  // uint16_t density_jack = adcBuffer[0];
  // uint16_t v_oct_jack = adcBuffer[1];
  // uint16_t waveform_jack = mux2Data[5];
  // uint16_t index_jack = mux2Data[4];
  // uint16_t depth_jack = mux2Data[6];
  // uint16_t cadence_jack = mux2Data[7];

  // float density = clip(density_knob + scaleJackInput(density_jack, 100, 32868, 64000, 1.0, -1.0), -1.0f, 1.0f); // Combine the density from the knobs and jacks
  // float waveform = clip(waveform_knob + scaleJackInput(waveform_jack, 100, 32868, 64000, 6.0, -6.0), 0.0f, 4.9999f); // Combine the waveform from the knobs and jacks
  // float ratio = clip(ratio_knob + scaleJackInput(cadence_jack, 100, 32868, 64000, 1.0, -1.0), 0.0f, 1.0f); // Combine the ratio from the knobs and jacks
  // float depth = clip(depth_knob + scaleJackInput(depth_jack, 100, 32868, 64000, 1.0, -1.0), 0.0f, 1.0f); // Combine the depth from the knobs and jacks

  // uint8_t lock = mux1Data[4] > 32768 ? 1 : 0; // Lock the ratio if the value is above 32768
  // uint8_t couple = mux1Data[7] > 32768 ? 1 : 0; // Couple the frequency if the value is above 32768
  // float trigger_output = 0.0f;
  // float internal_lfo_phase = 0.0f;

  // for (int i = offset; i < BUFFER_SIZE / 2 + offset; i+=2) {
  //   // DWT->CYCCNT = 0;  // Reset the cycle counter
  //   int idx = i >> 1;
  //   int inputIdx = idx * AUDIO_ADC_CHANNELS + 1; // sync input
  //   uint8_t resync = audioInputBuffer[inputIdx] < 1800 ? 1 : 0; // resync if the input is below a threshold

  //   uint16_t linear_fm_jack = audioInputBuffer[inputIdx - 1]; // linear fm input
  //   float linear_fm = clip(scaleJackInput(linear_fm_jack, 122, 2059, 2583, 1.0, -1.0), -1.0f, 1.0f);
  //   float v_oct = clip(scaleJackInput(v_oct_jack, 100, 32868, 64000, 8.0f, -8.0f), -8.0f, 8.0f);

  state->pitch = pot_to_float(
    reading->pitch_pot,
    &calibration->pitch_pot,
    1.0f,
    0.0f
  );

  state->density = pot_and_jack_to_float(
    reading->density_pot,
    reading->density_jack,
    &calibration->density_pot,
    &calibration->density_jack,
    1.0f,
    -1.0f
  );

  state->cadence = pot_and_jack_to_float(
    reading->cadence_pot,
    reading->cadence_jack,
    &calibration->cadence_pot,
    &calibration->cadence_jack,
    1.0f,
    0.0f
  );

  state->torque = pot_and_jack_to_float(
    reading->torque_pot,
    reading->torque_jack,
    &calibration->torque_pot,
    &calibration->torque_jack,
    1.0f,
    0.0f
  );

  state->waveform = pot_and_jack_to_float(
    reading->waveform_pot,
    reading->waveform_jack,
    &calibration->waveform_pot,
    &calibration->waveform_jack,
    4.9999f,
    0.0f
  );

  state->v_oct = jack_to_float(
    reading->voct_jack,
    &calibration->voct_jack,
    8.0f,
    -8.0f
  );

  // Leave off linear_fm, since it's read at a faster rate
  // and computed per-sample

  uint8_t fm_index_detect = HAL_GPIO_ReadPin(INDEX_DETECT_PORT, INDEX_DETECT_PIN);

  if (fm_index_detect) {
    state->fm_index = 1.0f;
  } else {
    state->fm_index = jack_to_float(
    reading->fm_index_jack,
    &calibration->fm_index_jack,
    5.0f,
    0.0f
  );
  }

  // Leave off sync, since it's read at a faster rate
  // and computed per-sample

  state->coupling = switch_to_uint8(
    reading->coupling_switch,
    &calibration->coupling_switch
  );

  state->quantization = switch_to_uint8(
    reading->quant_switch,
    &calibration->quant_switch
  );
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM2) {
    // Timer for MUX channel switching
    setMultiplexerChannel(muxChannel);
  }
}

// Callback for ADC full transfer complete
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
  if (hadc->Instance == ADC1) {
    mux1Data[muxChannel] = muxBuffer[0];
    mux2Data[muxChannel] = muxBuffer[1];
    muxChannel = (muxChannel + 1) % 8;

    adc1ReadCounter++;
  }
  else if (hadc->Instance == ADC2) {
    adc2ReadCounter++;\
  }
  else if (hadc->Instance == ADC3) {
    adc3ReadCounter++;
  }
}

// Callback for Half Transfer Complete
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
  if (hi2s->Instance==SPI1) {
    halfBufferReady = 1;
    // SCB_InvalidateDCache_by_Addr((uint32_t *)audioBuffer, (BUFFER_SIZE / 2) * sizeof(uint16_t));
    readCounter++;
  }
}

// Callback for Full Transfer Complete
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
  if (hi2s->Instance==SPI1) {
    // SCB_InvalidateDCache_by_Addr((uint32_t *) &audioBuffer[BUFFER_SIZE / 2], (BUFFER_SIZE / 2) * sizeof(uint16_t));
    fullBufferReady = 1;
  }
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
