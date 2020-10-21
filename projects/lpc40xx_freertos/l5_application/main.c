#include <stdio.h>

#define PART1

#include "FreeRTOS.h"
#include "board_io.h"
#include "common_macros.h"
#include "delay.h"
#include "gpio.h"
#include "lpc_peripherals.h"
#include "periodic_scheduler.h"
#include "semphr.h"
#include "task.h"

void producer_task(void *p);
void consumer_task(void *p);

static QueueHandle_t signal_queue;

int main(void) {

  signal_queue = xQueueCreate(1, sizeof(int));

  xTaskCreate(producer_task, "producer", 2048 / (sizeof(void *)), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(consumer_task, "consumer", 2048 / (sizeof(void *)), NULL, PRIORITY_LOW, NULL);

  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}

void producer_task(void *p) {
  int data = 0;
  while (1) {

    data = data + 1;
    if (xQueueSend(signal_queue, &data, 0)) {
      fprintf(stderr, "Sent the Data\n");
    }
    vTaskDelay(1000); // 1sec
  }
}

void consumer_task(void *p) {
  int data;
  while (1) {

    xQueueReceive(signal_queue, &data, portMAX_DELAY);
    printf("Received from Queue: %d \n\n", data);
  }
}
