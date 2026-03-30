#ifndef __nRF24L01_API_H
#define __nRF24L01_API_H

#include "nRF24L01.h"
#include "system.h"
#define uchar unsigned char
#define uint unsigned int
void delay_1us(uchar num);
void delay_150us(void);
uchar SPI_RW(uchar byte);
uchar NRF24L01_Write_Reg(uchar reg, uchar value);
uchar NRF24L01_Read_Reg(uchar reg);
uchar NRF24L01_Read_Buf(uchar reg, uchar *pBuf, uchar len);
uchar NRF24L01_Write_Buf(uchar reg, uchar *pBuf, uchar len);
uchar NRF24L01_RxPacket(uchar *rxbuf);
uchar NRF24L01_TxPacket(uchar *txbuf);
uchar NRF24L01_Check(void);
void NRF24L01_RT_Init(void);
void NRF24L01_RT_receive_ready(void);
void NRF_TX_Mode(void);
void NRF_RX_Mode(void);
void SEND_BUF(uchar *buf);
HAL_StatusTypeDef NRF24L01_Init(void);

// TX Request
void nrf_send_info_request(void);
void nrf_send_map_request(void);
void nrf_send_track_request(void);

// TX controler Command
void nrf_send_solve_command(void);
void nrf_send_image_process_command(void);

// main
void nrf_receive_process_irq(void);
void nrf_receive_process_main(void);

#endif

