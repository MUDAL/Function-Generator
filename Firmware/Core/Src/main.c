/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "ad9833.h"
#include "lcd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
    ST_HIGHLIGHT_WAVE = 0,
    ST_HIGHLIGHT_FREQ,
    ST_SELECT_WAVE,
    ST_SELECT_FREQ
}State;

typedef enum
{
    IDLE = 0,
    LONG_PRESS,
    QUICK_PRESS
}PressType;

typedef struct 
{
    GPIO_TypeDef* port;
    GPIO_PinState old_state;
    uint16_t pin;
    uint32_t time_pressed;
    uint32_t debounce_delay;   
}ButtonStruct;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEFAULT_FREQ    500 // Hz
#define MAX_FREQ_MHZ    10
#define WAVE_NAME_LEN   3
#define NUM_COLS        16
#define ROW_INACTIVE    "  "
#define ROW_HIGHLIGHT   "- "
#define ROW_SELECT      "->"
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

static bool Button_Debounce(ButtonStruct* button_ptr, GPIO_PinState expected_state)
{
    bool debounced = false;

    if(HAL_GPIO_ReadPin(button_ptr->port, button_ptr->pin) == expected_state)
    {
        HAL_Delay(button_ptr->debounce_delay);
        if(HAL_GPIO_ReadPin(button_ptr->port, button_ptr->pin) == expected_state)
        {
            debounced = true;
        }
    }
    return debounced;
}

