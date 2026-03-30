#ifndef __CAR_MAP_H
#define __CAR_MAP_H
#include "system.h"
void SetMapValue_ByPixel(int x, int y, uint8_t state);
uint8_t GetMapValue_ByPixel(int x, int y);
void Map_Compress_Process(int meter_pixel);
#endif
