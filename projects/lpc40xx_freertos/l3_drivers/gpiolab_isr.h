#include "gpio.h"
#include "lpc40xx.h"
#include <stdint.h>
#pragma once

typedef enum {
  GPIO_INTR__FALLING_EDGE,
  GPIO_INTR__RISING_EDGE,
} gpio_interrupt_e;

typedef void (*function_pointer_t)(void);

void gpio0__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback);

void gpio0__interrupt_dispatcher(void);

void clear_pin_interrupt(gpio_s gpio);