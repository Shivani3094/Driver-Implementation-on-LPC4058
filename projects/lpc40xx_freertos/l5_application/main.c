#include <stdio.h>

#define PART1
// define PART2

#include "FreeRTOS.h"
#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"
#include "task.h"
#include "uart_lab.h"

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

//****** PART 1 **********
void uart_write_task(void *p);
void uart_read_task(void *p);

//***** PART 2
void uart_write_task2(void *p);
void uart_read_task2(void *p);

int main(void) {

#ifdef PART1
  uart_lab__init(UART_3, 96, 38400);
  xTaskCreate(uart_read_task, "UART_Rx", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(uart_write_task, "UART_Tx", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#endif

#ifdef PART2
  uart_lab__init(UART_3, 96, 115200);
  uart__enable_receive_interrupt(UART_3);

  xTaskCreate(uart_write_task2, "UART_Task", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(uart_read_task2, "UART_Task", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
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

/***************************** PART 1 *******************************/
// TODO: Use uart_lab__init() function and initialize UART2 or UART3 (your choice)
// TODO: Pin Configure IO pins to perform UART2/UART3 function

void uart_read_task(void *p) {
  char read_byte;
  while (1) {
    // TODO: Use uart_lab__polled_get() function and printf the received value
    read_byte = uart_lab__polled_get(UART_3);
    printf("Byte read from UART = %c\n\n", read_byte);
    vTaskDelay(100);
  }
}

void uart_write_task(void *p) {
  const char write_byte = 'C';
  while (1) {
    uart_lab__polled_put(UART_3, write_byte);
    // TODO: Use uart_lab__polled_put() function and send a value
    printf("Byte to write into UART = %c\n", write_byte);
    vTaskDelay(100);
  }
}

/***************************** END of PART 1 ****************************/
void uart_read_task2(void *p) {
  char read_byte;
  while (1) {
    // TODO: Use uart_lab__polled_get() function and printf the received value
    uart_lab__get_char_from_queue(&read_byte, 0);
    printf("Byte read from UART = %c\n\n", read_byte);
    vTaskDelay(100);
  }
}

void uart_write_task2(void *p) {
  const char write_byte = 'C';
  while (1) {
    uart_lab__polled_put(UART_3, write_byte);
    // TODO: Use uart_lab__polled_put() function and send a value
    printf("Byte to write into UART = %c\n", write_byte);
    vTaskDelay(100);
  }
}

/***************************** END of PART 2 ****************************/