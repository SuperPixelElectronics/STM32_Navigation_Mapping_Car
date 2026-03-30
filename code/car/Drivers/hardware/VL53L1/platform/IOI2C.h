#ifndef __IOI2C_H
#define __IOI2C_H
#include "main.h"
//IO口操作宏定义
#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2))
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr))
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum))

#define SCL_H         HAL_GPIO_WritePin(VL53_SCL_GPIO_Port,VL53_SCL_Pin,GPIO_PIN_SET)
#define SCL_L         HAL_GPIO_WritePin(VL53_SCL_GPIO_Port,VL53_SCL_Pin,GPIO_PIN_RESET)

#define SDA_H         HAL_GPIO_WritePin(VL53_SDA_GPIO_Port,VL53_SDA_Pin,GPIO_PIN_SET)
#define SDA_L         HAL_GPIO_WritePin(VL53_SDA_GPIO_Port,VL53_SDA_Pin,GPIO_PIN_RESET)

#define SCL_read      HAL_GPIO_ReadPin(VL53_SCL_GPIO_Port,VL53_SCL_Pin)
#define SDA_read      HAL_GPIO_ReadPin(VL53_SDA_GPIO_Port,VL53_SDA_Pin)

extern void IIC_Init(void);                //初始化IIC的IO口


extern uint8_t IIC_ReadOneByte(uint8_t SlaveAddress,uint16_t REG_Address,uint8_t* data);
extern unsigned char IICwriteByte(unsigned char dev, uint16_t reg, unsigned char data);
extern uint8_t IICwriteBytes(uint8_t dev, uint16_t reg, uint16_t length, uint8_t* data);
extern uint8_t IICwriteBit(uint8_t dev,uint16_t reg,uint8_t bitNum,uint8_t data);
extern uint8_t IICreadBytes(uint8_t SlaveAddress,uint16_t REG_Address,uint8_t len,uint8_t *data);


#endif

//------------------End of File----------------------------
