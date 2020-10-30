#ifndef __MODBUS_H
#define __MODBUS_H
#include "sys.h"


#define SlaveID  0x01     //´Ó»úµØÖ·

void DisposeReceive( void );
void Modbus_03_Slave(void);
void Modbus_06_Slave(void);
void Modbus_16_Slave(void);
#endif
