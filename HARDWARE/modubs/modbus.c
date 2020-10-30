#include "modbus.h"
#include "crc16.h"
#include "uart2.h"

#define HoldRegStartAddr    0x0000																						//保持寄存器起始地址
#define HoldRegCount        9																									//保持寄存器数量
#define HoldMaxValue        1000            																	//寄存器最大值

extern u8 data_backup[DMA_REC_LEN]; 													  							//modbus接收到数据备份
extern u16 dataLen_backup;																										//modbus接收数据长度备份


u8 SendBuf[DMA_REC_LEN] = {0};
u16 StartRegAddr = HoldRegStartAddr;
u8 err = 0;                                                        						//错误代码

u8 HoldReg[HoldRegCount*2] = {0, 1, 0, 2, 0, 3, 0, 4,0, 5, 0, 6, 0, 7, 0, 8,0,9}; //存储8路数据值，尝试修改


/*
错误代码说明:
0x01  不是支持的功能码
0x02  起始地址不在规定范围内
0x03  寄存器数量不在规定范围内
0x04  数据校验错误
*/
//处理接收到的数据
// 接收: [地址][功能码][起始地址高][起始地址低][总寄存器数高][总寄存器数低][CRC低][CRC高]
void DisposeReceive( void )
{
    u16 CRC16 = 0, CRC16Temp = 0;
    if( data_backup[0] == SlaveID )                                 					//地址等于本机地址 地址范围:1 - 32
    {
        CRC16 = App_Tab_Get_CRC16( data_backup, dataLen_backup - 2 );  				//CRC校验 低字节在前 高字节在后 高字节为报文最后一个字节
        CRC16Temp = ( ( u16 )( data_backup[dataLen_backup - 1] << 8 ) | data_backup[dataLen_backup - 2] );
        if( CRC16 != CRC16Temp )
        {
            err = 4;                                               						//CRC校验错误
        }
        StartRegAddr = ( u16 )( data_backup[2] << 8 ) | data_backup[3];
        if( StartRegAddr > (HoldRegStartAddr + HoldRegCount - 1) ) 
        {
            err = 2;                                               						//起始地址不在规定范围内 00 - 07    1 - 8号通道
        }
        if( err == 0 )
        {
            switch( data_backup[1] )                                					//功能码
            {
                case 3:                                            						//读多个寄存器
                {
                    Modbus_03_Slave();
                    break;
                }
                case 6:                                            						//写单个寄存器
                {
                    Modbus_06_Slave();
                    break;
                }
                case 16:                                           						//写多个寄存器
                {
                    Modbus_16_Slave();
                    break;
                }
                default:
                {
                    err = 1;                                       						//不支持该功能码
                    break;
                }
            }
        }
        if( err > 0 )
        {
            SendBuf[0] = data_backup[0];
            SendBuf[1] = data_backup[1] | 0x80;
            SendBuf[2] = err;                                      						//发送错误代码
            CRC16Temp = App_Tab_Get_CRC16( SendBuf, 3 );           						//计算CRC校验值
            SendBuf[3] = CRC16Temp & 0xFF;                         						//CRC低位
            SendBuf[4] = ( CRC16Temp >> 8 );                       						//CRC高位
            uart2_Send( SendBuf, 5 );					
            err = 0;                                               						//发送完数据后清除错误标志
        }
    }
}

