#include <stdio.h>

#include "FreeRTOS.h"
#include "board_io.h"
#include "common_macros.h"
#include "delay.h"
#include "gpio.h"
#include "gpiolab_isr.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "periodic_scheduler.h"
#include "semphr.h"
#include "sj2_cli.h"
#include "task.h"

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

static void gpio0_interrupt(void);
/************ PART 1*********************/
static void gpio_interrupt(void);
void config_gpio_interrupt(void);
void sleep_on_sem_task(void *p);
static void clear_gpio_interrupt(void);
void blinkLed_task(void *p);

static SemaphoreHandle_t switch_pressed_signal;

/************ PART 2*********************/
void pin30_isr(void);
void pin29_isr(void);
static SemaphoreHandle_t switch29_signal;
static SemaphoreHandle_t switch30_signal;
void Task30(void *p);
void Task29(void *p);

static gpio_s led_sw29 = {1, 18};
static gpio_s led_sw30 = {1, 24};

int main(void) {

  /*************** PART 0*************************/
  LPC_GPIO0->DIR &= ~(1 << 30);
  (LPC_GPIOINT->IO0IntEnF) |= (1 << 30);

  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio0_interrupt, NULL);

  NVIC_EnableIRQ(GPIO_IRQn);

  while (1) {
    {
      gpio__set(led_sw30);
      delay__ms(100);
      gpio__reset(led_sw30);
      delay__ms(100);
    }
  }
  /************END of PART 0*********************/

  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  /*************** PART 1*************************/
  switch_pressed_signal = xSemaphoreCreateBinary();
  config_gpio_interrupt();
  NVIC_EnableIRQ(GPIO_IRQn);
  xTaskCreate(sleep_on_sem_task, "Semaphore", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(blinkLed_task, "Blink LED", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  /************END of PART 1*********************/

  /***************** PART 2*********************/

  switch29_signal = xSemaphoreCreateBinary();
  switch30_signal = xSemaphoreCreateBinary();

  LPC_GPIO0->DIR &= ~(1 << 29);
  LPC_GPIO0->DIR &= ~(1 << 30);

  gpio0__attach_interrupt(30, GPIO_INTR__RISING_EDGE, pin30_isr);
  gpio0__attach_interrupt(29, GPIO_INTR__FALLING_EDGE, pin29_isr);

  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio0__interrupt_dispatcher, NULL);
  NVIC_EnableIRQ(GPIO_IRQn);

  xTaskCreate(Task29, "Blink LED", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(Task30, "Blink LED", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  /************ END of PART 2*********************/

  create_uart_task();
  create_blinky_tasks();

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
  create_blinky_tasks();

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

/*************** PART 0*************************/
void gpio0_interrupt(void) {
  fprintf(stderr, "Task is Interrupted.\n");
  (LPC_GPIOINT->IO0IntClr) |= (1 << 30);
}
/***************END of PART 0*************************/

/*************** PART 1*************************/
void gpio_interrupt(void) {
  xSemaphoreGiveFromISR(switch_pressed_signal, NULL);
  fprintf(stderr, "Task is Interrupted. Switched is Pressed\n");
  clear_gpio_interrupt();
}

void config_gpio_interrupt(void) {

  gpio_s switch_num = {0, 30};
  gpio__set_as_input(switch_num);
  LPC_GPIOINT->IO0IntEnF |= (1 << 30);
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio_interrupt, "GPIO for Part 1");
}

void clear_gpio_interrupt(void) { LPC_GPIOINT->IO0IntClr = 0xFFFFFFFF; }

void sleep_on_sem_task(void *p) {
  while (1) {
    if (xSemaphoreTake(switch_pressed_signal, portMAX_DELAY))
      ;
  }
}

void blinkLed_task(void *p) {
  gpio_s led_sw2 = {1, 24};
  while (1) {
    gpio__set(led_sw2);
    delay__ms(100);
    gpio__reset(led_sw2);
    delay__ms(100);
    fprintf(stderr, "LED is Blinking\n");
  }
}
/************END of PART 1*********************/

/*******************PART 2*********************/

void Task29(void *p) {

  while (1) {
    if (xSemaphoreTakeFromISR(switch29_signal, NULL)) {
      gpio__set(led_sw29);
      delay__ms(100);
      gpio__reset(led_sw29);
      delay__ms(100);
    }
  }
}

void Task30(void *p) {

  while (1) {
    if (xSemaphoreTakeFromISR(switch30_signal, NULL)) {
      gpio__set(led_sw30);
      delay__ms(100);
      gpio__reset(led_sw30);
      delay__ms(100);
    }
  }
}

void pin30_isr(void) {
  fprintf(stderr, "Entered ISR due to Pin 30.\n ");
  xSemaphoreGiveFromISR(switch30_signal, NULL);
}

void pin29_isr(void) {
  fprintf(stderr, "Entered ISR due to pin 29.\n ");
  xSemaphoreGiveFromISR(switch29_signal, NULL);
}

/************END of PART 2*********************/