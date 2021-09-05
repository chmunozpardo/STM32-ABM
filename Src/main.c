/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
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

#include <Stdbool.h>
#include <string.h>

#include "fatfs.h"
#include "lwip.h"
#include "lwip/apps/httpd.h"

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
CAN_HandleTypeDef hcan1;
CAN_TxHeaderTypeDef TxHeader;
uint8_t TxData[128];
uint32_t TxMailbox;
HAL_StatusTypeDef err;

UART_HandleTypeDef huart3;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

UART_HandleTypeDef huart3;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

/* USER CODE BEGIN PV */

bool LD1ON = false;  // this variable will indicate if the LD1 LED on the board is ON or not
bool LD2ON = false;  // this variable will indicate if our LD2 LED on the board is ON or not
bool LD3ON = false;  // this variable will indicate if our LD3 LED on the board is ON or not

// just declaring the function for the compiler [= CGI #2 =]
const char *LedCGIhandler(int iIndex, int iNumParams, char *pcParam[],
                          char *pcValue[]);

// in our SHTML file <form method="get" action="/leds.cgi"> [= CGI #3 =]
const tCGI LedCGI = {"/leds.cgi", LedCGIhandler};

// [= CGI #4 =]
tCGI theCGItable[1];

// just declaring SSI handler function [* SSI #1 *]
u16_t mySSIHandler(int iIndex, char *pcInsert, int iInsertLen);

// [* SSI #2 *]
#define numSSItags 5

// [* SSI #3 *]
char const *theSSItags[numSSItags] = {"tag1", "tag2", "tag3", "tag4", "tag5"};

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_CAN1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// the actual function for handling CGI [= CGI #5 =]
const char *LedCGIhandler(int iIndex, int iNumParams, char *pcParam[],
                          char *pcValue[]) {
    uint32_t i = 0;

    if (iIndex == 0) {
        //turning the LED lights off
        HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
        // we put this variable to false to indicate that the LD1 LED on the board is not ON
        LD1ON = false;

        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
        // we put this variable to false to indicate that the LD2 LED on the board is not ON
        LD2ON = false;

        HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
        // we put this variable to false to indicate that the LD3 LED on the board is not ON
        LD3ON = false;
    }

    for (i = 0; i < iNumParams; i++) {
        if (strcmp(pcParam[i], "led") == 0) {
            if (strcmp(pcValue[i], "1") == 0) {
                HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);
                // LD1 LED (green) on the board is ON!
                LD1ON = true;
            } else if (strcmp(pcValue[i], "2") == 0) {
                HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
                // LD2 LED (blue) on the board is ON!
                LD2ON = true;
            } else if (strcmp(pcValue[i], "3") == 0) {
                HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
                // LD3 LED (red) on the board is ON!
                LD3ON = true;
            }
        } else if (strcmp(pcParam[i], "id") == 0) {
            unsigned int idValue;
            sscanf(pcValue[i], "0x%x", &idValue);
            TxHeader.StdId = (u_int8_t)idValue;
            TxHeader.ExtId = 0x01;
        } else if (strcmp(pcParam[i], "message") == 0) {
            unsigned int split_input;
            const char comma_decode[4] = "%2C";
            const char newline_decode[7] = "%0D%0A";
            char *comma_token;
            char *newline_token;
            int count = 0;
            newline_token = strtok(pcValue[i], newline_decode);
            while (newline_token != NULL) {
                comma_token = strtok(newline_token, comma_decode);
                while (comma_token != NULL) {
                    sscanf(comma_token, "0x%x", &split_input);
                    TxData[count++] = (u_int8_t)split_input;
                    comma_token = strtok(NULL, comma_decode);
                }
                newline_token = strtok(NULL, newline_decode);
            }
            TxHeader.DLC = count;
        }
    }

    err = HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
    if (err != HAL_OK) {
        Error_Handler();
    }

    HAL_Delay(10);
    // the extension .shtml for SSI to work
    return "/index.shtml";
}  // END [= CGI #5 =]

// function to initialize CGI [= CGI #6 =]
void myCGIinit(void) {
    //add LED control CGI to the table
    theCGItable[0] = LedCGI;
    //give the table to the HTTP server
    http_set_cgi_handlers(theCGItable, 1);
}  // END [= CGI #6 =]

