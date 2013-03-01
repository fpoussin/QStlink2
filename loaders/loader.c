/*
    This file is part of QSTLink2.

    QSTLink2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QSTLink2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QSTLink2.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>

#if defined(STM32F0)
	#include "inc/stm32f0xx.h"
	#define FLASH_REG_ADDR 0x40022000
#elif defined(STM32F1)
	#include "inc/stm32f10x.h"
	#define FLASH_REG_ADDR 0x40022000
#elif defined(STM32F2)
	#include "inc/stm32f2xx.h"
	#define FLASH_REG_ADDR 0x40023c00
#elif defined(STM32F30)
	#include "inc/stm32f30x.h"
	#define FLASH_REG_ADDR 0x40023c00
#elif defined(STM32F37)
	#include "inc/stm32f37x.h"
	#define FLASH_REG_ADDR 0x40023c00
#elif defined(STM32F4)
	#include "inc/stm32f4xx.h"
	#define FLASH_REG_ADDR 0x40023c00
#elif defined(STM32L1)
	#include "inc/stm32l1xx.h"
	#define FLASH_REG_ADDR 0x40022000
#else
	#error "No valid device specified"
#endif

#define mmio32(x)   (*(volatile uint32_t *)(x))
#define mmio16(x)   (*(volatile uint16_t *)(x))

#define FLASH_ACR_OFFSET 0x00
#define FLASH_KEYR_OFFSET 0x04
#define FLASH_OPTKEYR_OFFSET 0x08
#define FLASH_SR_OFFSET 0x0C
#define FLASH_CR_OFFSET 0x10
#define FLASH_OPTCR_OFFSET 0x14

#define PARAMS_ADDR 0x200003E8
#define PARAM_DEST 0x00 // Destination in the flash
#define PARAM_LEN 0x04 // How many times we copy data from sram to flash
#define PARAM_BUSY 0x08 // If the program is busy writing
#define PARAM_PGSIZE 0x1C // Half word or word



static inline uint32_t flashBusy(void) {
	
	return mmio32(FLASH_REG_ADDR + FLASH_SR_OFFSET) & FLASH_SR_BSY;
}

int main(void) {
	
	uint32_t len = 0;
	uint32_t dest = 0;
	uint32_t pgsize = 0;
	while (1) {
		dest = mmio32(PARAMS_ADDR+PARAM_DEST);
		len = mmio32(PARAMS_ADDR+PARAM_LEN);
		pgsize = mmio32(PARAMS_ADDR+PARAM_PGSIZE);
		mmio32(PARAMS_ADDR+PARAM_BUSY) = 1;
		
		uint32_t i=0;
		for (i=0; i<len ;i++) {
			
			while (flashBusy()) {}; // Wait for the flash controller to finish writing.
			
			if (pgsize == 2)
				mmio16(dest+i) = mmio16(SRAM_BASE+i);
			else
				mmio32(dest+i) = mmio32(SRAM_BASE+i);
		}
		mmio32(PARAMS_ADDR+PARAM_BUSY) = 0;
		//~ asm("bkpt");
	}
	return 0;
}

