#include <stdio.h>

//#define Q29
#define Q30

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
void spi_task(void);
static QueueHandle_t signal_queue;

int main(void) {

#ifdef Q29
  signal_queue = xQueueCreate(1, sizeof(int));

  xTaskCreate(producer_task, "producer", 2048 / (sizeof(void *)), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(consumer_task, "consumer", 2048 / (sizeof(void *)), NULL, PRIORITY_LOW, NULL);
#endif

#ifdef Q30
spi_task();
#endif

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
/************************************ Q30 ***********************************/

// GIVEN
void device_cs(void);
void device_ds(void);
uint8_t exchange_spi_byte(uint8_t output);

uint8_t get_byte_of_data(const uint32_t address) {
const uint8_t dummy_byte = 0xAA;
  device_cs();
  read_opcode();
  (void)exchange_spi_byte((address >> 16) & 0xFF);
  (void)exchange_spi_byte((address >> 8) & 0xFF);
  (void)exchange_spi_byte((address >> 0) & 0xFF);
  const uint8_t read = ssp2_lab__exchange_byte(dummy_byte);
  device_ds();

  return read;
}

void read_opcode(void) {
  const uint8_t opcode_read = 0x1B;
  ssp2_lab__exchange_byte(opcode_read);
}

void spi_task(void)
{
  uint8_t data = get_byte_of_data(0x0);
  fprintf(stderr, "Data Read from Flash = 0x%x\n", data);
}