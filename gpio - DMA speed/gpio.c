#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"

// debugging signals
#define DMA_YES GPIOC->BSRR = GPIO_Pin_14;
#define DMA_NO 	GPIOC->BRR = GPIO_Pin_14;


// Every 28.444us DMA is started
// on my original stm32, DMA activity shown on PC14 is 20.6us
// eBay purchase stm32, DMA activity shown on PC14 is 26.1us
uint32_t DMABuffer[33];

 void TIM4_IRQHandler(void)
{
	
	uint32_t itstatus = TIM4->SR & (TIM_IT_Update | TIM_IT_CC2);
	if (itstatus & TIM_IT_Update)
	{
		TIM4->SR = ~TIM_IT_Update;
		
		// debugging indicators
		DMA_YES;
		
		DMA1_Channel5->CCR = DMA_CCR5_MINC | DMA_CCR5_DIR | DMA_CCR5_TCIE | DMA_CCR5_EN;
	
	}
}

//  DMA to GPIOA
void DMA1_Channel5_IRQHandler(void)
{
		// debugging indicators
		DMA_NO;

		// setup for next time
		DMA1_Channel5->CCR = DMA_CCR5_MINC | DMA_CCR5_DIR | DMA_CCR5_TCIE;
		DMA1_Channel5->CNDTR = sizeof(DMABuffer);

		DMA1->IFCR = DMA1_IT_TC5 ;
		//DMA_ClearITPendingBit(DMA1_IT_TC5);
}

int main()
{

	//Enable clock for GPOIC
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitTypeDef GPIO_InitDef;
	GPIO_StructInit(&GPIO_InitDef);
	GPIO_InitDef.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14;
	GPIO_InitDef.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitDef.GPIO_Speed = GPIO_Speed_50MHz;	 
	GPIO_Init(GPIOC, &GPIO_InitDef);

	// DMA target GPIO
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	
	GPIO_StructInit(&GPIO_InitDef);
	GPIO_InitDef.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
	GPIO_InitDef.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitDef.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitDef);

	// 72MHz
	// TIM1_UP can only request on DM1_Channel5
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	TIM_TimeBaseInitTypeDef timerInitStructure;
	timerInitStructure.TIM_Prescaler = 0;
	timerInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	timerInitStructure.TIM_Period = 1;																// FIXME:  why does this not delay image?
	timerInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	timerInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1, &timerInitStructure);

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 , ENABLE );
	DMA_InitTypeDef DMA_InitStructure;
	DMA_InitStructure.DMA_BufferSize = sizeof(DMABuffer);
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)DMABuffer;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;		// NB reading 4 bytes into FIFO
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&GPIOA->ODR;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);

	// Enable TIM1 DRQ to DMA
	TIM_DMACmd(TIM1, TIM_DMA_Update , ENABLE);	

	/* Interrupt when DMA is complete */
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;		// this has to have absolute priority as its a direct timing signal
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);
	DMA_Cmd(DMA1_Channel5, DISABLE);
	TIM_Cmd(TIM1, ENABLE);
	

	// TIMER4 triggers DMA
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	timerInitStructure.TIM_Prescaler = 0;	
	timerInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	timerInitStructure.TIM_Period = 2048;
	timerInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	timerInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM4, &timerInitStructure);
	TIM4->SMCR  = TIM_SMCR_MSM;  
	TIM_Cmd(TIM4, ENABLE);
		
	// TIM4 IRQ handler
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
	
	for (;;)
		__NOP();
	
}
