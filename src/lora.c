/*
 * lora.c
 *
 *  Created on: Oct 19, 2023
 *      Author: danne
 */


#include "lora.h"

void create_lora_string_and_send_data(uint16_t* vel_array, uint16_t *vel_index) {
    print_uart("CLOCK! \r\n ");

    char data_string[57] = "AT+MSG="; // Initialize with prefix
    int str_length = strlen(data_string);

    for(int i = 0 ; i < *vel_index  ; i++) {
        print_uart("i %d: %d\r\n", i, vel_array[i]);
    }

    for(int i = 0; i < *vel_index; i++) {

        int len;
        len = snprintf(NULL, 0, "%dX", vel_array[i]);

        // If adding the current number would exceed the limit,
        // print the current string and start over
        if(str_length + len > 50){
            data_string[str_length] = '\0'; // Proper null-terminator
            print_uart("Now sending %s to LoRa\r\n", data_string);
            send_message(data_string);
            strcpy(data_string, "AT+MSG="); // New line prefix
            str_length = strlen(data_string); // Reset the length
        }

        // Add the current number to the string
        sprintf(data_string + str_length, "%dX", vel_array[i]);
        str_length += len;
    }

    // Print the last string if it contains something
    if (str_length > strlen("AT+MSG=")) {
        data_string[str_length] = '\0'; // Proper null-terminator
        print_uart("Now sending %s to LoRa\r\n", data_string);
        send_message(data_string);
    }

    memset(vel_array, 0, VEL_ARRAY_SIZE * sizeof(uint16_t));
    memset(data_string, 0, sizeof(data_string));
    *vel_index = 0;
}


#define DELAY 5000

void join_lora_network(void){
	print_lora("AT+ID=DevEui,\"%s\"", DEVEUI);
	check_for_send_success(2000);

	print_lora("AT+ID=AppEui,\"%s\"", APPEUI);
	check_for_send_success(2000);

	print_lora("AT+KEY=APPKEY,\"%s\"", "ABCDABCDABCDABCDABCDABCDABCDABCD");
	check_for_send_success(2000);

	print_lora("AT+DR=EU868");
	check_for_send_success(2000);

	print_lora("AT+CH=NUM,0-2");
	check_for_send_success(2000);

	print_lora("AT+MODE=LWOTAA");
	check_for_send_success(2000);

	// Try and join until joined
	do{
		print_lora("AT+JOIN");
	} while(!check_for_send_success(15000));
}

void send_message(const char str[]){
	print_uart("%s", str);
	print_lora("%s", str);
	check_for_send_success(2500);
	check_for_send_success(12000);
}

bool check_for_send_success(uint16_t TIME_WAIT){
    char uart_receive_buffert[RECEIVE_BUFFER_SIZE];
    memset(uart_receive_buffert, 0, RECEIVE_BUFFER_SIZE);

    HAL_UART_Receive(&LORA_UART, (uint8_t*)uart_receive_buffert, RECEIVE_BUFFER_SIZE, TIME_WAIT);

    print_uart("\r\nBuffer: %s", uart_receive_buffert);

    // Check for "fail" in the received buffer
    char* substr_fail = strstr(uart_receive_buffert, "fail");
    if (substr_fail != NULL) {
        print_uart("Found 'fail' at position: %d\n", substr_fail - uart_receive_buffert);
        return false;
    }

    // Check for "RX: " in the received buffer
    char* substr_rx = strstr(uart_receive_buffert, "RX: \"");
    if (substr_rx != NULL) {
        int value1, value2;

        // Try parsing two characters first
        if (sscanf(substr_rx, "RX: \"%2x%2x\"", &value1, &value2) == 2) {
            // Convert each individual ASCII value to its corresponding digit
            int digit1 = value1 - '0';
            int digit2 = value2 - '0';

            // Combine the two digits
            rtc_interval = digit1 * 10 + digit2;
        }
        else if (sscanf(substr_rx, "RX: \"%2x\"", &value1) == 1) {
            // Single character/digit case
            rtc_interval = value1 - '0';
        }

        print_uart("Extracted value: %d\n", rtc_interval);
        change_rtc_interval(rtc_interval);
    }

    // Clear the receive buffer
    memset(uart_receive_buffert, 0, RECEIVE_BUFFER_SIZE);

    return true;
}
