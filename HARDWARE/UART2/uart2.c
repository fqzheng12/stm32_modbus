//����2�����ж� + DMA���ݴ���
#include "uart2.h"
#define UART2_DMA  1																				//ʹ�ô���2  DMA����
#define  CHECK_NONE_ONE_STOP    1 //��У��λ  1��ֹͣλ  1��Ч  0 ��Ч 
#define  CHECK_NONE_TWO_STOP    0 //��У��λ  2��ֹͣλ  1��Ч  0 ��Ч 
#define  CHECK_EVEN    0          //ż��У��   1��Ч  0 ��Ч 
#define  CHECK_ODD     0          //����У��   1��Ч  0 ��Ч 

u8 dma_rec_buff[DMA_REC_LEN] = {0};
u16 uart2_rec_cnt = 0;																			//���ڽ������ݳ���

u8 data_backup[DMA_REC_LEN] = {0}; 													//���ݱ���
u16 dataLen_backup = 0;																			//���ȱ���
_Bool receiveOK_flag = 0;																		//������ɱ�־λ

/*
�����ж���ʲô��˼�أ�
ָ���ǵ����߽�������ʱ��һ�����������ˣ���ʱ����û�н��մ��䣬���ڿ���״̬��IDLE�ͻ���1�����������жϣ��������ݷ���ʱ��IDLEλ�ͻ���0��
ע�⣺��1֮���������Զ���0��Ҳ������Ϊ״̬λ��1��һֱ�����жϣ���ֻ��0���䵽1ʱ�Ż������Ҳ�������Ϊ�����ش�����
���ԣ�Ϊȷ���´ο����ж��������У���Ҫ���жϷ������������������������־λ��
*/

void  uart2_init( u16 baud )
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA, ENABLE );
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_USART2, ENABLE );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;												 //���츴��ģʽ
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init( GPIOA, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;									 //��������ģʽ
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init( GPIOA, &GPIO_InitStructure );

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init( &NVIC_InitStructure );

    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
		
#if(CHECK_EVEN == 1) 																											 //���������żУ��  ����λ����Ҫ��Ϊ9λ
    USART_InitStructure.USART_WordLength = USART_WordLength_9b;
    USART_InitStructure.USART_Parity = USART_Parity_Even;
#endif

#if(CHECK_ODD == 1) 																											 //�����������У��  ����λ����Ҫ��Ϊ9λ
    USART_InitStructure.USART_WordLength = USART_WordLength_9b;
    USART_InitStructure.USART_Parity = USART_Parity_Odd;
#endif

#if(CHECK_NONE_ONE_STOP==1)																							   //ֹͣλΪ һλ
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
#endif

#if(CHECK_NONE_TWO_STOP==1)																								 //ֹͣλΪ ��λ																	
    USART_InitStructure.USART_StopBits = USART_StopBits_2;
#endif		
		
		
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    USART_Init( USART2, &USART_InitStructure );
		
#if (UART2_DMA == 1)
    USART_ITConfig( USART2, USART_IT_IDLE, ENABLE );											 //ʹ�ܴ��ڿ����ж�
    USART_DMACmd( USART2, USART_DMAReq_Rx, ENABLE );											 //ʹ�ܴ���2 DMA����
    uartDMA_Init();																												 //��ʼ�� DMA
#else
    USART_ITConfig( USART2, USART_IT_RXNE, ENABLE );			  							 //ʹ�ܴ���RXNE�����ж�
#endif
    USART_Cmd( USART2, ENABLE );																					 //ʹ�ܴ���2
		
//RXNE�жϺ�IDLE�жϵ�����
//�����յ�1���ֽڣ��ͻ����RXNE�жϣ������յ�һ֡���ݣ��ͻ����IDLE�жϡ��������Ƭ��һ���Է�����8���ֽڣ��ͻ����8��RXNE�жϣ�1��IDLE�жϡ�
}

void uartDMA_Init( void )
{
    DMA_InitTypeDef  DMA_IniStructure;

    RCC_AHBPeriphClockCmd( RCC_AHBPeriph_DMA1, ENABLE ); 									 //ʹ��DMAʱ��

    DMA_DeInit( DMA1_Channel6 );																			     //DMA1ͨ��6��Ӧ USART2_RX
    DMA_IniStructure.DMA_PeripheralBaseAddr = ( u32 )&USART2->DR;					 //DMA����usart����ַ
    DMA_IniStructure.DMA_MemoryBaseAddr = ( u32 )dma_rec_buff; 						 //DMA�ڴ����ַ
    DMA_IniStructure.DMA_DIR = DMA_DIR_PeripheralSRC;											 //���ݴ��䷽�򣬴������ȡ���͵��ڴ�
    DMA_IniStructure.DMA_BufferSize = DMA_REC_LEN;												 //DMAͨ����DMA����Ĵ�С  Ҳ���� DMAһ�δ�����ֽ���
    DMA_IniStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;				 //�����ַ�Ĵ�������
    DMA_IniStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;								 //���ݻ�������ַ����
    DMA_IniStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; //�������ֽ�Ϊ��λ����
    DMA_IniStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; 				 //���ݻ��������ֽ�Ϊ��λ����
    DMA_IniStructure.DMA_Mode = DMA_Mode_Normal;													 //��������������ģʽ
    DMA_IniStructure.DMA_Priority = DMA_Priority_Medium;									 //DMAͨ�� xӵ�������ȼ�
    DMA_IniStructure.DMA_M2M = DMA_M2M_Disable;														 //DMAͨ��xû������Ϊ�ڴ浽�ڴ洫��

    DMA_Init( DMA1_Channel6, &DMA_IniStructure );
    DMA_Cmd( DMA1_Channel6, ENABLE );
}

