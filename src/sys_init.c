/*Sysclock 120Mhz
P = 2, Q = 5, M = 15, N = 144
APB1 = 4, APB2 = 2
AHB = 1 valor default nem precisa alterar*/
#include "sys_init.h"
//Registrador PLLP 0 = divisor 2

uint32_t SystemCoreClock = 12000000;

void configureClock() {
    //alterando valor de latencia no registrador FLASH_ACR para 3 wait states
    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_3WS << FLASH_ACR_LATENCY_Pos);
    //Enable HSE
    SET_BIT(RCC->CR, RCC_CR_HSEON);
    //Esperar até HSE estar estável
    while(!READ_BIT(RCC->CR, RCC_CR_HSERDY));
    //Configurar PLL (HSE_CLK / M * N) / Q = USB_CLK
    MODIFY_REG(RCC->PLLCFGR, 
    RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLP | RCC_PLLCFGR_PLLQ | RCC_PLLCFGR_PLLSRC,
    PLL_CONFIG
     );
    //Enable PLL datasheet tem que habilitar o pll depois de configurar
    SET_BIT(RCC->CR, RCC_CR_PLLON);
    //Esperar PLL estar estável
    while(!READ_BIT(RCC->CR, RCC_CR_PLLRDY));
    //Muda clock do sistema para PLL
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, _VAL2FLD(RCC_CFGR_SW, RCC_CFGR_SW_PLL));
    //Configurar prescaler APB1 de acordo com datasheet 101 (5) = prescaler 4
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, _VAL2FLD(RCC_CFGR_PPRE1, 5));
    //Configurar prescale APB2 100 (4) = prescaler 2
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, _VAL2FLD(RCC_CFGR_PPRE2, 4));
    //Esperar até PLL ser usado
    while(READ_BIT(RCC->CFGR, RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
    //Desabilitar HSI
    CLEAR_BIT(RCC->CR, RCC_CR_HSION);
    SystemCoreClockUpdate();
}

void configureMCO1() {
    //Configurar MCO1 para testar clock no osciloscópio 
    //MCOPRE1 = 2 (100b) MCO1SRC = PLL (11b)
    MODIFY_REG(RCC->CFGR, RCC_CFGR_MCO1 | RCC_CFGR_MCO1PRE, MCO_CONFIG);
    //Enable GPIOA Clock
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);
    //Selecionar GPIOA pino 8 como very high speed PA8 AHB1 usa clock de 120MHz
    MODIFY_REG(GPIOA->OSPEEDR, GPIO_OSPEEDER_OSPEEDR8, _VAL2FLD(GPIO_OSPEEDER_OSPEEDR8, 3));
    //Selecionar pino PA8 para alternate function mode (10b)
    MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER8, _VAL2FLD(GPIO_MODER_MODER8, 2));
}

