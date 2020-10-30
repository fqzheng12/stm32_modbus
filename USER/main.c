#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "uart2.h"
#include "modbus.h"
#include "string.h"

extern u8 data_backup[DMA_REC_LEN]; 													//数据备份
extern u16 dataLen_backup;																			//长度备份
extern _Bool receiveOK_flag;																		//接收完成标志位
u8 senddate[2]={0x55,0x15};
int main(void)
{
    u8  j = 0;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
    delay_init();       //延时函数初始化
    LED_Init();         //初始化与LED连接的硬件接口
    uart2_init(9600);
//	GPIO_SetBits(GPIOB,GPIO_Pin_2);
    while(1)
    {
//     uart2_Send(senddate,2);
        if( receiveOK_flag )																		//一帧数据接收完成后开始处理数据
        {
            receiveOK_flag = 0;
            DisposeReceive();																		//处理modbus通信
        }
			
//        j++;
//        if(j > 50)
//        {
//            j = 0;
//            LED = !LED;
//        }
        delay_ms(1000);
    }
}


