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

#if defined(STM32F0)
	#include <stm32f0xx.h>
	#include <stm32f0xx_flash.h>
	#define FLASH_STEP 4
	#define FLASH_PGM FLASH_ProgramWord
#elif defined(STM32F1)
	#include <stm32f10x.h>
	#include <stm32f10x_flash.h>
	#define FLASH_STEP 4
	#define FLASH_PGM FLASH_ProgramWord
#elif defined(STM32F2)
	#include <stm32f2xx.h>
	#include <stm32f2xx_flash.h>
	#define FLASH_STEP 4
	#define FLASH_PGM FLASH_ProgramWord
	#define FLASH_EraseAllPages() FLASH_EraseAllSectors(VoltageRange_3)
#elif defined(STM32F30)
	#include <stm32f30x.h>
	#include <stm32f30x_flash.h>
	#define FLASH_STEP 4
	#define FLASH_PGM FLASH_ProgramWord
#elif defined(STM32F37)
	#include <stm32f37x.h>
	#include <stm32f37x_flash.h>
	#define FLASH_STEP 4
	#define FLASH_PGM FLASH_ProgramWord
#elif defined(STM32F4)
	#include <stm32f4xx.h>
	#include <stm32f4xx_flash.h>
	#define FLASH_STEP 4
	#define FLASH_PGM FLASH_ProgramWord
	#define FLASH_EraseAllPages() FLASH_EraseAllSectors(VoltageRange_3)
#elif defined(STM32L1)
	#include <stm32l1xx.h>
	#include <stm32l1xx_flash.h>
	#define FLASH_STEP 4
	#define FLASH_PGM FLASH_FastProgramWord
#else
	#error "No valid device specified"
#endif

#define mmio64(x)   (*(volatile uint64_t *)(x))
#define mmio32(x)   (*(volatile uint32_t *)(x))
#define mmio16(x)   (*(volatile uint16_t *)(x))
#define mmio8(x)   (*(volatile uint8_t *)(x))

#define PARAMS_ADDR ((uint32_t)0x200007D0) // Parameters address in the ram.

#define MASK_STRT (1<<0) // Start bit
#define MASK_BUSY (1<<1) // Busy bit
#define MASK_SUCCESS (1<<2) // Success bit

#define MASK_VEREN (1<<4) // Verification enable bit
#define MASK_DELEN (1<<5) // Delete enable bit

#define MASK_VERR (1<<14) // Verification error
#define MASK_ERR (1<<15) // Error

#define BUFFER_ADDR 0x20000800
#define PARAMS_LEN (BUFFER_ADDR-PARAMS_ADDR)

#define PARAMS ((PARAMS_TypeDef *) PARAMS_ADDR)

typedef struct
{
	__IO uint32_t DEST;          /*!Address offset: 0x00 - Destination in the flash.  Set by debugger.*/
	__IO uint32_t LEN;          /*!Address offset: 0x04 - How many bytes we copy data from sram to flash. Set by debugger.*/
	__IO uint32_t STATUS;          /*!Address offset: 0x08 -  Status. Set by program and debugger. */
	__IO uint32_t TEST;          /*!Address offset: 0x0C -  For testing */

} PARAMS_TypeDef;

extern uint32_t __params__;
extern uint32_t __buffer__;

int loader(void) {

	uint32_t i;
	for (i=0;i < PARAMS_LEN ;i+=4) { // Clear parameters
		mmio32(PARAMS_ADDR+i) = 0;
	}
	
	FLASH_Unlock();
	
	//~ if (PARAMS->STATUS & MASK_DELEN)
		//~ FLASH_EraseAllPages();
	
	while (1) {
		
		//~ PARAMS->TEST =  0;
		
		asm volatile ("bkpt"); // Halt core after init and before writing to flash.
		asm volatile ("nop"); 
		
		//~ if (!(PARAMS->STATUS  & MASK_STRT)) // Skip if not ready
				//~ continue;
		
		PARAMS->STATUS &= ~MASK_STRT; // Clear start bit
		PARAMS->STATUS  &= ~MASK_ERR; // Clear error bit
		PARAMS->STATUS  &= ~MASK_SUCCESS; // Clear success bit
		//~ PARAMS->STATUS  |= MASK_BUSY; // Set busy bit
		
		uint32_t i=0;			
		while (i < PARAMS->LEN) {
			
			if (FLASH_PGM(PARAMS->DEST+i,  mmio32(BUFFER_ADDR+i)) == FLASH_COMPLETE)	{
					
				i+=FLASH_STEP;
				PARAMS->STATUS |= MASK_SUCCESS; // Set success bit
			}
			else { 
				/* Error occurred while writing data in Flash memory. 
				User can add here some code to deal with this error */
				PARAMS->STATUS |= MASK_ERR; // Set error bit
				break;
			}
		}
		PARAMS->TEST =  PARAMS->DEST+i;
		//~ PARAMS->STATUS &= ~MASK_BUSY; // Clear busy bit
	} 
	return 0;
}

