#include <stdint.h>

typedef void (*isr_handler_t)(void);

extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

extern int main(void);
extern void SystemInit(void);
extern void __libc_init_array(void);

void Reset_Handler(void);
void Default_Handler(void);

#define WEAK_DEFAULT_HANDLER(name) \
    void name(void) __attribute__((weak, alias("Default_Handler")))

WEAK_DEFAULT_HANDLER(NMI_Handler);
WEAK_DEFAULT_HANDLER(HardFault_Handler);
WEAK_DEFAULT_HANDLER(MemManage_Handler);
WEAK_DEFAULT_HANDLER(BusFault_Handler);
WEAK_DEFAULT_HANDLER(UsageFault_Handler);
WEAK_DEFAULT_HANDLER(SVC_Handler);
WEAK_DEFAULT_HANDLER(DebugMon_Handler);
WEAK_DEFAULT_HANDLER(PendSV_Handler);
WEAK_DEFAULT_HANDLER(SysTick_Handler);
WEAK_DEFAULT_HANDLER(WWDG_IRQHandler);
WEAK_DEFAULT_HANDLER(PVD_IRQHandler);
WEAK_DEFAULT_HANDLER(TAMP_STAMP_IRQHandler);
WEAK_DEFAULT_HANDLER(RTC_WKUP_IRQHandler);
WEAK_DEFAULT_HANDLER(FLASH_IRQHandler);
WEAK_DEFAULT_HANDLER(RCC_IRQHandler);
WEAK_DEFAULT_HANDLER(EXTI0_IRQHandler);
WEAK_DEFAULT_HANDLER(EXTI1_IRQHandler);
WEAK_DEFAULT_HANDLER(EXTI2_IRQHandler);
WEAK_DEFAULT_HANDLER(EXTI3_IRQHandler);
WEAK_DEFAULT_HANDLER(EXTI4_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA1_Stream0_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA1_Stream1_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA1_Stream2_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA1_Stream3_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA1_Stream4_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA1_Stream5_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA1_Stream6_IRQHandler);
WEAK_DEFAULT_HANDLER(ADC_IRQHandler);
WEAK_DEFAULT_HANDLER(CAN1_TX_IRQHandler);
WEAK_DEFAULT_HANDLER(CAN1_RX0_IRQHandler);
WEAK_DEFAULT_HANDLER(CAN1_RX1_IRQHandler);
WEAK_DEFAULT_HANDLER(CAN1_SCE_IRQHandler);
WEAK_DEFAULT_HANDLER(EXTI9_5_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM1_BRK_TIM9_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM1_UP_TIM10_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM1_TRG_COM_TIM11_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM1_CC_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM2_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM3_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM4_IRQHandler);
WEAK_DEFAULT_HANDLER(I2C1_EV_IRQHandler);
WEAK_DEFAULT_HANDLER(I2C1_ER_IRQHandler);
WEAK_DEFAULT_HANDLER(I2C2_EV_IRQHandler);
WEAK_DEFAULT_HANDLER(I2C2_ER_IRQHandler);
WEAK_DEFAULT_HANDLER(SPI1_IRQHandler);
WEAK_DEFAULT_HANDLER(SPI2_IRQHandler);
WEAK_DEFAULT_HANDLER(USART1_IRQHandler);
WEAK_DEFAULT_HANDLER(USART2_IRQHandler);
WEAK_DEFAULT_HANDLER(USART3_IRQHandler);
WEAK_DEFAULT_HANDLER(EXTI15_10_IRQHandler);
WEAK_DEFAULT_HANDLER(RTC_Alarm_IRQHandler);
WEAK_DEFAULT_HANDLER(OTG_FS_WKUP_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM8_BRK_TIM12_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM8_UP_TIM13_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM8_TRG_COM_TIM14_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM8_CC_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA1_Stream7_IRQHandler);
WEAK_DEFAULT_HANDLER(FSMC_IRQHandler);
WEAK_DEFAULT_HANDLER(SDIO_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM5_IRQHandler);
WEAK_DEFAULT_HANDLER(SPI3_IRQHandler);
WEAK_DEFAULT_HANDLER(UART4_IRQHandler);
WEAK_DEFAULT_HANDLER(UART5_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM6_DAC_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM7_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA2_Stream0_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA2_Stream1_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA2_Stream2_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA2_Stream3_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA2_Stream4_IRQHandler);
WEAK_DEFAULT_HANDLER(ETH_IRQHandler);
WEAK_DEFAULT_HANDLER(ETH_WKUP_IRQHandler);
WEAK_DEFAULT_HANDLER(CAN2_TX_IRQHandler);
WEAK_DEFAULT_HANDLER(CAN2_RX0_IRQHandler);
WEAK_DEFAULT_HANDLER(CAN2_RX1_IRQHandler);
WEAK_DEFAULT_HANDLER(CAN2_SCE_IRQHandler);
WEAK_DEFAULT_HANDLER(OTG_FS_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA2_Stream5_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA2_Stream6_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA2_Stream7_IRQHandler);
WEAK_DEFAULT_HANDLER(USART6_IRQHandler);
WEAK_DEFAULT_HANDLER(I2C3_EV_IRQHandler);
WEAK_DEFAULT_HANDLER(I2C3_ER_IRQHandler);
WEAK_DEFAULT_HANDLER(OTG_HS_EP1_OUT_IRQHandler);
WEAK_DEFAULT_HANDLER(OTG_HS_EP1_IN_IRQHandler);
WEAK_DEFAULT_HANDLER(OTG_HS_WKUP_IRQHandler);
WEAK_DEFAULT_HANDLER(OTG_HS_IRQHandler);
WEAK_DEFAULT_HANDLER(DCMI_IRQHandler);
WEAK_DEFAULT_HANDLER(HASH_RNG_IRQHandler);
WEAK_DEFAULT_HANDLER(FPU_IRQHandler);

