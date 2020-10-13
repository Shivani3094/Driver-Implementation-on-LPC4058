#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define PART1
//#define PART2
#define PART3

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

//***** Extra Credit ***********
void receiver_task(void *p);
void sender_task(void *p);

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

#ifdef PART3
  puts("LAB7/UART: PART 3\n");
  uart_lab__init(UART_3, 96, 9600);
  uart_lab__init(UART_2, 96, 9600);
  uart__enable_receive_interrupt(UART_3);
  uart__enable_receive_interrupt(UART_2);

  xTaskCreate(receiver_task, "UART_Task", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(sender_task, "UART_Task", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
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
    if (uart_lab__get_char_from_queue(UART_3, &read_byte, 0)) {
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

/***************************** PART 3 *******************************/
void sender_task(void *p) {
  char number_as_string[16] = {0};

  while (true) {
    const int number = rand();
    sprintf(number_as_string, "%i", number);

    // Send one char at a time to the other board including terminating NULL char
    for (int i = 0; i <= strlen(number_as_string); i++) {
      uart_lab__polled_put(UART_2, number_as_string[i]);
    }

    printf("Sent: %i over UART to Receiver Task \n", number);
    vTaskDelay(500);
  }
}

void receiver_task(void *p) {
  char number_as_string[16] = {0};
  int counter = 0;
  while (true) {
    char byte = 0;
    uart_lab__get_char_from_queue(UART_3, &byte, portMAX_DELAY);

    // This is the last char, so print the number
    if ('\0' == byte) {
      number_as_string[counter] = '\0';
      counter = 0;
      printf("Received this number from Sender Task: %s \n\n", number_as_string);
    }
    // We have not yet received the NULL '\0' char, so buffer the data
    else {
      if (counter < 16) {
        number_as_string[counter] = byte;
        counter++;
      }
    }
  }
}
/***************************** END OF FUNCTION ****************************/