// Sensor_Recording.h
#ifndef __SENSOR_RECORDING_H
#define __SENSOR_RECORDING_H

#include "stm32f10x.h"

// 函数声明
uint8_t Record_Start(uint8_t segment_id);
uint8_t Record_Stop(void);
uint8_t Record_SaveFrame(void);
uint8_t Record_IsActive(void);
uint8_t Record_GetSegment(void);
uint16_t Record_GetFrameCount(void);
uint32_t Record_GetStartTime(void);
uint32_t Record_GetDuration(void);
uint16_t Record_GetRemainingFrames(void);
uint8_t Record_GetConsecutiveErrors(void);

// 方案一：帧保存标志管理函数
uint8_t Record_CheckNeedSave(void);
void Record_SetNeedSaveFlag(uint8_t flag);
void Record_ClearSaveFlag(void);
uint8_t Record_GetLastSaveResult(void);
uint8_t ModifyKeyFrame(uint8_t segment_id, uint16_t new_key_frame);
uint8_t ReadKeyFrame(uint8_t segment_id, uint16_t* key_frame_ptr);

#endif /* __SENSOR_RECORDING_H */
