#ifndef __UART2_H
#define __UART2_H
#include "sys.h"

#define DMA_REC_LEN 50																			//DMA数据接收缓冲区

void  uart2_init(u16 baud);
void uartDMA_Init( void );
void myDMA_Enable( DMA_Channel_TypeDef*DMA_CHx );
void uart2_Send( u8 *buf, u16 len );
void copy_data( u8 *buf, u16 len );
#endif

