//串口2空闲中断 + DMA数据传输
#include "uart2.h"
#define UART2_DMA  1																				//使用串口2  DMA传输
#define  CHECK_NONE_ONE_STOP    1 //无校验位  1个停止位  1有效  0 无效 
#define  CHECK_NONE_TWO_STOP    0 //无校验位  2个停止位  1有效  0 无效 
#define  CHECK_EVEN    0          //偶数校验   1有效  0 无效 
#define  CHECK_ODD     0          //奇数校验   1有效  0 无效 

u8 dma_rec_buff[DMA_REC_LEN] = {0};
u16 uart2_rec_cnt = 0;																			//串口接收数据长度

u8 data_backup[DMA_REC_LEN] = {0}; 													//数据备份
u16 dataLen_backup = 0;																			//长度备份
_Bool receiveOK_flag = 0;																		//接收完成标志位

/*
空闲中断是什么意思呢？
指的是当总线接收数据时，一旦数据流断了，此时总线没有接收传输，处于空闲状态，IDLE就会置1，产生空闲中断；又有数据发送时，IDLE位就会置0；
注意：置1之后它不会自动清0，也不会因为状态位是1而一直产生中断，它只有0跳变到1时才会产生，也可以理解为上升沿触发。
所以，为确保下次空闲中断正常进行，需要在中断服务函数发送任意数据来清除标志位。
*/

void  uart2_init( u16 baud )
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA, ENABLE );
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_USART2, ENABLE );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;												 //推挽复用模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init( GPIOA, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;									 //浮空输入模式
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
		
#if(CHECK_EVEN == 1) 																											 //如果定义了偶校验  数据位长度要改为9位
    USART_InitStructure.USART_WordLength = USART_WordLength_9b;
    USART_InitStructure.USART_Parity = USART_Parity_Even;
#endif

#if(CHECK_ODD == 1) 																											 //如果定义了奇校验  数据位长度要改为9位
    USART_InitStructure.USART_WordLength = USART_WordLength_9b;
    USART_InitStructure.USART_Parity = USART_Parity_Odd;
#endif

#if(CHECK_NONE_ONE_STOP==1)																							   //停止位为 一位
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
#endif

#if(CHECK_NONE_TWO_STOP==1)																								 //停止位为 两位																	
    USART_InitStructure.USART_StopBits = USART_StopBits_2;
#endif		
		
		
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    USART_Init( USART2, &USART_InitStructure );
		
#if (UART2_DMA == 1)
    USART_ITConfig( USART2, USART_IT_IDLE, ENABLE );											 //使能串口空闲中断
    USART_DMACmd( USART2, USART_DMAReq_Rx, ENABLE );											 //使能串口2 DMA接收
    uartDMA_Init();																												 //初始化 DMA
#else
    USART_ITConfig( USART2, USART_IT_RXNE, ENABLE );			  							 //使能串口RXNE接收中断
#endif
    USART_Cmd( USART2, ENABLE );																					 //使能串口2
		
//RXNE中断和IDLE中断的区别？
//当接收到1个字节，就会产生RXNE中断，当接收到一帧数据，就会产生IDLE中断。比如给单片机一次性发送了8个字节，就会产生8次RXNE中断，1次IDLE中断。
}

void uartDMA_Init( void )
{
    DMA_InitTypeDef  DMA_IniStructure;

    RCC_AHBPeriphClockCmd( RCC_AHBPeriph_DMA1, ENABLE ); 									 //使能DMA时钟

    DMA_DeInit( DMA1_Channel6 );																			     //DMA1通道6对应 USART2_RX
    DMA_IniStructure.DMA_PeripheralBaseAddr = ( u32 )&USART2->DR;					 //DMA外设usart基地址
    DMA_IniStructure.DMA_MemoryBaseAddr = ( u32 )dma_rec_buff; 						 //DMA内存基地址
    DMA_IniStructure.DMA_DIR = DMA_DIR_PeripheralSRC;											 //数据传输方向，从外设读取发送到内存
    DMA_IniStructure.DMA_BufferSize = DMA_REC_LEN;												 //DMA通道的DMA缓存的大小  也就是 DMA一次传输的字节数
    DMA_IniStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;				 //外设地址寄存器不变
    DMA_IniStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;								 //数据缓冲区地址递增
    DMA_IniStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; //外设以字节为单位搬运
    DMA_IniStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; 				 //数据缓冲区以字节为单位搬入
    DMA_IniStructure.DMA_Mode = DMA_Mode_Normal;													 //工作在正常缓存模式
    DMA_IniStructure.DMA_Priority = DMA_Priority_Medium;									 //DMA通道 x拥有中优先级
    DMA_IniStructure.DMA_M2M = DMA_M2M_Disable;														 //DMA通道x没有设置为内存到内存传输

    DMA_Init( DMA1_Channel6, &DMA_IniStructure );
    DMA_Cmd( DMA1_Channel6, ENABLE );
}

