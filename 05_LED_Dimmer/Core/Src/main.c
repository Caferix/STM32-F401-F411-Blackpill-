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
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* USER CODE BEGIN PV */
uint8_t dutyCycle = 0;    // LED'in parlaklık oranı (%0 - %100)
uint32_t pwmTick = 0;     // PWM periyodunu takip etmek için zaman damgası
uint32_t onTime = 0;      // Periyot içinde LED'in yanık kalacağı süre (ms)
#define PWM_PERIOD 10     // Toplam periyot süresi: 10ms (Yani saniyede 100 kez döner)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void delay_us(uint32_t us) {
    uint32_t startTick = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - startTick) < ticks);
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
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
DWT->CYCCNT = 0;
DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  while (1)
  {
// AŞAMA A : GetTick() kullanarak buton ile duty cycle kontrolü

    // uint32_t now = HAL_GetTick(); // İşlemcinin açıldığından beri geçen süreyi ms olarak al.
    // uint32_t phase = now % PWM_PERIOD;

    // onTime = (PWM_PERIOD * dutyCycle) / 100; // Periyodun neresinde olduğumuza karar veriyoruz

    // if(phase > onTime){
    //   HAL_GPIO_WritePin(GPIOA, LED0_Pin|LED1_Pin|LED2_Pin|LED3_Pin|LED4_Pin, GPIO_PIN_RESET);

    // }else{
    //   HAL_GPIO_WritePin(GPIOA, LED0_Pin|LED1_Pin|LED2_Pin|LED3_Pin|LED4_Pin, GPIO_PIN_SET);
    // }


    // if(HAL_GPIO_ReadPin(GPIOB, BUTTON_Pin) == GPIO_PIN_SET){
    //   HAL_Delay(50);
    //   dutyCycle +=25;
    //   if(dutyCycle > 100 ) dutyCycle = 0;

    //   while(HAL_GPIO_ReadPin(GPIOB, BUTTON_Pin) == GPIO_PIN_SET);
    //   HAL_Delay(50);
    // }

    // AŞAMA B: busy-wait döngü tabanlı pwm

    // 1. Hesaplamalar
    // uint32_t totalPeriod = 10000;              // 10ms
    // uint32_t highTime = dutyCycle * 100;       // %25 ise 2500us
    // uint32_t lowTime = totalPeriod - highTime; // %25 ise 7500us

    // // 2. LED ON (High Sinyali)
    // if (highTime > 0) {
    //     HAL_GPIO_WritePin(GPIOA, LED0_Pin|LED1_Pin|LED2_Pin|LED3_Pin|LED4_Pin, GPIO_PIN_RESET);
    //     delay_us(highTime);
    // }

    // // 3. LED OFF (Low Sinyali)
    // if (lowTime > 0) {
    //     HAL_GPIO_WritePin(GPIOA, LED0_Pin|LED1_Pin|LED2_Pin|LED3_Pin|LED4_Pin, GPIO_PIN_SET);  
    //     delay_us(lowTime);
    // }

    // // 4. BUTON OKUMA (İşte sorun burada başlayacak)
    // if (HAL_GPIO_ReadPin(GPIOB, BUTTON_Pin) == GPIO_PIN_SET) {
    //     dutyCycle += 25;
    //     if (dutyCycle > 100) dutyCycle = 0;
        
    //     while(HAL_GPIO_ReadPin(GPIOB, BUTTON_Pin) == GPIO_PIN_SET); // Bırakana kadar bekle
    // }


  // AŞAMA C (BUTONSUZ PWM - MERAKTAN :) )

  // 1. PWM Üretimi (Mikrosaniye Hassasiyetinde)
// DWT->CYCCNT bize işlemcinin her bir tikini verir. 
// 84MHz'de saniyede 84 milyon tik demektir. (vaow)

uint32_t totalTicks = 10000 * (SystemCoreClock / 1000000); // 10ms'lik toplam tik sayısı
uint32_t currentTick = DWT->CYCCNT % totalTicks;           // 10ms'lik periyodun neresindeyiz?
uint32_t activeTicks = (dutyCycle * totalTicks) / 100;     // Yanık kalması gereken tik sayısı

if (currentTick < activeTicks) {
    HAL_GPIO_WritePin(GPIOA, LED0_Pin|LED1_Pin|LED2_Pin|LED3_Pin|LED4_Pin, GPIO_PIN_RESET);
} else {
    HAL_GPIO_WritePin(GPIOA, LED0_Pin|LED1_Pin|LED2_Pin|LED3_Pin|LED4_Pin, GPIO_PIN_SET);
}

// 2. Nefes Alma Güncellemesi (Milisaniye ile devam edebiliriz)
static uint32_t lastUpdate = 0;
static int8_t direction = 1;
uint32_t now = HAL_GetTick();

if (now - lastUpdate >= 10) { // Her 10ms'de bir %1 değişim 
    lastUpdate = now;
    dutyCycle += direction;
    
    if (dutyCycle >= 100) direction = -1;
    if (dutyCycle <= 0)   direction = 1;
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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
