#ifndef __TRAJECTORY_TRACKER_H
#define __TRAJECTORY_TRACKER_H

#include "system.h" // 包含您已有的系统定义

void Tracker_Init(TrajectoryTracker *tracker, PointType *track, uint16_t len, float ld, float speed);
void Tracker_Update(TrajectoryTracker *tracker, VectorType current_pose);

#endif
