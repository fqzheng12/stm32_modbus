#include "led.h"

void LED_Init(void)
{

    GPIO_InitTypeDef GPIO_InitStucture;;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStucture.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_2;
    GPIO_InitStucture.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStucture.GPIO_Speed = GPIO_Speed_50MHz;
	
//	GPIO_InitStucture.GPIO_Pin = GPIO_Pin_2;
//    GPIO_InitStucture.GPIO_Mode = GPIO_Mode_Out_PP;
//    GPIO_InitStucture.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOB, &GPIO_InitStucture);
	
//	GPIO_SetBits(GPIOB,GPIO_Pin_0);
//	GPIO_SetBits(GPIOB,GPIO_Pin_2);
		GPIO_ResetBits(GPIOB,GPIO_Pin_0|GPIO_Pin_2);
//	GPIO_ResetBits(GPIOB,GPIO_Pin_2);
}

