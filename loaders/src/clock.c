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
#elif defined(STM32F1_LOW_MED) ||defined(STM32F1)
    #include <stm32f10x.h>
#elif defined(STM32F2)
    #include <stm32f2xx.h>
#elif defined(STM32F30)
    #include <stm32f30x.h>
#elif defined(STM32F37)
    #include <stm32f37x.h>
#elif defined(STM32F4)
    #include <stm32f4xx.h>
#elif defined(STM32L1)
    #include <stm32l1xx.h>
#else
    #error "No valid device specified"
#endif


/* Run at 24 MHz */
void stm32_clock_init(void) {

#if !STM32_NO_INIT
  /* HSI setup, it enforces the reset situation in order to handle possible
     problems with JTAG probes and re-initializations.*/
  /* PWR clock enable.*/
  RCC->APB1ENR = RCC_APB1ENR_PWREN;

  RCC->CR |= RCC_CR_HSION;                  /* Make sure HSI is ON.         */
  while (!(RCC->CR & RCC_CR_HSIRDY))
    ;                                       /* Wait until HSI is stable.    */

  /* HSI is selected as new source without touching the other fields in
     CFGR. Clearing the register has to be postponed after HSI is the
     new source.*/
  RCC->CFGR &= ~RCC_CFGR_SW;                /* Reset SW */
  RCC->CFGR |= RCC_CFGR_SWS_HSI;            /* Select HSI as internal*/
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI)
    ;                                       /* Wait until HSI is selected.  */

#if defined(STM32F30) || defined(STM32F37) || defined(STM32F1) || defined(STM32F0)
  /* Clock settings.*/
  RCC->CFGR  |= RCC_CFGR_PLLSRC_HSI_Div2 | RCC_CFGR_PLLMULL6 ;

#elif defined(STM32F4)

  /* reset PLL config register */
  RCC->PLLCFGR = 0;
  /* set HSI as source for the PLL */
  RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSI;
  /* set division factor for main PLL input clock to 16 */
  RCC->PLLCFGR |= RCC_PLLCFGR_PLLM_4;
  /* set main PLL multiplication factor for VCO to 168 */
   RCC->PLLCFGR |= RCC_PLLCFGR_PLLN_3 | RCC_PLLCFGR_PLLN_5 | RCC_PLLCFGR_PLLN_7;
  /* set main PLL division factor for main system clock to 4 */
  RCC->PLLCFGR |= RCC_PLLCFGR_PLLP_0;

#elif defined(STM32L1)

  /* Core voltage setup.*/
  while ((PWR->CSR & PWR_CSR_VOSF) != 0)
    ;                           /* Waits until regulator is stable.         */
  PWR->CR = (1 << 11);
  while ((PWR->CSR & PWR_CSR_VOSF) != 0)
    ;                           /* Waits until regulator is stable.         */

  RCC->CFGR  = 0;
  RCC->ICSCR = (RCC->ICSCR & ~RCC_ICSCR_MSIRANGE ) |  RCC_ICSCR_MSIRANGE_5;
  RCC->CR    = RCC_CR_MSION;
  while ((RCC->CR & RCC_CR_MSIRDY) == 0)
    ;                           /* Waits until MSI is stable.               */

  RCC->CFGR  = RCC_CFGR_PLLDIV2 | RCC_CFGR_PLLMUL3 | RCC_CFGR_PLLSRC ;


#endif

  /* PLL activation.*/
  RCC->CR   |= RCC_CR_PLLON;
  while (!(RCC->CR & RCC_CR_PLLRDY))
    ;                                       /* Waits until PLL is stable.   */

  /* Switches clock source.*/
  /* set sysclock to be driven by the PLL clock */
  RCC->CFGR |= RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & RCC_CFGR_SWS) != (RCC_CFGR_SW_PLL << 2))
    ;                                       /* Waits selection complete.    */

#endif /* !STM32_NO_INIT */
}
