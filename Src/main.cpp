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

#include <bits/stdc++.h>
#include <stdbool.h>
#include <string.h>

#include "fatfs.h"
#include "lwip.h"
#include "lwip/apps/httpd.h"

CAN_HandleTypeDef hcan1;
CAN_TxHeaderTypeDef TxHeader;
CAN_RxHeaderTypeDef RxHeader;
uint8_t TxData[128];
uint8_t RxData[128];
uint32_t TxMailbox;
HAL_StatusTypeDef err;

long unsigned int ExtIdSave = 0x500000;
unsigned int delaySave = 10;

std::vector<std::vector<unsigned int>> txVector;
std::vector<std::vector<unsigned int>> rxVector;

UART_HandleTypeDef huart3;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

bool LD1ON = false;  // this variable will indicate if the LD1 LED on the board is ON or not
bool LD2ON = false;  // this variable will indicate if our LD2 LED on the board is ON or not
bool LD3ON = false;  // this variable will indicate if our LD3 LED on the board is ON or not

const char *LedCGIhandler(int iIndex, int iNumParams, char *pcParam[],
                          char *pcValue[]);

const tCGI LedCGI = {"/abm.cgi", LedCGIhandler};

tCGI theCGItable[1];

u16_t mySSIHandler(int iIndex, char *pcInsert, int iInsertLen);

#define numSSItags 6

char const *theSSItags[numSSItags] = {"id", "sent", "received", "sentjson", "receivedjson", "delay"};

std::string vectorToString(std::vector<std::vector<unsigned int>> vector) {
    std::stringstream ss;
    for (unsigned int i = 0; i < vector.size(); i++) {
        ss << "0x" << std::uppercase << std::hex << vector[i][0] << ',';
        ss << "0x" << std::uppercase << std::hex << vector[i][1];
        if (1 < vector[i].size() - 1) ss << ',';
        for (unsigned int j = 2; j < vector[i].size(); j++) {
            ss << "0x" << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << vector[i][j];
            if (j < vector[i].size() - 1) ss << ',';
        }
        if (i < vector.size() - 1) ss << std::endl;
    }
    return ss.str();
}

std::string vectorToJSON(std::vector<std::vector<unsigned int>> vector) {
    std::stringstream ss;
    ss << "[";
    for (unsigned int i = 0; i < vector.size(); i++) {
        ss << "{";
        ss << "\"rca\":"
           << "\"0x" << std::uppercase << std::hex << vector[i][0] << "\",";
        ss << "\"length\":"
           << "\"0x" << std::uppercase << std::hex << vector[i][1] << "\"";
        if (1 < vector[i].size() - 1) {
            ss << ",\"data\":[";
        }
        for (unsigned int j = 2; j < vector[i].size(); j++) {
            ss << "\"0x" << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << vector[i][j] << "\"";
            if (j < vector[i].size() - 1)
                ss << ",";
            else
                ss << "]";
        }
        ss << "}";
        if (i < vector.size() - 1) ss << ",";
    }
    ss << "]";
    return ss.str();
}

std::vector<std::vector<unsigned int>> tokenize(std::string str) {
    std::vector<std::vector<unsigned int>> out;
    std::string delimiter = "%2C";
    std::string delimiter2 = "%0D%0A";
    std::string subtoken(str);
    size_t pos = 0;
    size_t pos2 = 0;
    while ((pos = str.find(delimiter2)) != std::string::npos) {
        std::string token;
        subtoken = str.substr(0, pos);
        std::vector<unsigned int> temp;
        while ((pos2 = subtoken.find(delimiter)) != std::string::npos) {
            token = subtoken.substr(0, pos2);
            temp.push_back((unsigned int)std::stoi(token, nullptr, 0));
            subtoken.erase(0, pos2 + delimiter.length());
        }
        if (subtoken.size() > 0) temp.push_back((unsigned int)std::stoi(subtoken, nullptr, 0));
        if (temp.size() > 0) out.push_back(temp);
        str.erase(0, pos + delimiter2.length());
    }
    std::vector<unsigned int> temp;
    while ((pos2 = str.find(delimiter)) != std::string::npos) {
        std::string token;
        token = str.substr(0, pos2);
        temp.push_back((unsigned int)std::stoi(token, nullptr, 0));
        str.erase(0, pos2 + delimiter.length());
    }
    if (str.size() > 0) temp.push_back((unsigned int)std::stoi(str, nullptr, 0));
    if (temp.size() > 0) out.push_back(temp);
    return out;
}

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_CAN1_Init(void);

