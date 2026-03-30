#include "IOI2C.h"

#define TRUE 1
#define FALSE 0

static uint8_t IIC_Start(void);				
static void IIC_Stop(void);	  		
static void IIC_Send_Byte(uint8_t txd);	
static uint8_t IIC_Read_Byte(void);
static uint8_t IIC_Wait_Ack(void); 	
static void IIC_Ack(void);				
static void IIC_NAck(void);		


static void IIC_delay(void)
{
    uint16_t i=10;//针对72Mhz主频运行速率的延时
   while(i)
   {
     i--;
   }
}


/**************************实现函数********************************************
*函数原型:		void IIC_Start(void)
*功　　能:		产生IIC起始信号
*******************************************************************************/
static uint8_t IIC_Start(void)
{
	SDA_L;
	SCL_L;
	SDA_H;
	SCL_H;

	IIC_delay();
	if(!SDA_read)
		// stm32在通过iic连接vl53l1x时，会出现iic起始信号发送失败的问题，即跳入false分支。而同样程序连接买的vl53l1x不会有这个问题。
		// 如果把vl53l1x拆掉，起码还可以过iic起始发送函数，而焊接vl53l1x后反而不行了
		// 这是因为最开始vl53l1x的btb接口没焊接好，导致这个芯片没有正常上电，直接拉低iic电平导致通信失败
		return FALSE;	
	SDA_L;
	IIC_delay();
	if(SDA_read)
		return FALSE;	
	SDA_L;
	IIC_delay();
	return TRUE;
}

/**************************实现函数********************************************
*函数原型:		void IIC_Stop(void)
*功　　能:	    //产生IIC停止信号
*******************************************************************************/
static void IIC_Stop(void)
{
	SCL_L;
	IIC_delay();
	SDA_L;
	IIC_delay();
	SCL_H;
	IIC_delay();
	SDA_H;
	IIC_delay();
}

/**************************实现函数********************************************
*函数原型:		uint8_t IIC_Wait_Ack(void)
*功　　能:	    等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
*******************************************************************************/
static uint8_t IIC_Wait_Ack(void) 	
{
	SCL_L;
	IIC_delay();
	SDA_H;
	IIC_delay();
	SCL_H;
	IIC_delay();
	// 最开始vl53l1x没有进行bga植球就直接焊接，导致虚焊和通信失败
	if(SDA_read)
	{
    SCL_L;
	  IIC_delay();
      return FALSE;
	}
	SCL_L;
	IIC_delay();
	return TRUE;
}

/**************************实现函数********************************************
*函数原型:		void IIC_Ack(void)
*功　　能:	    产生ACK应答
*******************************************************************************/
static void IIC_Ack(void)
{
	SCL_L;
	IIC_delay();
	SDA_L;
	IIC_delay();
	SCL_H;
	IIC_delay();
	SCL_L;
	IIC_delay();
}

/**************************实现函数********************************************
*函数原型:		void IIC_NAck(void)
*功　　能:	    产生NACK应答
*******************************************************************************/
static void IIC_NAck(void)
{
	SCL_L;
	IIC_delay();
	SDA_H;
	IIC_delay();
	SCL_H;
	IIC_delay();
	SCL_L;
	IIC_delay();
}

/**************************实现函数********************************************
*函数原型:		void IIC_Send_Byte(uint8_t txd)
*功　　能:	    IIC发送一个字节
*******************************************************************************/
static void IIC_Send_Byte(uint8_t SendByte) 
{
    uint8_t i=8;
    while(i--)
    {
			SCL_L;
			IIC_delay();
			if(SendByte&0x80)
				SDA_H;
			else
				SDA_L;
			SendByte<<=1;
			IIC_delay();
			SCL_H;
			IIC_delay();
    }
    SCL_L;
}

/**************************实现函数********************************************
*函数原型:		uint8_t IIC_Read_Byte(unsigned char ack)
*功　　能:	    //读1串字节，ack=1时，发送ACK，ack=0，发送nACK
*******************************************************************************/
static unsigned char IIC_Read_Byte(void)  
{
    uint8_t i=8;
    uint8_t ReceiveByte=0;

    SDA_H;
    while(i--)
    {
			ReceiveByte<<=1;
			SCL_L;
			IIC_delay();
			SCL_H;
			IIC_delay();
			if(SDA_read)
			{
				ReceiveByte|=0x01;
			}
    }
    SCL_L;
    return ReceiveByte;
}