static PressType Button_HandlePress(ButtonStruct* button_ptr)
{
    const uint32_t max_release_time = 350; // In milliseconds
    PressType return_value = IDLE;

    if(Button_Debounce(button_ptr, GPIO_PIN_RESET))
    {
        if(button_ptr->old_state == GPIO_PIN_SET)
        {
            button_ptr->old_state = GPIO_PIN_RESET;
            button_ptr->time_pressed = HAL_GetTick();
        }
        else
        {
            return_value = LONG_PRESS;
        }
    }
    else if(button_ptr->old_state == GPIO_PIN_RESET)
    {
        if(Button_Debounce(button_ptr, GPIO_PIN_SET))
        {
            button_ptr->old_state = GPIO_PIN_SET;
            if((HAL_GetTick() - button_ptr->time_pressed) < max_release_time)
            {
                return_value = QUICK_PRESS;
            }
        }
    }
    return return_value;
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
  MX_I2C1_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */

  State state = ST_HIGHLIGHT_WAVE;

  ButtonStruct top_button = {.debounce_delay = 25,
                             .old_state = GPIO_PIN_SET,
                             .pin = SW_TOP_Pin,
                             .port = SW_TOP_GPIO_Port,
                             .time_pressed = 0};

  ButtonStruct center_button = {.debounce_delay = 25,
                                .old_state = GPIO_PIN_SET,
                                .pin = SW_CENTER_Pin,
                                .port = SW_CENTER_GPIO_Port,
                                .time_pressed = 0};

  ButtonStruct bottom_button = {.debounce_delay = 25,
                                .old_state = GPIO_PIN_SET,
                                .pin = SW_BOTTOM_Pin,
                                .port = SW_BOTTOM_GPIO_Port,
                                .time_pressed = 0};

  const char space = ' ';
  char row1_buffer[NUM_COLS + 1] = "- WAVE:";
  char row2_buffer[NUM_COLS + 1] = "  FREQ:";

  // Starting points for data to be displayed on both LCD rows
  uint8_t row1_buffer_data_index = strlen(row1_buffer);
  uint8_t row2_buffer_data_index = strlen(row2_buffer);

  const char wave_names[3][WAVE_NAME_LEN + 1] = {"SIN", "SQR", "TRI"};
  const WaveDef waves[3] = {WAVE_SINE, WAVE_SQUARE, WAVE_TRIANGLE};
  uint8_t wave_select = 0;

  uint32_t frequency_Hz = DEFAULT_FREQ;
  uint32_t frequency_kHz = frequency_Hz  / 1000;
  uint32_t frequency_MHz = frequency_kHz / 1000;

  // Factor to speed up or slow down frequency increments/decrements
  uint8_t factor_select = 0;
  uint32_t factors[6] = {1, 10, 100, 1000, 10000, 100000};

  // String manipulation for LCD data display
  size_t freq_str_len = 0;
  char freq_str[11] = {0};
  sprintf(freq_str, "%ld.00%s", frequency_Hz, "Hz");

  // Preparing default content to be displayed on the LCD
  memset(row1_buffer + row1_buffer_data_index, space,
         NUM_COLS - row1_buffer_data_index);

  memcpy(row1_buffer + row1_buffer_data_index,
         wave_names[wave_select],
         WAVE_NAME_LEN);

  memcpy(row2_buffer + row2_buffer_data_index,
         freq_str,
         strlen(freq_str));

  // Hardware initializations
  LCD_Init();
  AD9833_Init(WAVE_SINE, frequency_Hz, 0);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      switch(state)
      {
          case ST_HIGHLIGHT_WAVE:
              memcpy(row1_buffer, ROW_HIGHLIGHT, 2);
              memcpy(row2_buffer, ROW_INACTIVE,  2);

              if(Button_HandlePress(&center_button) == QUICK_PRESS)
              {
                  state = ST_SELECT_WAVE;
              }
              else if(Button_HandlePress(&bottom_button) == QUICK_PRESS)
              {
                  state = ST_HIGHLIGHT_FREQ;
              }
              break;

          case ST_HIGHLIGHT_FREQ:
              memcpy(row1_buffer, ROW_INACTIVE,  2);
              memcpy(row2_buffer, ROW_HIGHLIGHT, 2);

              if(Button_HandlePress(&center_button) == QUICK_PRESS)
              {
                  state = ST_SELECT_FREQ;
              }
              else if(Button_HandlePress(&top_button) == QUICK_PRESS)
              {
                  state = ST_HIGHLIGHT_WAVE;
              }
              break;

          case ST_SELECT_WAVE:
              memcpy(row1_buffer, ROW_SELECT,   2);
              memcpy(row2_buffer, ROW_INACTIVE, 2);

              if(Button_HandlePress(&center_button) == QUICK_PRESS)
              {
                  state = ST_HIGHLIGHT_WAVE;
              }
              else if(Button_HandlePress(&top_button) == QUICK_PRESS)
              {
                  if(wave_select == 2)
                  {
                      wave_select = 0;
                  }
                  else
                  {
                      wave_select++;
                  }
              }
              else if(Button_HandlePress(&bottom_button) == QUICK_PRESS)
              {
                  if(wave_select == 0)
                  {
                      wave_select = 2;
                  }
                  else
                  {
                      wave_select--;
                  }
              }
              
              AD9833_SetWaveform(waves[wave_select]);

              memcpy(row1_buffer + row1_buffer_data_index,
                     wave_names[wave_select],
                     WAVE_NAME_LEN);
              break;

          case ST_SELECT_FREQ:
              memcpy(row1_buffer, ROW_INACTIVE, 2);
              memcpy(row2_buffer, ROW_SELECT,   2);

              if(Button_HandlePress(&center_button) == QUICK_PRESS)
              {
                  state = ST_HIGHLIGHT_FREQ;
              }
              else
              {
                  PressType top_button_press = IDLE;
                  PressType bottom_button_press = IDLE;

                  top_button_press = Button_HandlePress(&top_button);
                  bottom_button_press = Button_HandlePress(&bottom_button);

                  switch(top_button_press)
                  {
                      case IDLE:
                          break;
                      case LONG_PRESS:
                          if(frequency_MHz != MAX_FREQ_MHZ)
                          {
                              frequency_Hz += factors[factor_select];
                          }
                          break;
                      case QUICK_PRESS:
                          if(factor_select != 5)
                          {
                              factor_select++;
                          }
                          break;
                  }

                  switch(bottom_button_press)
                  {
                      case IDLE:
                          break;
                      case LONG_PRESS:
                          if(frequency_Hz > factors[factor_select])
                          {
                              frequency_Hz -= factors[factor_select];
                          }
                          else
                          {
                              frequency_Hz = 0;
                          }
                          break;
                      case QUICK_PRESS:
                          if(factor_select != 0)
                          {
                              factor_select--;
                          }
                          break;
                  }
              }

              AD9833_SetFrequency(frequency_Hz);
              
              // Quotients
              frequency_kHz = frequency_Hz  / 1000;
              frequency_MHz = frequency_kHz / 1000;

              // Divide by 10 to allow 2 decimal point frequency display
              uint32_t remainder_kHz = lround((frequency_Hz  % 1000) / 10.0);
              uint32_t remainder_MHz = lround((frequency_kHz % 1000) / 10.0);

              if(frequency_Hz < 1000)
              {
                  sprintf(freq_str, "%ld.00%s", frequency_Hz, "Hz");
              }
              else if(frequency_kHz < 1000)
              {
                  if(remainder_kHz < 10)
                  {
                      sprintf(freq_str, "%ld.0%ld%s", frequency_kHz, remainder_kHz, "kHz");
                  }
                  else if(remainder_kHz >= 10 && remainder_kHz < 100)
                  {
                      sprintf(freq_str, "%ld.%ld%s", frequency_kHz, remainder_kHz, "kHz");
                  }
              }
              else if(frequency_MHz < 1000)
              {
                  if(remainder_MHz < 10)
                  {
                      sprintf(freq_str, "%ld.0%ld%s", frequency_MHz, remainder_MHz, "MHz");
                  }
                  else if(remainder_MHz >= 10 && remainder_MHz < 100)
                  {
                      sprintf(freq_str, "%ld.%ld%s", frequency_MHz, remainder_MHz, "MHz");
                  }
              }

              freq_str_len = strlen(freq_str);

              memset(row2_buffer + row2_buffer_data_index, space,
                     NUM_COLS - row2_buffer_data_index);

              memcpy(row2_buffer + row2_buffer_data_index, freq_str, freq_str_len);
              break;

          default:
              break;
      }
      
      // Last column of 1st row to show frequency update speed
      row1_buffer[NUM_COLS - 1] = factor_select + '0';

      LCD_SetCursor(0, 0);
      LCD_WriteString(row1_buffer);
      LCD_SetCursor(1, 0);
      LCD_WriteString(row2_buffer);
      
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
