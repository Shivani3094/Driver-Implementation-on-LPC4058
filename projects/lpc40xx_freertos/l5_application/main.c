#include <stdio.h>

//#define PART1
#define PART3

#include "FreeRTOS.h"
#include "i2c.h"
#include "i2c_slave.h"
#include "i2c_slave_functions.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

void turn_on_an_led(void);
void turn_off_an_led(void);
static volatile uint8_t slave_memory[max_memory_index];

static void led_task(void *p) {
  while (1) {
    if (slave_memory[0] == 0) {
      turn_on_an_led();
    } else {
      turn_off_an_led();
    }
    vTaskDelay(200);
  }
}

int main(void) {

#ifdef PART1
  sj2_cli__init();
  i2c2__slave_init(0x14);
#endif

#ifdef PART3
  i2c2__slave_init(0x14);
  xTaskCreate(led_task, "LED_TASK", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);

#endif
  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}

void turn_on_an_led(void) {
  const uint32_t led = (1U << 24);

  // Set the IOCON MUX function select pins to 000
  LPC_IOCON->P1_18 = 0x000;
  LPC_IOCON->P1_24 = 0x000;
  LPC_IOCON->P1_26 = 0x000;
  LPC_IOCON->P2_3 = 0x000;

  // Set the DIR register bit for the LED port pin
  LPC_GPIO1->DIR |= led;
  // Set PIN register bit to 1 to turn OFF LED
  LPC_GPIO1->CLR = led;
}

void turn_off_an_led(void) {
  const uint32_t led = (1U << 24);

  // Set the IOCON MUX function select pins to 000
  LPC_IOCON->P1_18 = 0x000;
  LPC_IOCON->P1_24 = 0x000;
  LPC_IOCON->P1_26 = 0x000;
  LPC_IOCON->P2_3 = 0x000;

  // Set the DIR register bit for the LED port pin
  LPC_GPIO1->DIR |= led;

  // Set PIN register bit to 0 to turn ON LED (led may be active low)
  LPC_GPIO1->SET = led;
}

bool i2c_slave_callback__read_memory(uint8_t memory_index, uint8_t *memory) {
  bool read_status;

  if (memory_index < max_memory_index) {
    *memory = slave_memory[memory_index];
    read_status = true;
  } else {
    read_status = false;
  }
  return read_status;
}

bool i2c_slave_callback__write_memory(uint8_t memory_index, uint8_t memory_value) {
  bool write_status;
  if (memory_index < max_memory_index) {
    slave_memory[memory_index] = memory_value;
    write_status = true;
  } else {
    write_status = false;
  }
  return write_status;
}
