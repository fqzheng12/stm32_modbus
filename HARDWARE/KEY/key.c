#include "key.h"
#include "delay.h"
#include "led.h"

//Ĭ������ ����Ϊ1 ����Ϊ0
void KEY_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

u8 KEY_Scan(u8 mode)
{
    static u8 key_up = 1;			//�����ɿ���־
    if(mode)
        key_up = 1;
    if(key_up && KEY == 1)		   //��������
    {
        delay_ms(10);
        key_up = 0;
        if(KEY == 1)
            return 1;
    }
    else if(KEY == 0)				//�����ɿ�
        key_up = 1;

    return 0;
}




