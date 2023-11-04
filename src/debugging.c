/*
 * debugging.c
 *
 *  Created on: Oct 19, 2023
 *      Author: danne
 */


#include "debugging.h"

void print_uart(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);

    char sensor_string_buff[150];

    vsprintf(sensor_string_buff, fmt, args);

    HAL_UART_Transmit(&UART_DEFINED, (uint8_t*)sensor_string_buff, strlen(sensor_string_buff), 100);

    va_end(args);
}

void print_lora(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);

    char lora_string_buf[150];

    vsprintf(lora_string_buf, fmt, args);

    HAL_UART_Transmit(&LORA_UART, (uint8_t*)lora_string_buf, strlen(lora_string_buf), 100);

    va_end(args);
}