__attribute__((used, section(".isr_vector")))
const isr_handler_t g_pfnVectors[] = {
    (isr_handler_t)&_estack,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0,
    0,
    0,
    0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,
    WWDG_IRQHandler,
    PVD_IRQHandler,
    TAMP_STAMP_IRQHandler,
    RTC_WKUP_IRQHandler,
    FLASH_IRQHandler,
    RCC_IRQHandler,
    EXTI0_IRQHandler,
    EXTI1_IRQHandler,
    EXTI2_IRQHandler,
    EXTI3_IRQHandler,
    EXTI4_IRQHandler,
    DMA1_Stream0_IRQHandler,
    DMA1_Stream1_IRQHandler,
    DMA1_Stream2_IRQHandler,
    DMA1_Stream3_IRQHandler,
    DMA1_Stream4_IRQHandler,
    DMA1_Stream5_IRQHandler,
    DMA1_Stream6_IRQHandler,
    ADC_IRQHandler,
    CAN1_TX_IRQHandler,
    CAN1_RX0_IRQHandler,
    CAN1_RX1_IRQHandler,
    CAN1_SCE_IRQHandler,
    EXTI9_5_IRQHandler,
    TIM1_BRK_TIM9_IRQHandler,
    TIM1_UP_TIM10_IRQHandler,
    TIM1_TRG_COM_TIM11_IRQHandler,
    TIM1_CC_IRQHandler,
    TIM2_IRQHandler,
    TIM3_IRQHandler,
    TIM4_IRQHandler,
    I2C1_EV_IRQHandler,
    I2C1_ER_IRQHandler,
    I2C2_EV_IRQHandler,
    I2C2_ER_IRQHandler,
    SPI1_IRQHandler,
    SPI2_IRQHandler,
    USART1_IRQHandler,
    USART2_IRQHandler,
    USART3_IRQHandler,
    EXTI15_10_IRQHandler,
    RTC_Alarm_IRQHandler,
    OTG_FS_WKUP_IRQHandler,
    TIM8_BRK_TIM12_IRQHandler,
    TIM8_UP_TIM13_IRQHandler,
    TIM8_TRG_COM_TIM14_IRQHandler,
    TIM8_CC_IRQHandler,
    DMA1_Stream7_IRQHandler,
    FSMC_IRQHandler,
    SDIO_IRQHandler,
    TIM5_IRQHandler,
    SPI3_IRQHandler,
    UART4_IRQHandler,
    UART5_IRQHandler,
    TIM6_DAC_IRQHandler,
    TIM7_IRQHandler,
    DMA2_Stream0_IRQHandler,
    DMA2_Stream1_IRQHandler,
    DMA2_Stream2_IRQHandler,
    DMA2_Stream3_IRQHandler,
    DMA2_Stream4_IRQHandler,
    ETH_IRQHandler,
    ETH_WKUP_IRQHandler,
    CAN2_TX_IRQHandler,
    CAN2_RX0_IRQHandler,
    CAN2_RX1_IRQHandler,
    CAN2_SCE_IRQHandler,
    OTG_FS_IRQHandler,
    DMA2_Stream5_IRQHandler,
    DMA2_Stream6_IRQHandler,
    DMA2_Stream7_IRQHandler,
    USART6_IRQHandler,
    I2C3_EV_IRQHandler,
    I2C3_ER_IRQHandler,
    OTG_HS_EP1_OUT_IRQHandler,
    OTG_HS_EP1_IN_IRQHandler,
    OTG_HS_WKUP_IRQHandler,
    OTG_HS_IRQHandler,
    DCMI_IRQHandler,
    0,
    HASH_RNG_IRQHandler,
    FPU_IRQHandler,
};

void Reset_Handler(void)
{
    uint32_t* source = &_sidata;

    for (uint32_t* destination = &_sdata; destination < &_edata; ++destination) {
        *destination = *source++;
    }

    for (uint32_t* destination = &_sbss; destination < &_ebss; ++destination) {
        *destination = 0U;
    }

    SystemInit();
    __libc_init_array();
    (void)main();

    while (1) {
    }
}

void Default_Handler(void)
{
    while (1) {
    }
}
