#include <stdio.h>

//#define PART1

#include "FreeRTOS.h"
#include "ssp2_lab.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

/************ PART 1 ******************/
void spi_task(void *p);
typedef struct {
  uint8_t manufacturer_id;
  uint8_t device_id_1;
  uint8_t device_id_2;
  uint8_t extended_device_id;
} adesto_flash_id_s;

int main(void) {

  #if PART1
  xTaskCreate(spi_task, "SPI_task_Part_1", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  #endif

  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  create_blinky_tasks();
  create_uart_task();

  return 0;
}

static void create_blinky_tasks(void) {
  /**
   * Use '#if (1)' if you wish to observe how two tasks can blink LEDs
   * Use '#if (0)' if you wish to use the 'periodic_scheduler.h' that will spawn 4 periodic tasks, one for each LED
   */
#if (1)
  // These variables should not go out of scope because the 'blink_task' will reference this memory
  static gpio_s led0, led1;

  led0 = board_io__get_led0();
  led1 = board_io__get_led1();

  xTaskCreate(blink_task, "led0", configMINIMAL_STACK_SIZE, (void *)&led0, PRIORITY_LOW, NULL);
  xTaskCreate(blink_task, "led1", configMINIMAL_STACK_SIZE, (void *)&led1, PRIORITY_LOW, NULL);
#else
  const bool run_1000hz = true;
  const size_t stack_size_bytes = 2048 / sizeof(void *); // RTOS stack size is in terms of 32-bits for ARM M4 32-bit CPU
  periodic_scheduler__initialize(stack_size_bytes, !run_1000hz); // Assuming we do not need the high rate 1000Hz task
  UNUSED(blink_task);
#endif
}

static void create_uart_task(void) {
  // It is advised to either run the uart_task, or the SJ2 command-line (CLI), but not both
  // Change '#if (0)' to '#if (1)' and vice versa to try it out
#if (0)
  // printf() takes more stack space, size this tasks' stack higher
  xTaskCreate(uart_task, "uart", (512U * 8) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#else
  sj2_cli__init();
  UNUSED(uart_task); // uart_task is un-used in if we are doing cli init()
#endif
}

static void blink_task(void *params) {
  const gpio_s led = *((gpio_s *)params); // Parameter was input while calling xTaskCreate()

  // Warning: This task starts with very minimal stack, so do not use printf() API here to avoid stack overflow
  while (true) {
    gpio__toggle(led);
    vTaskDelay(500);
  }
}

// This sends periodic messages over printf() which uses system_calls.c to send them to UART0
static void uart_task(void *params) {
  TickType_t previous_tick = 0;
  TickType_t ticks = 0;

  while (true) {
    // This loop will repeat at precise task delay, even if the logic below takes variable amount of ticks
    vTaskDelayUntil(&previous_tick, 2000);

    /* Calls to fprintf(stderr, ...) uses polled UART driver, so this entire output will be fully
     * sent out before this function returns. See system_calls.c for actual implementation.
     *
     * Use this style print for:
     *  - Interrupts because you cannot use printf() inside an ISR
     *    This is because regular printf() leads down to xQueueSend() that might block
     *    but you cannot block inside an ISR hence the system might crash
     *  - During debugging in case system crashes before all output of printf() is sent
     */
    ticks = xTaskGetTickCount();
    fprintf(stderr, "%u: This is a polled version of printf used for debugging ... finished in", (unsigned)ticks);
    fprintf(stderr, " %lu ticks\n", (xTaskGetTickCount() - ticks));

    /* This deposits data to an outgoing queue and doesn't block the CPU
     * Data will be sent later, but this function would return earlier
     */
    ticks = xTaskGetTickCount();
    printf("This is a more efficient printf ... finished in");
    printf(" %lu ticks\n\n", (xTaskGetTickCount() - ticks));
  }
}

/************************ PART 1 **************************/
static gpio_s set_chip_select;
const uint8_t opcode_to_initiate_read = 0x9F;
adesto_flash_id_s dummy_byte_read = {0xAA, 0x55, 0xFF, 0xBB};
adesto_flash_id_s spi_byte_write;

// Set Chip Select
static void adesto_cs(void) {
  set_chip_select = gpio__construct_as_output(GPIO__PORT_1, 10);
  gpio__reset(set_chip_select);
}

// Reset Chip Select
static void adesto_ds(void) { gpio__set(set_chip_select); }

adesto_flash_id_s adesto_read_signature(void) {
  adesto_cs();
  {
    ssp2_lab__exchange_byte(opcode_to_initiate_read);

    spi_byte_write.manufacturer_id = ssp2_lab__exchange_byte(dummy_byte_read.manufacturer_id);
    spi_byte_write.device_id_1 = ssp2_lab__exchange_byte(dummy_byte_read.device_id_1);
    spi_byte_write.device_id_2 = ssp2_lab__exchange_byte(dummy_byte_read.device_id_2);
    spi_byte_write.extended_device_id = ssp2_lab__exchange_byte(dummy_byte_read.extended_device_id);
  }
  adesto_ds();

  return spi_byte_write;
}

void spi_task(void *p) {
  const uint32_t spi_clock_mhz = 24;
  ssp2_lab__init(spi_clock_mhz);
  configure__ssp2_lab_pin_functions();

  while (1) {
    adesto_flash_id_s id = adesto_read_signature();
    fprintf(stderr, "Manfacture ID: %x\n Device ID1: %x\n Device ID2: %x\n Extended device ID:%x\n\n",
            id.manufacturer_id, id.device_id_1, id.device_id_2, id.extended_device_id);

    vTaskDelay(1000);
  }
}
/************************ END of PART 1 **************************/