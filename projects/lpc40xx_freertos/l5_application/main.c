#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "gpio_lab.h"
#include "semphr.h"

#include "periodic_scheduler.h"
#include "sj2_cli.h"

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

// Task Created for Part0 in Lab2
void led_task0(void *param);

// Task Created for Part2 in Lab2
void led_task2(void *task_parameter);

// Task Created for Part 3 in Lab2
void switch_task3(void *task_parameter);
void led_task3(void *task_parameter);

static SemaphoreHandle_t switch_press_indication;

// Task Created for Part 3 in Lab2
void switch_task4(void *task_parameter);
void led_task4(void *task_parameter);

static SemaphoreHandle_t switch_press_indication2;
static SemaphoreHandle_t switch_notpressed;

int main(void) {

  /******************************************************************************/
  /*                          Start of MultiBlink LED                           */
  /******************************************************************************/
  switch_press_indication2 = xSemaphoreCreateBinary();
  switch_notpressed = xSemaphoreCreateBinary();

  static port_pin_s switch_multi = {1, 15};

  xTaskCreate(switch_task4, "Press_Switch", 2048 / sizeof(void *), &switch_multi, PRIORITY_LOW, NULL);
  xTaskCreate(led_task4, "Blink_LED", 2048 / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);

  /******************************************************************************/
  /*                          END of Part 3:                                    */
  /******************************************************************************/

  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  /******************************************************************************/
  /*                          Start of Part 0:                                  */
  /******************************************************************************/

  xTaskCreate(led_task0, "LED_TASK0", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);

  /******************************************************************************/
  /*                          END of Part 0:                                    */
  /******************************************************************************/
  create_blinky_tasks();
  create_uart_task();

  /******************************************************************************/
  /*                          Start of Part 2:                                  */
  /******************************************************************************/

  // led (Port, pin) present on the board
  static port_pin_s led0 = {1, 18};
  static port_pin_s led1 = {2, 3};

  // Other LED's present on the board
  /*
  static port_pin_s led0 = {1, 24};
  static port_pin_s led1 = {1, 26};
  */
  xTaskCreate(led_task2, "LED_PORTx.PINy", 2048 / sizeof(void *), (void *)&led0, PRIORITY_LOW, NULL);
  xTaskCreate(led_task2, "LED_PORTx.PINy", 2048 / sizeof(void *), (void *)&led1, PRIORITY_LOW, NULL);

  /******************************************************************************/
  /*                          END of Part 2:                                    */
  /******************************************************************************/
  /******************************************************************************/
  /*                          Start of Part 3:                                  */
  /******************************************************************************/
  switch_press_indication = xSemaphoreCreateBinary();

  static port_pin_s switch1 = {1, 15};
  static port_pin_s led = {1, 24};

  xTaskCreate(switch_task3, "Press_Switch", 2048 / sizeof(void *), &switch1, PRIORITY_LOW, NULL);
  xTaskCreate(led_task3, "Blink_LED", 2048 / sizeof(void *), &led, PRIORITY_LOW, NULL);

  /******************************************************************************/
  /*                          END of Part 3:                                    */
  /******************************************************************************/

  return 0;
}

/******************************************************************************/
/*                        Task for Lab 2: Part 0                              */
/* This function will directly manipulate the microcontroller  register to    */
/* blink the onbaord LED.                                                     */
/******************************************************************************/
void led_task0(void *param) {

  const uint32_t led = (1U << 24);

  // Set the IOCON MUX function select pins to 000
  LPC_IOCON->P1_18 = 0x000;
  LPC_IOCON->P1_24 = 0x000;
  LPC_IOCON->P1_26 = 0x000;
  LPC_IOCON->P2_3 = 0x000;

  // Set the DIR register bit for the LED port pin
  LPC_GPIO1->DIR |= led;

  while (true) {
    // Set PIN register bit to 0 to turn ON LED (led may be active low)
    LPC_GPIO1->SET = led;
    vTaskDelay(500);

    // Set PIN register bit to 1 to turn OFF LED
    LPC_GPIO1->CLR = led;
    vTaskDelay(500);
  }
}

