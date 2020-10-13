#include <stdio.h>

//#define PART1
#define PART2

#include "FreeRTOS.h"
#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"
#include "task.h"
#include "uart_lab.h"

//****** PART 1 **********
void uart_write_task(void *p);
void uart_read_task(void *p);

//***** PART 2 ***********
void uart_write_task2(void *p);
void uart_read_task2(void *p);

int main(void) {

#ifdef PART1
  uart_lab__init(UART_3, 96, 38400);
  xTaskCreate(uart_read_task, "UART_Rx", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(uart_write_task, "UART_Tx", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#endif

#ifdef PART2
  puts("LAB7- UART: Part2\n");
  uart_lab__init(UART_3, 96, 38400);
  uart__enable_receive_interrupt(UART_3);

  xTaskCreate(uart_write_task2, "UART_Task", 2048, NULL, PRIORITY_LOW, NULL);
  xTaskCreate(uart_read_task2, "UART_Task", 2048, NULL, PRIORITY_LOW, NULL);
#endif

  puts("Starting RTOS\n");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}

/***************************** PART 1 *******************************/

void uart_read_task(void *p) {
  char read_byte;
  while (1) {
    read_byte = uart_lab__polled_get(UART_3);
    printf("Byte read from UART = %c\n\n", read_byte);
    vTaskDelay(100);
  }
}

void uart_write_task(void *p) {
  const char write_byte = 'C';
  while (1) {
    uart_lab__polled_put(UART_3, write_byte);
    printf("Byte to write into UART = %c \n", write_byte);
    vTaskDelay(100);
  }
}

/***************************** END of PART 1 ****************************/

/***************************** PART 2 *******************************/
void uart_read_task2(void *p) {
  char read_byte;
  while (1) {
    // IF statement is added to avoid a garbage read if nothing is written into UART
    if (uart_lab__get_char_from_queue(&read_byte, 0)) {
      printf("Byte read from UART = %c\n\n", read_byte);
      vTaskDelay(300);
    }
  }
}

void uart_write_task2(void *p) {
  const char write_byte = 'A';
  while (1) {
    uart_lab__polled_put(UART_3, write_byte);
    printf("Byte to write into UART = %c\n", write_byte);
    vTaskDelay(300);
  }
}

/***************************** END of PART 2 ****************************/