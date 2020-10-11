#include "uart_lab.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"

#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "queue.h"

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

static QueueHandle_t uart_rx_queue;

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
  // Refer to LPC User manual and setup the register bits correctly
  // The first page of the UART chapter has good instructions
  const uint16_t divider_16_bit = peripheral_clock * 1000 * 1000 / (16 * baud_rate);
  printf("Baud Rate = %luHz\n", baud_rate);

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
    while (!(LPC_UART3->LSR & Tx_Data_Ready)) {
      // Keep Looping until its not empty;
    }
  }
}

/*******************************/
// Private function of our uart_lab.c
static void receive_interrupt(void) {
  // TODO: Read the IIR register to figure out why you got interrupted
  // INTSTATUS bit 0 (0 << 0) At least one interrupt is pending
  const uint8_t read_iir_reg = LPC_UART3->IIR;
  const uint8_t read_IIR_status = read_iir_reg & (1 << 0);
  // TODO: Based on IIR status, read the LSR register to confirm if there is data to be read
  if (!(read_IIR_status)) {
    const uint8_t rx_fifo_ready = LPC_UART3->LSR;
    const uint8_t rx_data_read_ready = rx_fifo_ready & (1 << 0);

    if (rx_data_read_ready) {
      // TODO: Based on LSR status, read the RBR register and input the data to the RX Queue
      const char byte = LPC_UART3->RBR;
      printf("CHARACTER = %c\n", byte);
      xQueueSendFromISR(uart_rx_queue, &byte, NULL);
    } else {
      printf("Rx Not Ready\n");
    }
  } else {
    printf("Interrupt Not Pending\n");
  }
}

// Public function to enable UART interrupt
// TODO Declare this at the header file
void uart__enable_receive_interrupt(uart_number_e uart_number) {
  // TODO: Use lpc_peripherals.h to attach your interrupt
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART3, receive_interrupt, NULL);
  NVIC_EnableIRQ(UART3_IRQn);

  // TODO: Enable UART receive interrupt by reading the LPC User manual
  // Hint: Read about the IER register
  const uint8_t Rx_Intr_Enable = (1 << 0);
  LPC_UART3->IER |= (Rx_Intr_Enable);

  // TODO: Create your RX queue
  uart_rx_queue = xQueueCreate(1, sizeof(int));
}

// Public function to get a char from the queue (this function should work without modification)
// TODO: Declare this at the header file
bool uart_lab__get_char_from_queue(char *input_byte, uint32_t timeout) {
  return xQueueReceive(uart_rx_queue, input_byte, timeout);
}