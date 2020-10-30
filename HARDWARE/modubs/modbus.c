#include "modbus.h"
#include "crc16.h"
#include "uart2.h"

#define HoldRegStartAddr    0x0000																						//���ּĴ�����ʼ��ַ
#define HoldRegCount        9																									//���ּĴ�������
#define HoldMaxValue        1000            																	//�Ĵ������ֵ

extern u8 data_backup[DMA_REC_LEN]; 													  							//modbus���յ����ݱ���
extern u16 dataLen_backup;																										//modbus�������ݳ��ȱ���


u8 SendBuf[DMA_REC_LEN] = {0};
u16 StartRegAddr = HoldRegStartAddr;
u8 err = 0;                                                        						//�������

u8 HoldReg[HoldRegCount*2] = {0, 1, 0, 2, 0, 3, 0, 4,0, 5, 0, 6, 0, 7, 0, 8,0,9}; //�洢8·����ֵ�������޸�


/*
�������˵��:
0x01  ����֧�ֵĹ�����
0x02  ��ʼ��ַ���ڹ涨��Χ��
0x03  �Ĵ����������ڹ涨��Χ��
0x04  ����У�����
*/
//������յ�������
// ����: [��ַ][������][��ʼ��ַ��][��ʼ��ַ��][�ܼĴ�������][�ܼĴ�������][CRC��][CRC��]
void DisposeReceive( void )
{
    u16 CRC16 = 0, CRC16Temp = 0;
    if( data_backup[0] == SlaveID )                                 					//��ַ���ڱ�����ַ ��ַ��Χ:1 - 32
    {
        CRC16 = App_Tab_Get_CRC16( data_backup, dataLen_backup - 2 );  				//CRCУ�� ���ֽ���ǰ ���ֽ��ں� ���ֽ�Ϊ�������һ���ֽ�
        CRC16Temp = ( ( u16 )( data_backup[dataLen_backup - 1] << 8 ) | data_backup[dataLen_backup - 2] );
        if( CRC16 != CRC16Temp )
        {
            err = 4;                                               						//CRCУ�����
        }
        StartRegAddr = ( u16 )( data_backup[2] << 8 ) | data_backup[3];
        if( StartRegAddr > (HoldRegStartAddr + HoldRegCount - 1) ) 
        {
            err = 2;                                               						//��ʼ��ַ���ڹ涨��Χ�� 00 - 07    1 - 8��ͨ��
        }
        if( err == 0 )
        {
            switch( data_backup[1] )                                					//������
            {
                case 3:                                            						//������Ĵ���
                {
                    Modbus_03_Slave();
                    break;
                }
                case 6:                                            						//д�����Ĵ���
                {
                    Modbus_06_Slave();
                    break;
                }
                case 16:                                           						//д����Ĵ���
                {
                    Modbus_16_Slave();
                    break;
                }
                default:
                {
                    err = 1;                                       						//��֧�ָù�����
                    break;
                }
            }
        }
        if( err > 0 )
        {
            SendBuf[0] = data_backup[0];
            SendBuf[1] = data_backup[1] | 0x80;
            SendBuf[2] = err;                                      						//���ʹ������
            CRC16Temp = App_Tab_Get_CRC16( SendBuf, 3 );           						//����CRCУ��ֵ
            SendBuf[3] = CRC16Temp & 0xFF;                         						//CRC��λ
            SendBuf[4] = ( CRC16Temp >> 8 );                       						//CRC��λ
            uart2_Send( SendBuf, 5 );					
            err = 0;                                               						//���������ݺ���������־
        }
    }
}

