# gpio---DMA-speed

This is a simple DMA test from memory to GPIOx->ODR because I have found different STM32 blue pill boards have wildly different performance.
It sets up TIM4 to issue DMA every 28.44us and raises PC14 when DMA starts, and resets PC14 on DMA1_IT_TC5 so you can easily measure it using a scope.

I get 20.6us on good stm32 and ~26us on others for reasons I don't really understand (slow SRAM?)
