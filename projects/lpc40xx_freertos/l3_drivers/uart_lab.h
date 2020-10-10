#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  UART_2,
  UART_3,
} uart_number_e;

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate);

// Read the byte from RBR and actually save it to the pointer
char uart_lab__polled_get(uart_number_e uart);

bool uart_lab__polled_put(uart_number_e uart, char output_byte);

void check_Tx_status(uart_number_e uart);

void check_Rx_status(uart_number_e uart);