/*
�������ܣ������ּĴ���  03
��վ������:      0x01 0x03   0x0000  0x0001  0x840A     ����0��ʼ��1�����ּĴ���
��վ������Ӧ����:  0x01 0x03   0x02    0x09C4  0xBF87     ������2�ֽ�����Ϊ 0x09C4
*/
void Modbus_03_Slave( void )
{
    u16 RegNum = 0;
    u16 CRC16Temp = 0;
    u8 i = 0;
    RegNum = ( u16 )( data_backup[4] << 8 ) | data_backup[5];        					//��ȡ�Ĵ�������
    if( ( StartRegAddr + RegNum ) <= (HoldRegStartAddr + HoldRegCount) )      //�Ĵ�����ַ+�Ĵ������� �ڹ涨��Χ�� <=8
    {
        SendBuf[0] = data_backup[0];																					//��ַ	
        SendBuf[1] = data_backup[1];																					//������
        SendBuf[2] = RegNum * 2;																							//�����ֽ�����
        for( i = 0; i < RegNum; i++ )                             						//ѭ����ȡ���ּĴ����ڵ�ֵ
        {
            SendBuf[3 + i * 2] = HoldReg[StartRegAddr * 2 + i * 2];
            SendBuf[4 + i * 2] = HoldReg[StartRegAddr * 2 + i * 2 + 1];
        }
        CRC16Temp = App_Tab_Get_CRC16( SendBuf, RegNum * 2 + 3 );  						//��ȡCRCУ��ֵ
        SendBuf[RegNum * 2 + 3] = CRC16Temp & 0xFF;                						//CRC��λ
        SendBuf[RegNum * 2 + 4] = ( CRC16Temp >> 8 );              						//CRC��λ
        uart2_Send( SendBuf, RegNum * 2 + 5 );
    }
    else
    {
        err = 3;                                                   						//�Ĵ����������ڹ涨��Χ��
    }
}
/*
��������:д�������ּĴ��� 06
��վ������:      0x01 0x06    0x0000  0x1388    0x849C   д0�żĴ�����ֵΪ0x1388
��վ������Ӧ����:  0x01 0x06    0x0000  0x1388    0x849C    0�żĴ�����ֵΪ0x1388
*/
void Modbus_06_Slave( void )
{
    u16  RegValue = 0;
    u16 CRC16Temp = 0;
    RegValue = ( u16 )( data_backup[4] << 8 ) | data_backup[5];      					//��ȡ�Ĵ���ֵ
    if( RegValue <= HoldMaxValue )                                          	//�Ĵ���ֵ������1000
    {
        HoldReg[StartRegAddr * 2] = data_backup[4];                 					//�洢�Ĵ���ֵ
        HoldReg[StartRegAddr * 2 + 1] = data_backup[5];
        SendBuf[0] = data_backup[0];																					//��ַ
        SendBuf[1] = data_backup[1];																					//������
        SendBuf[2] = data_backup[2];																					//��ַ��λ
        SendBuf[3] = data_backup[3];																					//��ַ��λ
        SendBuf[4] = data_backup[4];																					//ֵ��λ
        SendBuf[5] = data_backup[5];																					//ֵ��λ
        CRC16Temp = App_Tab_Get_CRC16( SendBuf, 6 );              						//��ȡCRCУ��ֵ
        SendBuf[6] = CRC16Temp & 0xFF;                            						//CRC��λ
        SendBuf[7] = ( CRC16Temp >> 8 );                          						//CRC��λ
        uart2_Send( SendBuf, 8 );
    }
    else
    {
        err =  3;                                                  						//�Ĵ�����ֵ���ڹ涨��Χ��
    }
}






/*
��������:д����������ּĴ���ֵ 16
��վ������:       0x01 0x10    0x7540  0x0002  0x04  0x0000 0x2710    0xB731  д��0x7540��ַ��ʼ��2�����ּĴ���ֵ ��4�ֽ�
��վ������Ӧ����:   0x01 0x10    0x7540  0x0002  0x5A10                         д��0x7540��ַ��ʼ��2�����ּĴ���ֵ
*/
void Modbus_16_Slave( void )
{
    u16 RegNum = 0;
    u16 CRC16Temp = 0;
    u8 i = 0;
    RegNum = ( u16 )( data_backup[4] << 8 ) | data_backup[5];        					//��ȡ�Ĵ�������
    if( ( StartRegAddr + RegNum ) <= (HoldRegStartAddr + HoldRegCount) )     	//�Ĵ�����ַ+�Ĵ������� �ڹ涨��Χ�� <=8
    {
        for( i = 0; i < RegNum; i++ )                              						//�洢�Ĵ�������ֵ
        {
            HoldReg[StartRegAddr * 2 + i * 2] = data_backup[i * 2 + 7];
            HoldReg[StartRegAddr * 2 + 1 + i * 2] = data_backup[i * 2 + 8];
        }
        SendBuf[0] = data_backup[0];																					//��ʼ��ַ
        SendBuf[1] = data_backup[1];																					//������
        SendBuf[2] = data_backup[2];																					//��ַ��λ
        SendBuf[3] = data_backup[3];																					//��ַ��λ
        SendBuf[4] = data_backup[4];																					//�Ĵ���������λ
        SendBuf[5] = data_backup[5];																					//�Ĵ���������λ
        CRC16Temp = App_Tab_Get_CRC16( SendBuf, 6 );                					//��ȡCRCУ��ֵ
        SendBuf[6] = CRC16Temp & 0xFF;                              					//CRC��λ
        SendBuf[7] = ( CRC16Temp >> 8 );                            					//CRC��λ
        uart2_Send( SendBuf, 8 );
    }
    else
    {
        err = 3;                                                   						//�Ĵ����������ڹ涨��Χ��
    }
}