/* Private user code ---------------------------------------------------------*/
const char *LedCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    std::vector<std::vector<unsigned int>> commandsStore;

    bool outputJSON = false;

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

    for (int i = 0; i < iNumParams; i++) {
        if (strcmp(pcParam[i], "id") == 0) {
            if (strcmp(pcValue[i], "") == 0)
                ExtIdSave = 0x500000;
            else
                sscanf(pcValue[i], "0x%lx", &ExtIdSave);
            TxHeader.ExtId = ExtIdSave;
        } else if (strcmp(pcParam[i], "message") == 0) {
            std::string str(pcValue[i]);
            if (str.size() > 0) txVector = tokenize(str);
        } else if (strcmp(pcParam[i], "json") == 0) {
            if (strcmp(pcValue[i], "true") == 0) {
                outputJSON = true;
            }
        } else if (strcmp(pcParam[i], "delay") == 0) {
            if (strcmp(pcValue[i], "") == 0)
                delaySave = 10;
            else
                delaySave = (unsigned int)std::stoi(pcValue[i], nullptr, 0);
        }
    }
    rxVector.clear();
    for (unsigned int i = 0; i < txVector.size(); i++) {
        TxHeader.ExtId = ExtIdSave + txVector[i][0];
        TxHeader.DLC = txVector[i][1];
        for (unsigned int j = 2; j < txVector[i].size(); j++) {
            TxData[j - 2] = txVector[i][j];
        }
        err = HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
        if (err != HAL_OK) {
            Error_Handler();
        }
        if (txVector[i][1] == 0) {
            HAL_Delay(10);
            int rxFifoSize = HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0);
            if (rxFifoSize > 0) {
                for (int i = 0; i < rxFifoSize; i++) {
                    if (HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &RxHeader, RxData) != HAL_OK) {
                        /* Reception Error */
                        Error_Handler();
                    }
                    std::vector<unsigned int> temp;
                    temp.push_back(RxHeader.ExtId - ExtIdSave);
                    temp.push_back(RxHeader.DLC);
                    for (long unsigned int j = 0; j < RxHeader.DLC; j++) {
                        temp.push_back(RxData[j]);
                    }
                    rxVector.push_back(temp);
                }
            }
        }
        HAL_Delay(delaySave);
    }
    char tmp[20] = {0};
    sprintf(tmp, "Sent:%04u Recv:%04u", txVector.size(), rxVector.size());
    setCursor(0, 1);
    print(tmp);

    // the extension .shtml for SSI to work
    if (outputJSON) return "/results.json";
    return "/index.shtml";
}

void myCGIinit(void) {
    //add LED control CGI to the table
    theCGItable[0] = LedCGI;
    //give the table to the HTTP server
    http_set_cgi_handlers(theCGItable, 1);
}

u16_t mySSIHandler(int iIndex, char *pcInsert, int iInsertLen) {
    if (iIndex == 0) {
        std::stringstream ss;
        ss << "0x" << std::uppercase << std::hex << ExtIdSave;
        const char *myStr3 = ss.str().c_str();
        strcpy(pcInsert, myStr3);
        return strlen(myStr3);
    } else if (iIndex == 1) {
        std::string str = vectorToString(txVector);
        const char *myStr3 = str.c_str();
        strcpy(pcInsert, myStr3);
        return strlen(myStr3);
    } else if (iIndex == 2) {
        std::string str = vectorToString(rxVector);
        const char *myStr3 = str.c_str();
        strcpy(pcInsert, myStr3);
        return strlen(myStr3);
    } else if (iIndex == 3) {
        std::string str = vectorToJSON(txVector);
        const char *myStr3 = str.c_str();
        strcpy(pcInsert, myStr3);
        return strlen(myStr3);
    } else if (iIndex == 4) {
        std::string str = vectorToJSON(rxVector);
        const char *myStr3 = str.c_str();
        strcpy(pcInsert, myStr3);
        return strlen(myStr3);
    } else if (iIndex == 5) {
        std::string str = std::to_string(delaySave);
        const char *myStr3 = str.c_str();
        strcpy(pcInsert, myStr3);
        return strlen(myStr3);
    }
    return 0;
}

void mySSIinit(void) {
    http_set_ssi_handler(mySSIHandler, (char const **)theSSItags, numSSItags);
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {
    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();
    /* Configure the system clock */
    SystemClock_Config();
    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_USART3_UART_Init();
    MX_USB_OTG_FS_PCD_Init();
    MX_CAN1_Init();
    MX_LWIP_Init();
    MX_FATFS_Init();

    char str[20] = {0};
    LiquidCrystal(GPIOC, GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIOD, GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7);

    sprintf(str, "Initializing...");
    print(str);

    httpd_init();

    u32_t local_ip;

    while (1) {
        MX_LWIP_Process();
        local_ip = ip4_addr_get_u32(netif_ip4_addr(netif_list));
        if (local_ip != 0) break;
    }

    myCGIinit();
    mySSIinit();

    memset(str, 0, 20);
    sprintf(str, "%u.%u.%u.%u - OK ", (unsigned char)(local_ip), (unsigned char)(local_ip >> 8), (unsigned char)(local_ip >> 16), (unsigned char)(local_ip >> 24));
    setCursor(0, 0);
    print(str);

    /* Infinite loop */
    while (1) {
        MX_LWIP_Process();
    }
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
    CAN_FilterTypeDef sFilterConfig;

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

    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;

    if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_CAN_Start(&hcan1) != HAL_OK) {
        Error_Handler();
    }

    TxHeader.StdId = 0x0;
    TxHeader.ExtId = 0x0;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_EXT;
    TxHeader.DLC = 2;
    TxHeader.TransmitGlobalTime = DISABLE;
}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void) {
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
}

/**
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_PCD_Init(void) {
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
}

/**
  * @brief EXTI line detection callbacks
  * @param GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == USER_Btn_Pin) {
        /* Toggle LED1 */
        HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
    }
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
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0x0F, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

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