//���»ָ�DMAָ��
void myDMA_Enable( DMA_Channel_TypeDef*DMA_CHx )
{
    DMA_Cmd( DMA_CHx, DISABLE );  																				 //�ر�DMA1��ָʾ��ͨ��
    DMA_SetCurrDataCounter( DMA_CHx, DMA_REC_LEN );												 //DMAͨ����DMA����Ĵ�С
    DMA_Cmd( DMA_CHx, ENABLE );  																					 //DMA1��ָʾ��ͨ��
}
//����len���ֽ�
//buf:�������׵�ַ
//len:���͵��ֽ���
void uart2_Send( u8 *buf, u16 len )
{
    u16 t;
	 GPIO_SetBits(GPIOB,GPIO_Pin_2);
//	GPIO_ResetBits(GPIOB,GPIO_Pin_2);
    for( t = 0; t < len; t++ )																						 //ѭ����������
    {
        while( USART_GetFlagStatus( USART2, USART_FLAG_TC ) == RESET );
			
        USART_SendData( USART2, buf[t] );
    }
    while( USART_GetFlagStatus( USART2, USART_FLAG_TC ) == RESET );
		GPIO_ResetBits(GPIOB,GPIO_Pin_2);
//		GPIO_SetBits(GPIOB,GPIO_Pin_2);
}

//���ݽ��յ�������
void copy_data( u8 *buf, u16 len )
{
    u16 t;
    dataLen_backup = len;																									 //�������ݳ���
    for( t = 0; t < len; t++ )
    {
        data_backup[t] = buf[t];																					 //���ݽ��յ������ݣ���ֹ�ڴ������ݹ����н��յ������ݣ��������ݸ��ǵ���
    }
}
// Modbus RTU ͨ��Э��
//��ַ�� ������ �Ĵ�����λ �Ĵ�����λ  ���ݸ�λ  ���ݵ�λ CRC��λ CRC��λ
//01     03      00          00         03         e8      xx        xx
//���ÿ����жϽ��մ��ڲ���������
//���ڽ���һ�����ݽ�����Ż�����ж�,�����ݷ��͹�����,�Ѿ����յ������ݽ��ᱻ����DMA�Ļ�������
void USART2_IRQHandler( void )
{
    u8 tem = 0;
#if (UART2_DMA == 1)			//���ʹ���� UART2_DMA ��ʹ�ô��ڿ����жϺ� DMA���ܽ�������
	if( USART_GetITStatus( USART2, USART_IT_IDLE ) != RESET )							 			  //�����ж�  һ֡���ݷ������
    {
			
        USART_ReceiveData( USART2 );																			 			//��ȡ����ע�⣺������Ҫ�������ܹ���������жϱ�־λ��
        DMA_Cmd( DMA1_Channel6, DISABLE );																 			//�ر� DMA ��ֹ�������ݸ���
        uart2_rec_cnt = DMA_REC_LEN - DMA_GetCurrDataCounter( DMA1_Channel6 );  //DMA���ջ��������ݳ��ȼ�ȥ��ǰ DMA����ͨ����ʣ�൥Ԫ���������Ѿ����յ�����������
//        uart2_Send( dma_rec_buff, uart2_rec_cnt );														//���ͽ��յ�������
			  copy_data( dma_rec_buff, uart2_rec_cnt );														    //��������
        receiveOK_flag = 1;																											//��λ���ݽ�����ɱ�־λ
        USART_ClearITPendingBit( USART2, USART_IT_IDLE );									 			//��������жϱ�־λ
        myDMA_Enable( DMA1_Channel6 );	
			//���»ָ� DMA�ȴ���һ�ν���
//			GPIO_SetBits(GPIOB,GPIO_Pin_2);
//        USART_SendData( USART2, tem );
//			GPIO_ResetBits(GPIOB,GPIO_Pin_2);
    }
#else										//���δʹ�� UART2_DMA ��ͨ�����淽ʽ��������  ÿ���յ�һ���ֽھͻ����һ���ж�
    if( USART_GetITStatus( USART2, USART_IT_RXNE ) != RESET )						 //�����ж�
    {
			 
			 
        tem = USART_ReceiveData( USART2 );
			GPIO_SetBits(GPIOB,GPIO_Pin_2);
        USART_SendData( USART2, tem );
			GPIO_ResetBits(GPIOB,GPIO_Pin_2);
    }
#endif
}

