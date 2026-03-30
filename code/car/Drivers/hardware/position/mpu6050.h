#ifndef __MPU6050_H
#define __MPU6050_H
#include "system.h"

#define MPU6050_AXL 2
#define MPU6050_AXH 3
#define MPU6050_AYL 4
#define MPU6050_AYH 5
#define MPU6050_AZL 6
#define MPU6050_AZH 7

#define MPU6050_TL 8
#define MPU6050_TH 9

#define MPU6050_WXL 13
#define MPU6050_WXH 14
#define MPU6050_WYL 15
#define MPU6050_WYH 16
#define MPU6050_WZL 17
#define MPU6050_WZH 18

#define MPU6050_ROLLL 24
#define MPU6050_ROLLH 25
#define MPU6050_PITCHL 26
#define MPU6050_PITCHH 27
#define MPU6050_YAWL 28
#define MPU6050_YAWH 29

#define MPU6050_HXL 35
#define MPU6050_HXH 36
#define MPU6050_HYL 37
#define MPU6050_HYH 38
#define MPU6050_HZL 39
#define MPU6050_HZH 40

#define MPU6050_G 9.80665f

void MPU_UART_Receive(void);
void mpu6050_init_all(void);
void mpu6050_process(void);
#endif
