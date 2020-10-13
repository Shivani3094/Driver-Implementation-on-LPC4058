#include "uart_lab.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"

#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "queue.h"

const uint32_t DLAB_reg = (1 << 7);

const uint32_t set_U2_Tx = 0b001;
const uint32_t set_U2_Rx = 0b001;

const uint32_t set_U3_Tx = 0b010;
const uint32_t set_U3_Rx = 0b010;

const uint8_t Rx_Data_Ready = (1 << 0);
const uint8_t Tx_Data_Ready = (1 << 5);

const uint8_t word_length = 3;

static QueueHandle_t uart_rx_queue;
static QueueHandle_t uart2_rx_queue;

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
  const uint16_t divider_16_bit = peripheral_clock * 1000 * 1000 / (16 * baud_rate);
  fprintf(stderr, "Baud Rate = %luHz\n", baud_rate);

  if (uart == 0) {
    // Power on Peripheral
    lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__UART2);

    // Setup DLL, DLM, FDR, LCR registers
    LPC_UART2->LCR |= DLAB_reg;
    LPC_UART2->DLM = (divider_16_bit >> 8) & 0xFF;
    LPC_UART2->DLL = (divider_16_bit >> 0) & 0xFF;
    LPC_UART2->LCR &= ~DLAB_reg;

    // Set pin as output
    LPC_IOCON->P0_10 &= ~(0b111 << 0);
    LPC_IOCON->P0_11 &= ~(0b111 << 0);
    LPC_IOCON->P0_10 |= set_U2_Tx;
    LPC_IOCON->P0_11 |= set_U2_Rx;

    // Word Length
    LPC_UART2->LCR = word_length;

  } else if (uart == 1) {
    // Power on Peripheral
    lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__UART3);

    // Setup DLL, DLM, FDR, LCR registers
    LPC_UART3->LCR |= DLAB_reg;
    LPC_UART3->DLM = (divider_16_bit >> 8) & 0xFF;
    LPC_UART3->DLL = (divider_16_bit >> 0) & 0xFF;
    LPC_UART3->LCR &= ~DLAB_reg;

    // Set pin as output
    LPC_IOCON->P4_28 &= ~(0b111 << 0);
    LPC_IOCON->P4_29 &= ~(0b111 << 0);
    LPC_IOCON->P4_28 |= set_U3_Tx;
    LPC_IOCON->P4_29 |= set_U3_Rx;

    // Word Length
    LPC_UART3->LCR = word_length;
  }
}

// Read the byte from RBR
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
  }
  return status;
}

void check_Rx_status(uart_number_e uart) {
  if (uart == 0) {
    while (!(Rx_Data_Ready & LPC_UART2->LSR)) {
      ;
    }
  } else if (uart == 1) {
    while (!(Rx_Data_Ready & LPC_UART3->LSR)) {
      ;
    }
  }
}

void check_Tx_status(uart_number_e uart) {
  if (uart == 0) {
    while (!(LPC_UART2->LSR & Tx_Data_Ready)) {
      // Keep Looping until its not empty;
    }
  } else if (uart == 1) {
    while (!(LPC_UART3->LSR & (1 << 5))) {

      // Keep Looping until its not empty;
    }
  }
}

static void receive_interrupt_U3(void) {

  // Read the IIR register
  const uint32_t read_iir_reg = LPC_UART3->IIR;
  const uint32_t read_IIR_status = read_iir_reg & (1 << 0);

  const uint32_t rx_fifo_ready = LPC_UART3->LSR;
  const uint32_t rx_data_read_ready = rx_fifo_ready & (1 << 0);

  char byte;
  // Based on IIR status, read the LSR register to confirm if there is data to be read
  if (!read_IIR_status) {
    if (rx_data_read_ready) {
      // Based on LSR status, read the RBR register and input the data to the RX Queue
      byte = LPC_UART3->RBR;
    } else {
      fprintf(stderr, "Rx Not Ready\n");
    }
  } else {
    fprintf(stderr, "Interrupt Not Pending\n");
  }
  xQueueSendFromISR(uart_rx_queue, &byte, NULL);
}

static void receive_interrupt_U2(void) {
  const uint32_t read_iir_reg = LPC_UART2->IIR;
  const uint32_t read_IIR_status = read_iir_reg & (1 << 0);

  const uint32_t rx_fifo_ready = LPC_UART2->LSR;
  const uint32_t rx_data_read_ready = rx_fifo_ready & (1 << 0);

  char byte;
  // Based on IIR status, read the LSR register to confirm if there is data to be read
  if (!read_IIR_status) {
    if (rx_data_read_ready) {
      // Based on LSR status, read the RBR register and input the data to the RX Queue
      byte = LPC_UART2->RBR;
    } else {
      fprintf(stderr, "Rx Not Ready\n");
    }
  } else {
    fprintf(stderr, "Interrupt Not Pending\n");
  }
  xQueueSendFromISR(uart2_rx_queue, &byte, NULL);
}

void uart__enable_receive_interrupt(uart_number_e uart_number) {

  if (uart_number == 1) {
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART3, receive_interrupt_U3, "UART3");
    NVIC_EnableIRQ(UART3_IRQn);

    // Enable UART receive interrupt
    const uint32_t Rx_Intr_Enable = (1 << 0);
    LPC_UART3->IER |= (Rx_Intr_Enable);

    // Create your RX queue
    uart_rx_queue = xQueueCreate(1, sizeof(char));
  } else if (uart_number == 0) {
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART2, receive_interrupt_U2, "UART2");
    NVIC_EnableIRQ(UART2_IRQn);

    // Enable UART receive interrupt
    const uint32_t Rx_Intr_Enable = (1 << 0);
    LPC_UART2->IER |= (Rx_Intr_Enable);

    // Create your RX queue
    uart2_rx_queue = xQueueCreate(1, sizeof(char));
  }
}

bool uart_lab__get_char_from_queue(uart_number_e uart_number, char *input_byte, uint32_t timeout) {

  bool temp = 0;

  if (uart_number == 1)
    temp = xQueueReceive(uart_rx_queue, input_byte, timeout);
  else if (uart_number == 0)
    temp = xQueueReceive(uart2_rx_queue, input_byte, timeout);

  return temp;
}