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

#if defined(STM32F2) || defined(STM32F4)
	#define FLASH_STEP 4
	#define FLASH_PGM FLASH_ProgramWord
	
	/* Base address of the Flash sectors */
	#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
	#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
	#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
	#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
	#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
	#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
	#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
	#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */
	#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
	#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
	#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
	#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */
	
	uint32_t GetSector(uint32_t Address);
#elif defined(STM32F0)
	#define FLASH_STEP 4
	#define FLASH_PGM FLASH_ProgramWord
	#define FLASH_PAGE_SIZE         ((uint32_t)0x00000400)   /* FLASH Page Size */
	uint32_t GetPage(uint32_t Address);
#elif defined(STM32F1) || defined(STM32F30) || defined(STM32F37)
	#define FLASH_STEP 4
	#define FLASH_PGM FLASH_ProgramWord
	#define FLASH_PAGE_SIZE         ((uint32_t)0x00000800)   /* FLASH Page Size */
	uint32_t GetPage(uint32_t Address);
#elif defined(STM32L1)
	#define FLASH_STEP 4
	#define FLASH_PGM FLASH_FastProgramWord
	#define FLASH_PAGE_SIZE         ((uint32_t)0x00000800)   /* FLASH Page Size */
	uint32_t GetPage(uint32_t Address);
#else
	#error "No valid device specified"
#endif

#ifndef FLASH_FLAG_PGERR
	#define FLASH_FLAG_PGERR 0
#endif

#ifndef FLASH_FLAG_WRPERR
	#define FLASH_FLAG_WRPERR 0
#endif

#define mmio64(x)   (*(volatile uint64_t *)(x))
#define mmio32(x)   (*(volatile uint32_t *)(x))
#define mmio16(x)   (*(volatile uint16_t *)(x))
#define mmio8(x)   (*(volatile uint8_t *)(x))

#define PARAMS_ADDR ((uint32_t)0x200007D0) // Parameters address in the ram.

#define MASK_STRT (1<<0) // Start bit
#define MASK_BUSY (1<<1) // Busy bit
#define MASK_SUCCESS (1<<2) // Success bit
#define MASK_DEL (1<<3) // Delete Success bit

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

	uint32_t i, from, to;
	uint32_t erased_sectors = 0;
	for (i=0;i < PARAMS_LEN ;i+=4) { // Clear parameters
		mmio32(PARAMS_ADDR+i) = 0;
	}
	
	while (1) {
		
		asm volatile ("bkpt"); // Halt core after init and before writing to flash.
		asm volatile ("nop"); 
		
		PARAMS->STATUS &= ~MASK_STRT; // Clear start bit
		PARAMS->STATUS  &= ~MASK_ERR; // Clear error bit
		PARAMS->STATUS  &= ~MASK_SUCCESS; // Clear success bit
		PARAMS->STATUS &= ~MASK_DEL; // Clear delete success bit
		
		FLASH_Unlock();
		
		from = PARAMS->DEST;
		to = from + PARAMS->LEN;
		
		// Erase flash where needed
		#if defined(STM32F2) || defined(STM32F4)
			FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR); 
			// Get the number of the start and end sectors
			const uint32_t StartSector = GetSector(from);
			const uint32_t EndSector = GetSector(to);
			uint32_t a;
			for (a = StartSector ; a <= EndSector ; a+=8) {
				if (erased_sectors & (1 << a/8)) continue; // Skip sectors already erased
				if (FLASH_EraseSector(a, VoltageRange_3) != FLASH_COMPLETE) {
					PARAMS->STATUS |= MASK_ERR;
					break;
				}
				erased_sectors |= 1 << a/8;
				PARAMS->STATUS |= MASK_DEL; // Set delete success bit
			}
		#else
			FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR); 
			uint32_t NbrOfPage = (to - from) / FLASH_PAGE_SIZE;
			//~ uint32_t EraseCounter;
			uint32_t a;
			for (a = 0 ; a <= NbrOfPage ; a++) {
				if (erased_sectors >= from + (FLASH_PAGE_SIZE * a)) continue; // Skip sectors already erased
				if (FLASH_ErasePage(from + (FLASH_PAGE_SIZE * a))!= FLASH_COMPLETE) {
					PARAMS->STATUS |= MASK_ERR;
					break;
				}
				erased_sectors = from + (FLASH_PAGE_SIZE * a);
				PARAMS->STATUS |= MASK_DEL; // Set delete success bit
			}
		#endif
			
		if (PARAMS->STATUS & MASK_ERR) { // If error during page delete, go back to breakpoint
			FLASH_Lock();
			continue;
		}
		
		// Flash programming
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
		
		FLASH_Lock(); // Lock flash after operations are done.
	} 
	return 0;
}

#if defined(STM32F2) || defined(STM32F4)
/**
  * @brief  Gets the sector of a given address
  * @param  None
  * @retval The sector of a given address
  */
uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0;
  
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_Sector_0;  
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_Sector_1;  
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_Sector_2;  
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_Sector_3;  
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_Sector_4;  
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_Sector_5;  
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_Sector_6;  
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_Sector_7;  
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_Sector_8;  
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_Sector_9;  
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_Sector_10;  
  }
  else/*(Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11))*/
  {
    sector = FLASH_Sector_11;  
  }

  return sector;
}

#else

uint32_t GetPage(uint32_t Address)
{
	const uint32_t base = ((uint32_t)0x08000000);
	return ((base - Address) / 1024);
}
#endif