// the actual function for SSI [* SSI #4 *]
u16_t mySSIHandler(int iIndex, char *pcInsert, int iInsertLen) {
    if (iIndex == 0) {
        if (LD1ON == false) {
            char myStr1[] = "<input value=\"1\" name=\"led\" type=\"checkbox\">";
            strcpy(pcInsert, myStr1);
            return strlen(myStr1);
        } else if (LD1ON == true) {
            // since the LD1 red LED on the board is ON we make its checkbox checked!
            char myStr1[] =
                "<input value=\"1\" name=\"led\" type=\"checkbox\" checked>";
            strcpy(pcInsert, myStr1);
            return strlen(myStr1);
        }
    } else if (iIndex == 1) {
        if (LD2ON == false) {
            char myStr2[] = "<input value=\"2\" name=\"led\" type=\"checkbox\">";
            strcpy(pcInsert, myStr2);
            return strlen(myStr2);
        } else if (LD2ON == true) {
            // since the LD2 blue LED on the board is ON we make its checkbox checked!
            char myStr2[] =
                "<input value=\"2\" name=\"led\" type=\"checkbox\" checked>";
            strcpy(pcInsert, myStr2);
            return strlen(myStr2);
        }
    } else if (iIndex == 2) {
        if (LD3ON == false) {
            char myStr3[] = "<input value=\"3\" name=\"led\" type=\"checkbox\">";
            strcpy(pcInsert, myStr3);
            return strlen(myStr3);
        } else if (LD3ON == true) {
            // since the LD3 blue LED on the board is ON we make its checkbox checked!
            char myStr3[] =
                "<input value=\"3\" name=\"led\" type=\"checkbox\" checked>";
            strcpy(pcInsert, myStr3);
            return strlen(myStr3);
        }
    } else if (iIndex == 3) {
        char myStr3[] = "<textarea style=\"resize:none\" name=\"id\" rows=\"1\" cols=\"10\"></textarea>";
        strcpy(pcInsert, myStr3);
        return strlen(myStr3);
    } else if (iIndex == 4) {
        char myStr3[] = "<textarea style=\"resize:none\" name=\"message\" rows=\"8\" cols=\"24\"></textarea>";
        strcpy(pcInsert, myStr3);
        return strlen(myStr3);
    }
    return 0;
}

// function to initialize SSI [* SSI #5 *]
void mySSIinit(void) {
    http_set_ssi_handler(mySSIHandler, (char const **)theSSItags, numSSItags);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {
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
    MX_USART3_UART_Init();
    MX_USB_OTG_FS_PCD_Init();
    MX_CAN1_Init();
    MX_LWIP_Init();
    MX_FATFS_Init();
    /* USER CODE BEGIN 2 */

    char str[20] = {0};
    LiquidCrystal(GPIOC, GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIOD, GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7);

    sprintf(str, "Initializing...");
    print(str);

    httpd_init();

    u32_t local_ip;

    while (1) {
        /* USER CODE END WHILE */
        MX_LWIP_Process();
        local_ip = ip4_addr_get_u32(netif_ip4_addr(netif_list));
        if (local_ip != 0) break;
        /* USER CODE BEGIN 3 */
    }

    // initializing CGI  [= CGI #7 =]
    myCGIinit();

    // initializing SSI [* SSI #6 *]
    mySSIinit();

    memset(str, 0, 20);
    sprintf(str, "%u.%u.%u.%u", (unsigned char)(local_ip), (unsigned char)(local_ip >> 8), (unsigned char)(local_ip >> 16), (unsigned char)(local_ip >> 24));
    setCursor(0, 0);
    print(str);

    /* USER CODE END 2 */
    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        /* USER CODE END WHILE */
        MX_LWIP_Process();
        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 13;
    RCC_OscInitStruct.PLL.PLLN = 195;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 5;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB buses clocks
  */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief CAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN1_Init(void) {
    /* USER CODE BEGIN CAN1_Init 0 */

    /* USER CODE END CAN1_Init 0 */

    /* USER CODE BEGIN CAN1_Init 1 */

    /* USER CODE END CAN1_Init 1 */
    hcan1.Instance = CAN1;
    hcan1.Init.TimeTriggeredMode = DISABLE;
    hcan1.Init.AutoBusOff = DISABLE;
    hcan1.Init.AutoWakeUp = DISABLE;
    hcan1.Init.AutoRetransmission = DISABLE;
    hcan1.Init.ReceiveFifoLocked = DISABLE;
    hcan1.Init.TransmitFifoPriority = DISABLE;
    hcan1.Init.Mode = CAN_MODE_NORMAL;
    hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan1.Init.TimeSeg1 = CAN_BS1_2TQ;
    hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
    hcan1.Init.Prescaler = 6;

    if (HAL_CAN_Init(&hcan1) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN CAN1_Init 2 */

    /* USER CODE END CAN1_Init 2 */
    if (HAL_CAN_Start(&hcan1) != HAL_OK) {
        /* Start Error */
        Error_Handler();
    }

    TxHeader.StdId = 0x321;
    TxHeader.ExtId = 0x01;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = 2;
    TxHeader.TransmitGlobalTime = DISABLE;
}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void) {
    /* USER CODE BEGIN USART3_Init 0 */

    /* USER CODE END USART3_Init 0 */

    /* USER CODE BEGIN USART3_Init 1 */

    /* USER CODE END USART3_Init 1 */
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 115200;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart3) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN USART3_Init 2 */

    /* USER CODE END USART3_Init 2 */
}

/**
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_PCD_Init(void) {
    /* USER CODE BEGIN USB_OTG_FS_Init 0 */

    /* USER CODE END USB_OTG_FS_Init 0 */

    /* USER CODE BEGIN USB_OTG_FS_Init 1 */

    /* USER CODE END USB_OTG_FS_Init 1 */
    hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
    hpcd_USB_OTG_FS.Init.dev_endpoints = 4;
    hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
    hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
    hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
    hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
    hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
    if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN USB_OTG_FS_Init 2 */

    /* USER CODE END USB_OTG_FS_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, LD1_Pin | LD3_Pin | LD2_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin : USER_Btn_Pin */
    GPIO_InitStruct.Pin = USER_Btn_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
    GPIO_InitStruct.Pin = LD1_Pin | LD3_Pin | LD2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
    GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : USB_OverCurrent_Pin */
    GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void) {
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
void assert_failed(uint8_t *file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
