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
	#include <stm32f0xx.h>
	#include <stm32f0xx_flash.h>
#elif defined(STM32F1)
	#include <stm32f10x.h>
	#include <stm32f10x_flash.h>
#elif defined(STM32F2)
	#include <stm32f2xx.h>
	#include <stm32f2xx_flash.h>
#elif defined(STM32F30)
	#include <stm32f30x.h>
	#include <stm32f30x_flash.h>
#elif defined(STM32F37)
	#include <stm32f37x.h>
	#include <stm32f37x_flash.h>
#elif defined(STM32F4)
	#include <stm32f4xx.h>
	#include <stm32f4xx_flash.h>
#elif defined(STM32L1)
	#include <stm32l1xx.h>
	#include <stm32l1xx_flash.h>
#else
	#error "No valid device specified"
#endif

#define mmio32(x)   (*(volatile uint32_t *)(x))
#define mmio16(x)   (*(volatile uint16_t *)(x))

#define PARAMS_ADDR 0x200007D0 // Parameters address in the ram.
#define OFFSET_DEST 0x00 // Destination in the flash.  Set by debugger.
#define OFFSET_LEN 0x04 // How many words (32bits) we copy data from sram to flash. Set by debugger.
#define OFFSET_STATUS 0x08 // Status. Set by program and debugger.

#define MASK_STRT (1<<0) // Start bit
#define MASK_BUSY (1<<1) // Busy bit

#define MASK_VEREN (1<<4) // Verification enable bit

#define MASK_VERR (1<<14) // Verification error
#define MASK_ERR (1<<15) // Error

int main(void) {

	uint32_t len = 0;
	uint32_t dest = 0;
	mmio32(PARAMS_ADDR+OFFSET_STATUS) =0 ; // Clear status.
	while (1) {
		
		if (mmio32(PARAMS_ADDR+OFFSET_STATUS) & MASK_STRT) // Skip if not ready
				continue;
		
		dest = mmio32(PARAMS_ADDR+OFFSET_DEST);
		len = mmio32(PARAMS_ADDR+OFFSET_LEN);
		mmio32(PARAMS_ADDR+OFFSET_STATUS) &= ~MASK_STRT; // Clear start bit
		mmio32(PARAMS_ADDR+OFFSET_STATUS) &= ~MASK_ERR; // Clear error bit
		mmio32(PARAMS_ADDR+OFFSET_STATUS) |= MASK_BUSY; // Set busy bit
		
		uint32_t i=0;			
		while (i < len) {
			
			if (FLASH_ProgramWord(dest+i, SRAM_BASE+i) == FLASH_COMPLETE)	{
				
				i++;
			}
			else	{ 
				/* Error occurred while writing data in Flash memory. 
				User can add here some code to deal with this error */
				mmio32(PARAMS_ADDR+OFFSET_STATUS) |= MASK_ERR; // Set error bit
				break;
			}
		}
		mmio32(PARAMS_ADDR+OFFSET_STATUS) &= ~MASK_BUSY; // Clear busy bit
		
		asm volatile ("bkpt #0x1"); // Halt core when done to be able to read/write data from debugger.
	} 
	
	
	return 0;
}