/*
函数功能：读保持寄存器  03
主站请求报文:      0x01 0x03   0x0000  0x0001  0x840A     读从0开始的1个保持寄存器
从站正常响应报文:  0x01 0x03   0x02    0x09C4  0xBF87     读到的2字节数据为 0x09C4
*/
void Modbus_03_Slave( void )
{
    u16 RegNum = 0;
    u16 CRC16Temp = 0;
    u8 i = 0;
    RegNum = ( u16 )( data_backup[4] << 8 ) | data_backup[5];        					//获取寄存器数量
    if( ( StartRegAddr + RegNum ) <= (HoldRegStartAddr + HoldRegCount) )      //寄存器地址+寄存器数量 在规定范围内 <=8
    {
        SendBuf[0] = data_backup[0];																					//地址	
        SendBuf[1] = data_backup[1];																					//功能码
        SendBuf[2] = RegNum * 2;																							//返回字节数量
        for( i = 0; i < RegNum; i++ )                             						//循环读取保持寄存器内的值
        {
            SendBuf[3 + i * 2] = HoldReg[StartRegAddr * 2 + i * 2];
            SendBuf[4 + i * 2] = HoldReg[StartRegAddr * 2 + i * 2 + 1];
        }
        CRC16Temp = App_Tab_Get_CRC16( SendBuf, RegNum * 2 + 3 );  						//获取CRC校验值
        SendBuf[RegNum * 2 + 3] = CRC16Temp & 0xFF;                						//CRC低位
        SendBuf[RegNum * 2 + 4] = ( CRC16Temp >> 8 );              						//CRC高位
        uart2_Send( SendBuf, RegNum * 2 + 5 );
    }
    else
    {
        err = 3;                                                   						//寄存器数量不在规定范围内
    }
}
/*
函数功能:写单个保持寄存器 06
主站请求报文:      0x01 0x06    0x0000  0x1388    0x849C   写0号寄存器的值为0x1388
从站正常响应报文:  0x01 0x06    0x0000  0x1388    0x849C    0号寄存器的值为0x1388
*/
void Modbus_06_Slave( void )
{
    u16  RegValue = 0;
    u16 CRC16Temp = 0;
    RegValue = ( u16 )( data_backup[4] << 8 ) | data_backup[5];      					//获取寄存器值
    if( RegValue <= HoldMaxValue )                                          	//寄存器值不超过1000
    {
        HoldReg[StartRegAddr * 2] = data_backup[4];                 					//存储寄存器值
        HoldReg[StartRegAddr * 2 + 1] = data_backup[5];
        SendBuf[0] = data_backup[0];																					//地址
        SendBuf[1] = data_backup[1];																					//功能码
        SendBuf[2] = data_backup[2];																					//地址高位
        SendBuf[3] = data_backup[3];																					//地址低位
        SendBuf[4] = data_backup[4];																					//值高位
        SendBuf[5] = data_backup[5];																					//值低位
        CRC16Temp = App_Tab_Get_CRC16( SendBuf, 6 );              						//获取CRC校验值
        SendBuf[6] = CRC16Temp & 0xFF;                            						//CRC低位
        SendBuf[7] = ( CRC16Temp >> 8 );                          						//CRC高位
        uart2_Send( SendBuf, 8 );
    }
    else
    {
        err =  3;                                                  						//寄存器数值不在规定范围内
    }
}






/*
函数功能:写多个连续保持寄存器值 16
主站请求报文:       0x01 0x10    0x7540  0x0002  0x04  0x0000 0x2710    0xB731  写从0x7540地址开始的2个保持寄存器值 共4字节
从站正常响应报文:   0x01 0x10    0x7540  0x0002  0x5A10                         写从0x7540地址开始的2个保持寄存器值
*/
void Modbus_16_Slave( void )
{
    u16 RegNum = 0;
    u16 CRC16Temp = 0;
    u8 i = 0;
    RegNum = ( u16 )( data_backup[4] << 8 ) | data_backup[5];        					//获取寄存器数量
    if( ( StartRegAddr + RegNum ) <= (HoldRegStartAddr + HoldRegCount) )     	//寄存器地址+寄存器数量 在规定范围内 <=8
    {
        for( i = 0; i < RegNum; i++ )                              						//存储寄存器设置值
        {
            HoldReg[StartRegAddr * 2 + i * 2] = data_backup[i * 2 + 7];
            HoldReg[StartRegAddr * 2 + 1 + i * 2] = data_backup[i * 2 + 8];
        }
        SendBuf[0] = data_backup[0];																					//起始地址
        SendBuf[1] = data_backup[1];																					//功能码
        SendBuf[2] = data_backup[2];																					//地址高位
        SendBuf[3] = data_backup[3];																					//地址低位
        SendBuf[4] = data_backup[4];																					//寄存器数量高位
        SendBuf[5] = data_backup[5];																					//寄存器数量低位
        CRC16Temp = App_Tab_Get_CRC16( SendBuf, 6 );                					//获取CRC校验值
        SendBuf[6] = CRC16Temp & 0xFF;                              					//CRC低位
        SendBuf[7] = ( CRC16Temp >> 8 );                            					//CRC高位
        uart2_Send( SendBuf, 8 );
    }
    else
    {
        err = 3;                                                   						//寄存器数量不在规定范围内
    }
}














