#ifndef __CRC16_H
#define __CRC16_H
#include "sys.h"

u16 App_Tab_Get_CRC16( u8 * pucFrame, u16 usLen );              //�������CRC
u16 App_Calc_Get_CRC16( u8 *ptr,u8 len);                        //ֱ�������CRC

#endif