/**************************实现函数********************************************
*函数原型:		unsigned char IIC_ReadOneByte(unsigned char I2C_Addr,unsigned char addr)
*功　　能:	    读取指定设备 指定寄存器的一个值
输入	I2C_Addr  目标设备地址
		addr	   寄存器地址
返回   读出来的值
*******************************************************************************/
uint8_t IIC_ReadOneByte(uint8_t SlaveAddress,uint16_t REG_Address,uint8_t* data)
{
	if(!IIC_Start())
			return FALSE;
    IIC_Send_Byte(SlaveAddress); 
    if(!IIC_Wait_Ack())
		{
			IIC_Stop();
			return FALSE;
		}
    IIC_Send_Byte((uint8_t) REG_Address>>8);   
    IIC_Wait_Ack();
    IIC_Send_Byte((uint8_t) REG_Address & 0x00ff);   
    IIC_Wait_Ack();
    IIC_Start();
    IIC_Send_Byte(SlaveAddress+1);
    IIC_Wait_Ack();

		*data= IIC_Read_Byte();
    IIC_NAck();
    IIC_Stop();
    return TRUE;
}


/**************************实现函数********************************************
*函数原型:		uint8_t IICreadBytes(uint8_t dev, uint8_t reg, uint8_t length, uint8_t *data)
*功　　能:	    读取指定设备 指定寄存器的 length个值
输入	dev  目标设备地址
		reg	  寄存器地址
		length 要读的字节数
		*data  读出的数据将要存放的指针
返回   读出来的字节数量
*******************************************************************************/
uint8_t IICreadBytes(uint8_t SlaveAddress,uint16_t REG_Address,uint8_t len,uint8_t *data)
{
		uint8_t i = 0;
		if(!IIC_Start())
			return FALSE;
    IIC_Send_Byte(SlaveAddress); 
    if(!IIC_Wait_Ack())
		{
			IIC_Stop();
			return FALSE;
		}
    IIC_Send_Byte((uint8_t) REG_Address>>8);   
    IIC_Wait_Ack();
    IIC_Send_Byte(REG_Address&0x00ff);   
    IIC_Wait_Ack();
    IIC_Start();
    IIC_Send_Byte(SlaveAddress+1);
    IIC_Wait_Ack();

		for(i = 0;i<len;i++)
		{
			if(i != (len -1))
			{
				data[i]= IIC_Read_Byte();
				IIC_Ack();
			}
			else
			{
				data[i]= IIC_Read_Byte();
				IIC_NAck();
			}
		}
		IIC_Stop();
		return len;
}

/**************************实现函数********************************************
*函数原型:		uint8_t IICwriteBytes(uint8_t dev, uint8_t reg, uint8_t length, uint8_t* data)
*功　　能:	    将多个字节写入指定设备 指定寄存器
输入	dev  目标设备地址
		reg	  寄存器地址
		length 要写的字节数
		*data  将要写的数据的首地址
返回   返回写入长度
*******************************************************************************/
uint8_t IICwriteBytes(uint8_t dev, uint16_t reg, uint16_t length, uint8_t* data)
{

 	uint8_t count = 0;
	IIC_Start();
	IIC_Send_Byte(dev);	   
	IIC_Wait_Ack();
	IIC_Send_Byte(reg>>8);   
    IIC_Wait_Ack();
	IIC_Send_Byte(reg & 0x00ff);   
    IIC_Wait_Ack();
	for(count=0;count<length;count++)
	{
		IIC_Send_Byte(data[count]);
		IIC_Wait_Ack();
	 }
	IIC_Stop();

    return length; //status == 0;
}


/**************************实现函数********************************************
*函数原型:		unsigned char IICwriteByte(unsigned char dev, unsigned char reg, unsigned char data)
*功　　能:	    写入指定设备 指定寄存器一个字节
输入	dev  目标设备地址
		reg	   寄存器地址
		data  将要写入的字节
返回   1
*******************************************************************************/
uint8_t IICwriteByte(uint8_t dev, uint16_t reg, uint8_t data)
{
    return IICwriteBytes(dev, reg, 1, &data);
}


/**************************实现函数********************************************
*函数原型:		uint8_t IICwriteBit(uint8_t dev, uint8_t reg, uint8_t bitNum, uint8_t data)
*功　　能:	    读 修改 写 指定设备 指定寄存器一个字节 中的1个位
输入	dev  目标设备地址
		reg	   寄存器地址
		bitNum  要修改目标字节的bitNum位
		data  为0 时，目标位将被清0 否则将被置位
返回   成功 为1
 		失败为0
*******************************************************************************/
uint8_t IICwriteBit(uint8_t dev, uint16_t reg, uint8_t bitNum, uint8_t data)
{
    uint8_t b;
    IIC_ReadOneByte(dev, reg, &b);
    b = (data != 0) ? (b | (1 << bitNum)) : (b & ~(1 << bitNum));
    return IICwriteByte(dev, reg, b);
}

//------------------End of File----------------------------
