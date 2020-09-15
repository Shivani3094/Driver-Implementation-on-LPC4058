#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint8_t port;
  uint8_t pin;
} port_pin_s;

/// Should alter the hardware registers to set the pin as input
void gpiolab__set_as_input(port_pin_s gpio);

/// Should alter the hardware registers to set the pin as output
void gpiolab__set_as_output(port_pin_s gpio);

/// Should alter the hardware registers to set the pin as high
void gpiolab__set_high(port_pin_s gpio);

/// Should alter the hardware registers to set the pin as low
void gpiolab__set_low(port_pin_s gpio);

/**
 * Should alter the hardware registers to set the pin as low
 *
 * @param {bool} high - true => set pin high, false => set pin low
 */
void gpiolab__set(port_pin_s gpio, bool param);

/**
 * Should return the state of the pin (input or output, doesn't matter)
 *
 * @return {bool} level of pin high => true, low => false
 */
bool gpiolab__get_level(port_pin_s gpio);