/******************************************************************************/
/*                        Task for Lab 2: Part 2                              */
/* This function will use the user defined GPIO drivers to blink 2 LEDs in two*/
/* separate tasks                                                             */
/******************************************************************************/
void led_task2(void *task_parameter) {

  const port_pin_s led = *((port_pin_s *)(task_parameter));

  while (true) {
    gpiolab__set_high(led);
    vTaskDelay(500);

    gpiolab__set_low(led);
    vTaskDelay(500);
  }
}

/******************************************************************************/
/*                        Task for Lab 2: Part 3                              */
/* This function is used to design and LED and switch task. The LED toggles   */
/* when switch is pressed. The LED is off when switch is released.            */
/* Semaphores are used to monitor the switch action.                          */
/******************************************************************************/
void led_task3(void *task_parameter) {

  port_pin_s led = *((port_pin_s *)(task_parameter));

  while (true) {
    if (xSemaphoreTake(switch_press_indication, 1000)) {

      gpiolab__set_low(led);
      vTaskDelay(250);
      gpiolab__set_high(led);
      vTaskDelay(250);
      puts("Switch is pressed");
    } else {
      gpiolab__set_high(led);
      puts("Timeout: No switch press indication for 1000ms");
    }
  }
}

void switch_task3(void *task_parameter) {
  port_pin_s switch1 = *((port_pin_s *)task_parameter);

  while (true) {
    // Binary Semaphore is set when switch is pressed
    if (gpiolab__get_level(switch1)) {
      xSemaphoreGive(switch_press_indication);

      vTaskDelay(500);
    }
  }
}

/******************************************************************************/
/*                             Pre-existing Tasks                             */
/******************************************************************************/

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

/******************************************************************************/
/*                        Task for Multi LED Blink                            */
/* This function will blink the LED's from left to right and right to left.   */
/* The two semaphores used are 'switch_press_indication2' and                 */
/* 'switch_notpressed' to monitor the switch press action. LED would blink    */
/* only when the switch is pressed. The delay used is less so that the        */
/* semaphore can be taken and released faster and this will sync the LED      */
/* glow and switch press action.                                              */
/******************************************************************************/
void led_task4(void *task_parameter) {

  // Initialise with Null
  port_pin_s led = *((port_pin_s *)task_parameter);
  uint8_t count = 0;

  while (true) {
    if (xSemaphoreTake(switch_press_indication2, 100)) {
      for (count = 0; count < 8; count++) {

        // This would glow the LEDs from left to right
        if (count == 0) {
          led.pin = 18;
          led.port = 1;
        } else if (count == 1) {
          led.pin = 24;
          led.port = 1;
        } else if (count == 2) {
          led.pin = 26;
          led.port = 1;
        } else if (count == 3) {
          led.pin = 3;
          led.port = 2;
        }
        // This would glow the LEDs from right to left
        if (count == 4) {
          led.pin = 3;
          led.port = 2;
        } else if (count == 5) {
          led.pin = 26;
          led.port = 1;
        } else if (count == 6) {
          led.pin = 24;
          led.port = 1;
        } else if (count == 7) {
          led.pin = 18;
          led.port = 1;
        }

        // This is executed when switch is pressed.
        if (!(xSemaphoreTake(switch_notpressed, 50))) {
          gpiolab__set_high(led);
          vTaskDelay(25);
          puts("Switch is Pressed. LED is glowing");

          gpiolab__set_low(led);
          vTaskDelay(25);
        }
      }
      count = 0;
    } else {
      puts("Switch Not Pressed");
    }
  }
}
void switch_task4(void *task_parameter) {
  port_pin_s switch1 = *((port_pin_s *)task_parameter);

  while (true) {
    if (gpiolab__get_level(switch1)) {
      xSemaphoreGive(switch_press_indication2);
      vTaskDelay(50);
    } else {
      xSemaphoreGive(switch_notpressed);
      vTaskDelay(50);
    }
  }
}