#include "uart_lab.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdint.h>
#include <stdio.h>

const uint32_t clock_U2 = (1 << 24);
const uint32_t clock_U3 = (1 << 25);

const uint32_t DLAB_reg = (1 << 7);

const uint32_t set_U2_Tx = 0b001;
const uint32_t set_U2_Rx = 0b001;

const uint32_t set_U3_Tx = 0b010;
const uint32_t set_U3_Rx = 0b010;

const uint8_t Rx_Data_Ready = (1 << 0);
const uint8_t Tx_Data_Ready = (1 << 5);

const uint8_t word_length = 3;

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
  // Refer to LPC User manual and setup the register bits correctly
  // The first page of the UART chapter has good instructions
  const uint16_t divider_16_bit = peripheral_clock * 1000 * 1000 / (16 * baud_rate);

  if (uart == 0) {
    // Power on Peripheral
    // LPC_SC->PCONP |= clock_U2;
    lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__UART2);

    // Setup DLL, DLM, FDR, LCR registers
    LPC_UART2->LCR |= DLAB_reg;
    LPC_UART2->DLM = (divider_16_bit >> 8) & 0xFF;
    LPC_UART2->DLL = (divider_16_bit >> 0) & 0xFF;
    LPC_UART2->LCR &= ~DLAB_reg;

    // Set pin as output
    LPC_IOCON->P0_10 |= set_U2_Tx;
    LPC_IOCON->P0_11 |= set_U2_Rx;

    // Word Length
    LPC_UART2->LCR = word_length;

  } else if (uart == 1) {
    // Power on Peripheral
    // LPC_SC->PCONP |= clock_U3;
    lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__UART3);

    // Setup DLL, DLM, FDR, LCR registers
    LPC_UART3->LCR |= DLAB_reg;
    LPC_UART3->DLM = (divider_16_bit >> 8) & 0xFF;
    LPC_UART3->DLL = (divider_16_bit >> 0) & 0xFF;
    LPC_UART3->LCR &= ~DLAB_reg;

    // Set pin as output
    LPC_IOCON->P4_28 |= set_U3_Tx;
    LPC_IOCON->P4_29 |= set_U3_Rx;

    // Word Length
    LPC_UART3->LCR = word_length;
  }
}

// Read the byte from RBR and actually save it to the pointer
char uart_lab__polled_get(uart_number_e uart) {

  char output_byte = 0;

  if (uart == 0) {
    // Check LSR for Receive Data Ready
    check_Rx_status(uart);
    // Copy data from RBR register to input_byte
    output_byte = LPC_UART2->RBR;
  }

  else if (uart == 1) {
    check_Rx_status(uart);
    // Copy data from RBR register to input_byte
    output_byte = LPC_UART3->RBR;
  }
  return output_byte;
}

bool uart_lab__polled_put(uart_number_e uart, char output_byte) {

  bool status = false;
  // Copy output_byte to THR register

  // Check LSR for Transmit Hold Register Empty
  if (uart == 0) {
    check_Tx_status(uart);
    LPC_UART2->THR = output_byte;
    status = true;
    check_Tx_status(uart);
  }

  else if (uart == 1) {

    check_Tx_status(uart);
    LPC_UART3->THR = output_byte;
    status = true;
    check_Tx_status(uart);
  }

  return status;
}

void check_Rx_status(uart_number_e uart) {
  if (uart == 0) {
    while (!(Rx_Data_Ready & LPC_UART2->LSR)) {
      fprintf(stderr, "Receiver Buffer is full");
    }
  } else if (uart == 1) {
    while (!(Rx_Data_Ready & LPC_UART3->LSR)) {
      fprintf(stderr, "Receiver Buffer is full");
    }
  }
}

void check_Tx_status(uart_number_e uart) {
  if (uart == 0) {
    while (!(LPC_UART2->LSR & Tx_Data_Ready)) {
      // Keep Looping until its not empty;
    }
  } else if (uart == 1) {
    while (!(LPC_UART3->LSR & Tx_Data_Ready)) {
      // Keep Looping until its not empty;
    }
  }
}
