#include <stdio.h>

//#define PART0
//#define PART1
//#define PART2
//#define PART3
//#define PART4

#include "FreeRTOS.h"
#include "adc.h"
#include "pwm1.h"
#include "queue.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

/********Part 0********/
void pwm_task(void *p);
void pin_configure_pwm_channel_as_io_pin(void);

/********Part 1********/
void adc_task(void *p);
void pin_configure_adc_channel_as_io_pin(void);

/********* PART 2 *******/
static QueueHandle_t adc_to_pwm_task_queue;
void adc_task2(void *p);
void pwm_task2(void *p);

/********* PART 3 *******/
static QueueHandle_t adc_to_pwm_task3_queue;
void adc_task3(void *p);
void pwm_task3(void *p);


int main(void) {

#ifdef PART0
  /***************************** PART 0 ************************************/
  xTaskCreate(pwm_task, "PWM for Part 0", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);

  /***************************** PART 0 ************************************/
#endif

#ifdef PART1
  /***************************** PART 1 ************************************/
  xTaskCreate(adc_task, "ADC for Part 1", 4096 / sizeof(void *), NULL, PRIORITY_LOW, NULL);

  /***************************** PART 1 ************************************/
#endif

#ifdef PART2
  /***************************** PART 2 ************************************/
  // Queue will only hold 1 integer
  adc_to_pwm_task_queue = xQueueCreate(1, sizeof(int));

  xTaskCreate(adc_task2, "ADC for Part 2", 4096 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(pwm_task2, "PWM for Part 2", 4096 / sizeof(void *), NULL, PRIORITY_LOW, NULL);

  /***************************** PART 2 ************************************/
#endif

  /***************************** PART 3 ************************************/
  // Queue will only hold 1 integer
  adc_to_pwm_task3_queue = xQueueCreate(1, sizeof(int));

  xTaskCreate(adc_task3, "ADC for Part 3", 4096 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(pwm_task3, "PWM for Part 3", 4096 / sizeof(void *), NULL, PRIORITY_LOW, NULL);

  /***************************** PART 3 ************************************/

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

/**************************** PART 0 ***********************************/
void pwm_task(void *p) {

  pin_configure_pwm_channel_as_io_pin();

  pwm1__set_duty_cycle(PWM1__2_2, 50);
  pwm1__init_single_edge(1000); // 1000Hz

  uint8_t percent = 0;
  while (1) {
    pwm1__set_duty_cycle(PWM1__2_2, percent);
    fprintf(stderr, "Entered while loop ");

    if (++percent > 100) {
      percent = 0;
      fprintf(stderr, "Counter reached max capacity\n");
    }

    vTaskDelay(100);
  }
}

void pin_configure_pwm_channel_as_io_pin(void) {

  LPC_IOCON->P2_0 = 0b000;
  LPC_IOCON->P2_0 = 0b001;
}
/************************ END OF PART 0 ***********************************/

/****************************** PART 1 ************************************/
void adc_task(void *p) {

  adc__enable_burst_mode();

  pin_configure_adc_channel_as_io_pin();

  while (1) {
    const uint16_t adc_value = adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_4);
    float volts = (adc_value * 3.3) / 4096.0;
    fprintf(stderr, "ADC Value = %d, Volts = %f |", adc_value, volts);
    
    vTaskDelay(100);
  }
}

void pin_configure_adc_channel_as_io_pin(void) {

  // ADC0.4
  LPC_IOCON->P1_30 = 0b000; // clear the bits
  LPC_IOCON->P1_30 = 0b011; // set the bits

  LPC_IOCON->P1_30 &= ~(1U << 7);
}
/************************ END OF PART 1 ***********************************/

/****************************** PART 2 ************************************/
void adc_task2(void *p) {

  adc__enable_burst_mode();

  pin_configure_adc_channel_as_io_pin();

  while (1) {
    // Read ADC value by changing potentiometer
    int adc_reading = adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_4);

    // Send the values to queue
    xQueueSend(adc_to_pwm_task_queue, &adc_reading, 0);
    vTaskDelay(100);
  }
}

void pwm_task2(void *p) {

  int adc_reading = 0;

  while (1) {

    if (xQueueReceive(adc_to_pwm_task_queue, &adc_reading, 100)) {

      fprintf(stderr, "ADC Value Read = %d |", adc_reading);
      // vTaskDelay(100);
    }
  }
}

/************************ END OF PART 2 ***********************************/


/****************************** PART 3 ************************************/

void adc_task3(void *p) {

  adc__enable_burst_mode();

  pin_configure_adc_channel_as_io_pin();

  while (1) {
    // Read ADC value by changing potentiometer
    int adc_reading3 = adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_4);
    float volts = (adc_reading3 * 3.3) / 4096.0;
    fprintf(stderr, "ADC Value = %d, Volts = %f |", adc_reading3, volts);

    // Send the values to queue
    xQueueSend(adc_to_pwm_task_queue, &adc_reading3, 0);
    vTaskDelay(100);
  }
}

void pwm_task3(void *p) {

  int adc_reading3 = 0;
  LPC_GPIO1->DIR |= ( 1 << 18);
  int i = 0;

  pin_configure_pwm_channel_as_io_pin();
  pwm1__set_duty_cycle(PWM1__2_2, 50);
  pwm1__init_single_edge(1000); // 1000Hz

  while (1) {

    if (xQueueReceive(adc_to_pwm_task_queue, &adc_reading3, 100)) {
      for(i=0; i<4096;i++)
      {
        int percent = (adc_reading3 *100)/4096;
      pwm1__set_duty_cycle(PWM1__2_2, percent);
      }
    }
  }
}

/************************ END OF PART 3 ***********************************/
