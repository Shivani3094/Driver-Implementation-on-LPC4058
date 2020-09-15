
#include "gpio_lab.h"
#include "lpc40xx.h"

static const LPC_GPIO_TypeDef *gpiolab_memory_map[] = {LPC_GPIO0, LPC_GPIO1, LPC_GPIO2};

/// Should return the Port Number
static LPC_GPIO_TypeDef *gpiolab__get_struct(port_pin_s gpio) {
  return (LPC_GPIO_TypeDef *)gpiolab_memory_map[gpio.port];
}

/// Should alter the hardware registers to set the pin as input
void gpiolab__set_as_input(port_pin_s gpio) {

  gpiolab__get_struct(gpio)->DIR = (0U << gpio.pin);

  /// Alternative method to manipulate the Port Pins
  // LPC_GPIO1->DIR |= (0U << pin_num);
}

/// Should alter the hardware registers to set the pin as output
void gpiolab__set_as_output(port_pin_s gpio) {

  gpiolab__get_struct(gpio)->DIR = (1U << gpio.pin);

  /// Alternative method to manipulate the Port Pins
  // LPC_GPIO1->DIR |= (1U << pin_num);
}

/// Should alter the hardware registers to set the pin as high
void gpiolab__set_high(port_pin_s gpio) {

  gpiolab__get_struct(gpio)->SET = (UINT32_C(1) << gpio.pin);

  /// Alternative method to manipulate the Port Pins
  // LPC_GPIO1->SET = (1U << pin_num)
}

/// Should alter the hardware registers to set the pin as low
void gpiolab__set_low(port_pin_s gpio) {

  gpiolab__get_struct(gpio)->CLR = (UINT32_C(1) << gpio.pin);

  /// Alternative method to manipulate the Port Pins
  // LPC_GPIO1->CLR = (1U << pin_num);
}

/**
 * Should alter the hardware registers to set the pin as low
 *
 * @param {bool} high - true => set pin high, false => set pin low
 */
void gpiolab__set(port_pin_s gpio, bool param) {

  if (param == true) {
    gpiolab__get_struct(gpio)->PIN = (1U << gpio.pin);
  } else {
    gpiolab__get_struct(gpio)->PIN = (0U << gpio.pin);
  }
}

/**
 * Should return the state of the pin (input or output, doesn't matter)
 *
 * @return {bool} level of pin high => true, low => false
 */

bool gpiolab__get_level(port_pin_s gpio) { return (gpiolab__get_struct(gpio))->PIN & (1 << (gpio.pin)); }