//重新恢复DMA指针
void myDMA_Enable( DMA_Channel_TypeDef*DMA_CHx )
{
    DMA_Cmd( DMA_CHx, DISABLE );  																				 //关闭DMA1所指示的通道
    DMA_SetCurrDataCounter( DMA_CHx, DMA_REC_LEN );												 //DMA通道的DMA缓存的大小
    DMA_Cmd( DMA_CHx, ENABLE );  																					 //DMA1所指示的通道
}
//发送len个字节
//buf:发送区首地址
//len:发送的字节数
void uart2_Send( u8 *buf, u16 len )
{
    u16 t;
	 GPIO_SetBits(GPIOB,GPIO_Pin_2);
//	GPIO_ResetBits(GPIOB,GPIO_Pin_2);
    for( t = 0; t < len; t++ )																						 //循环发送数据
    {
        while( USART_GetFlagStatus( USART2, USART_FLAG_TC ) == RESET );
			
        USART_SendData( USART2, buf[t] );
    }
    while( USART_GetFlagStatus( USART2, USART_FLAG_TC ) == RESET );
		GPIO_ResetBits(GPIOB,GPIO_Pin_2);
//		GPIO_SetBits(GPIOB,GPIO_Pin_2);
}

//备份接收到的数据
void copy_data( u8 *buf, u16 len )
{
    u16 t;
    dataLen_backup = len;																									 //保存数据长度
    for( t = 0; t < len; t++ )
    {
        data_backup[t] = buf[t];																					 //备份接收到的数据，防止在处理数据过程中接收到新数据，将旧数据覆盖掉。
    }
}
// Modbus RTU 通信协议
//地址码 功能码 寄存器高位 寄存器低位  数据高位  数据低位 CRC高位 CRC低位
//01     03      00          00         03         e8      xx        xx
//利用空闲中断接收串口不定长数据
//串口接收一组数据结束后才会进入中断,在数据发送过程中,已经接收到的数据将会被存入DMA的缓冲区中
void USART2_IRQHandler( void )
{
    u8 tem = 0;
#if (UART2_DMA == 1)			//如果使能了 UART2_DMA 则使用串口空闲中断和 DMA功能接收数据
	if( USART_GetITStatus( USART2, USART_IT_IDLE ) != RESET )							 			  //空闲中断  一帧数据发送完成
    {
			
        USART_ReceiveData( USART2 );																			 			//读取数据注意：这句必须要，否则不能够清除空闲中断标志位。
        DMA_Cmd( DMA1_Channel6, DISABLE );																 			//关闭 DMA 防止后续数据干扰
        uart2_rec_cnt = DMA_REC_LEN - DMA_GetCurrDataCounter( DMA1_Channel6 );  //DMA接收缓冲区数据长度减去当前 DMA传输通道中剩余单元数量就是已经接收到的数据数量
//        uart2_Send( dma_rec_buff, uart2_rec_cnt );														//发送接收到的数据
			  copy_data( dma_rec_buff, uart2_rec_cnt );														    //备份数据
        receiveOK_flag = 1;																											//置位数据接收完成标志位
        USART_ClearITPendingBit( USART2, USART_IT_IDLE );									 			//清除空闲中断标志位
        myDMA_Enable( DMA1_Channel6 );	
			//重新恢复 DMA等待下一次接收
//			GPIO_SetBits(GPIOB,GPIO_Pin_2);
//        USART_SendData( USART2, tem );
//			GPIO_ResetBits(GPIOB,GPIO_Pin_2);
    }
#else										//如果未使能 UART2_DMA 则通过常规方式接收数据  每接收到一个字节就会进入一次中断
    if( USART_GetITStatus( USART2, USART_IT_RXNE ) != RESET )						 //接收中断
    {
			 
			 
        tem = USART_ReceiveData( USART2 );
			GPIO_SetBits(GPIOB,GPIO_Pin_2);
        USART_SendData( USART2, tem );
			GPIO_ResetBits(GPIOB,GPIO_Pin_2);
    }
#endif
}

