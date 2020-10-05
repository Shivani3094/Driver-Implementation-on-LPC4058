#include "ssp2_lab.h"
#include "gpio.h"
#include <stdint.h>
#include <stdio.h>

/*
 * Pin Numbers:
 * SSP2 -> SCK = P1_0
 * SSP2 -> MOSI = P1_1
 * SSP2 -> MISO = P1_4
 */
const uint32_t set_power_bit = (1 << 20);
const uint32_t cpu_clock = 96000; // 96MHz
const uint32_t busy_bit_SR = (1 << 4);
#define set_pin_as_output 0b100

void ssp2_lab__init(uint32_t max_clock_mhz) {

  // Power on Peripheral
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP2);
  LPC_SC->PCONP |= set_power_bit;

  // Setup prescalar register to be <= max_clock_mhz
  /*
   * PCLK = cpu_clock
   * SCR = 0
   * CPSDVSR = PCLK / (max_clock_mhz(SCR + 1))
   */

  LPC_SSP2->CPSR = cpu_clock / max_clock_mhz;
}

void configure__ssp2_lab_pin_functions(void) {
  // Set SCK, MISO and MOSI as output pins
  LPC_IOCON->P1_0 &= ~(set_pin_as_output); // SCK
  LPC_IOCON->P1_1 &= ~(set_pin_as_output); // MOSI
  LPC_IOCON->P1_4 &= ~(set_pin_as_output); // MISO

  LPC_IOCON->P1_0 |= set_pin_as_output;
  LPC_IOCON->P1_1 |= set_pin_as_output;
  LPC_IOCON->P1_4 |= set_pin_as_output;

  // Setup control registers CR0 and CR1
  LPC_SSP2->CR0 = 7;
  LPC_SSP2->CR1 = (1 << 1);
}
uint8_t ssp2_lab__exchange_byte(uint8_t data_out) {

  LPC_SSP2->DR = data_out;

  while (LPC_SSP2->SR & busy_bit_SR) {
    // This will loop until the busy bit is set to 0 indicating end of transfer
  }

  return (LPC_SSP2->DR);
}