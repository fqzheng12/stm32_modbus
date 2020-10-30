#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "uart2.h"
#include "modbus.h"
#include "string.h"

extern u8 data_backup[DMA_REC_LEN]; 													//���ݱ���
extern u16 dataLen_backup;																			//���ȱ���
extern _Bool receiveOK_flag;																		//������ɱ�־λ
u8 senddate[2]={0x55,0x15};
int main(void)
{
    u8  j = 0;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
    delay_init();       //��ʱ������ʼ��
    LED_Init();         //��ʼ����LED���ӵ�Ӳ���ӿ�
    uart2_init(9600);
//	GPIO_SetBits(GPIOB,GPIO_Pin_2);
    while(1)
    {
//     uart2_Send(senddate,2);
        if( receiveOK_flag )																		//һ֡���ݽ�����ɺ�ʼ��������
        {
            receiveOK_flag = 0;
            DisposeReceive();																		//����modbusͨ��
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


