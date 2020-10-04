#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdint.h>

uint8_t ssp2_lab__exchange_byte(uint8_t data_out);

void ssp2_lab__init(uint32_t max_clock_mhz);
