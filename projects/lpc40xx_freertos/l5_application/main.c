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
#include "sj2_cli.h"
#include "task.h"

void producer(void *p);
void consumer(void *p);

static QueueHandle_t switch_queue;

typedef enum { switch__off, switch__on } switch_e;
typedef enum { PRODUCER_HIGH, CONSUMER_HIGH, EQUAL_PRIORITY } set_priority;

switch_e get_switch_input_from_switch0(void);

int main(void) {

#ifdef PART1
  switch_queue =
      xQueueCreate(1, sizeof(switch_e)); // Choose depth of item being our enum (1 should be okay for this example)

  set_priority priority = EQUAL_PRIORITY;

  switch (priority) {
  case 0: {
    printf("*** PRODUCER HAS HIGHER PRIORITY ***\n\n");
    xTaskCreate(producer, "tx", 2048 / (sizeof(void *)), NULL, PRIORITY_HIGH, NULL);
    xTaskCreate(consumer, "rx", 2048 / (sizeof(void *)), NULL, PRIORITY_LOW, NULL);
    break;
  }

  case 1: {
    printf("*** CONSUMER HAS HIGHER PRIORITY ***\n\n");
    xTaskCreate(producer, "tx", 2048 / (sizeof(void *)), NULL, PRIORITY_LOW, NULL);
    xTaskCreate(consumer, "rx", 2048 / (sizeof(void *)), NULL, PRIORITY_HIGH, NULL);
    break;
  }

  case 2: {
    printf("*** EQUAL PRIORITY ***\n\n");
    xTaskCreate(producer, "tx", 2048 / (sizeof(void *)), NULL, PRIORITY_LOW, NULL);
    xTaskCreate(consumer, "rx", 2048 / (sizeof(void *)), NULL, PRIORITY_LOW, NULL);
    break;
  }
  default:
    printf("INVALID INPUT");
  }
#endif

  vTaskStartScheduler();

  return 0;
}

switch_e get_switch_input_from_switch0(void) {
  const uint32_t set_switch_as_input = ~(1 << 15);
  LPC_GPIO1->DIR &= set_switch_as_input;

  uint32_t bit_mask = (1 << 15);
  uint32_t check_switch_status = (LPC_GPIO1->PIN) & (bit_mask);
  
  bool switch_status;

  if (check_switch_status) {
    switch_status = switch__on;
  } else {
    switch_status = switch__off;
  }

  return switch_status;
}

void producer(void *p) {
  while (1) {
    const switch_e switch_value = get_switch_input_from_switch0();

    printf(" producer() sending message\n");
    if (!(xQueueSend(switch_queue, &switch_value, 0))) {
      printf("Failed to send to consumer\n");
    } else {
      printf("Successfully sent\n");
    }
    vTaskDelay(1000);
  }
}

void consumer(void *p) {
  switch_e switch_value;
  while (1) {
    printf("consumer() receiving message\n");
    xQueueReceive(switch_queue, &switch_value, portMAX_DELAY);
    printf("Received from Queue: %d \n\n", switch_value);
  }
}
