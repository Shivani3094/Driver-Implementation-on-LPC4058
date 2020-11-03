#include "i2c_slave.h"
#include "lpc40xx.h"
/*
SJ2 I2C's	SDA	    SCL	    Multiplexed
-------------------------------------------
I2C 0	    P1.30	P1.31	ADCx
I2C 1	    P0.0	P0.1	UART0, UART3, CAN1
I2C 2	    P0.10	P0.11	UART2
            P1.15	P4.29
MASTER: I2C2: P0.10 and P0.11
Slave: I2C0: P1.30 and P1.31
*/

const uint32_t set_mask_i2c0 = (0b1111111 << 1);
const uint32_t set_interface_enable = (1 << 6);
const uint32_t set_AA = (1 << 2);

void i2c2__slave_init(uint8_t slave_address_to_respond_to) {

  // Set I2CADR, I2CMASK, I2CONSET
  LPC_I2C2->ADR0 |= slave_address_to_respond_to;
  LPC_I2C2->MASK0 &= ~set_mask_i2c0;
  LPC_I2C2->CONSET = set_interface_enable | set_AA;
}