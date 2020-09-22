#include "gpiolab_isr.h"
//#include "gpio.h"
#include "lpc40xx.h"

#include <stdint.h>
#include <stdio.h>

// Note: You may want another separate array for falling vs. rising edge callbacks
static function_pointer_t gpio0_callbacks[32];
static void gpio__pin_handler(void) {
  while (1) {
  }
}

// static gpio_interrupt_e interrupt_type;
static function_pointer_t gpio0_callbacks[32] = {
    gpio__pin_handler, // GPIO0_0
    gpio__pin_handler, // GPIO0_1
    gpio__pin_handler, // GPIO0_2
    gpio__pin_handler, // GPIO0_3
    gpio__pin_handler, // GPIO0_4
    gpio__pin_handler, // GPIO0_5
    gpio__pin_handler, // GPIO0_6
    gpio__pin_handler, // GPIO0_7
    gpio__pin_handler, // GPIO0_8
    gpio__pin_handler, // GPIO0_9
    gpio__pin_handler, // GPIO0_10
    gpio__pin_handler, // GPIO0_11
    gpio__pin_handler, // GPIO0_12
    gpio__pin_handler, // GPIO0_13
    gpio__pin_handler, // GPIO0_14
    gpio__pin_handler, // GPIO0_15
    gpio__pin_handler, // GPIO0_16
    gpio__pin_handler, // GPIO0_17
    gpio__pin_handler, // GPIO0_18
    gpio__pin_handler, // GPIO0_19
    gpio__pin_handler, // GPIO0_20
    gpio__pin_handler, // GPIO0_21
    gpio__pin_handler, // GPIO0_22
    gpio__pin_handler, // GPIO0_23
    gpio__pin_handler, // GPIO0_24
    gpio__pin_handler, // GPIO0_25
    gpio__pin_handler, // GPIO0_26
    gpio__pin_handler, // GPIO0_27
    gpio__pin_handler, // GPIO0_28
    gpio__pin_handler, // GPIO0_29
    gpio__pin_handler, // GPIO0_30
    gpio__pin_handler, // GPIO0_31
};

void gpio0__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {

  // Store the callback based on the pin at gpio0_callbacks
  gpio0_callbacks[pin] = callback;

  // Configure GPIO 0 pin for rising or falling edge
  if (interrupt_type == GPIO_INTR__FALLING_EDGE) {
    // gpio_s switch30 = {0, pin};
    // LPC_GPIO0->DIR &= ~(1 << pin);
    (LPC_GPIOINT->IO0IntEnF) |= (1 << pin);

  } else if (interrupt_type == GPIO_INTR__RISING_EDGE) {

    // gpio_s switch29 = {0, pin};
    (LPC_GPIOINT->IO0IntEnR) |= (1 << pin);
  }
}

// We wrote some of the implementation for you
void gpio0__interrupt_dispatcher(void) {

  // Check which pin generated the interrupt
  gpio_s pin_that_generated_interrupt;

  if (LPC_GPIOINT->IO0IntStatF) {
    pin_that_generated_interrupt.pin_number = 29;

    // function_pointer_t attached_user_handler = gpio0_callbacks[pin_that_generated_interrupt];
    // fprintf(stderr, "pass4 attached_user_handler %p", attached_user_handler);
  } else if (LPC_GPIOINT->IO0IntStatR) {
    pin_that_generated_interrupt.pin_number = 30;
    // function_pointer_t attached_user_handler = gpio0_callbacks[pin_that_generated_interrupt];
    // fprintf(stderr, "pass4 attached_user_handler %p", attached_user_handler);
  }

  function_pointer_t attached_user_handler = gpio0_callbacks[pin_that_generated_interrupt.pin_number];
  fprintf(stderr, "Address of attached_user_handler = %p", attached_user_handler);

  attached_user_handler();
  clear_pin_interrupt(pin_that_generated_interrupt);
}

void clear_pin_interrupt(gpio_s gpio) { (LPC_GPIOINT->IO0IntClr) |= (1 << gpio.pin_number); }