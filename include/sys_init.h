#ifndef SYS_INIT_H
#define SYS_INIT_H

#include "system_stm32f7xx.h"
#include "stm32f746xx.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t SystemCoreClock = 12000000;

#define PLL_CONFIG \
(_VAL2FLD(RCC_PLLCFGR_PLLM, 15) | _VAL2FLD(RCC_PLLCFGR_PLLN, 144) | _VAL2FLD(RCC_PLLCFGR_PLLP, 0) \
| _VAL2FLD(RCC_PLLCFGR_PLLQ, 5) |  _VAL2FLD(RCC_PLLCFGR_PLLSRC, 1))

#define MCO_CONFIG _VAL2FLD(RCC_CFGR_MCO1, 3) | _VAL2FLD(RCC_CFGR_MCO1PRE, 4)


void configureClock();
void configureMCO1();

#ifdef __cplusplus
}
#endif

#endif