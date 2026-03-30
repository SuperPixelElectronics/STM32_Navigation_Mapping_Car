#ifndef __CAR_MAP_H
#define __CAR_MAP_H
#include "system.h"

// 地图读写操作 像素级读取
void SetMapValue_ByPixel(int x, int y, uint8_t state);
uint8_t GetMapValue_ByPixel(int x, int y);

// 地图读写操作 真实物理坐标
void SetMapValue(float x, float y, uint8_t state);
uint8_t GetMapValue(float x, float y);
void UpdateMapValue(float x, float y, int8_t delta);

// 射线追踪建图任务
void Update_Map(void);

// 图像处理
void erode_map(void);
void dilate_map(void);
void Map_Deep_Boundary_Clear(void);
void Map_Generate_Boundary_Contour(void);
void Map_Global_Opening_InPlace(void);

// 图像压缩和测试图像的生成
void Map_Compress_Process(int meter_pixel);
void Map_Generate_Test_C_Shape_Advanced(void);
#endif
