// Sensor_Convert.h
#ifndef __SENSOR_CONVERT_H
#define __SENSOR_CONVERT_H

#include "stm32f10x.h"
#include "string.h"
#include "CAT24M01.h"

// ==================== 统一结构体定义 ====================
// 1. 优化数据帧结构体（17字节）
typedef struct __attribute__((packed)) {
    uint16_t frame_seq;     // 帧序号
    uint8_t  adc[5];        // 8位ADC值
    int16_t  euler[3];      // 欧拉角（0.1°精度）
    int16_t  quaternion[4]; // 四元数（0.001精度）
} OptData_t;

// 2. 片段头结构体（16字节）
typedef struct __attribute__((packed)) {
    uint32_t start_timestamp;  // 起始时间戳
    uint16_t frame_count;      // 总帧数
    uint8_t  segment_id;       // 片段ID
    uint16_t key_frame;        // 关键帧号（新增）
    uint8_t  reserved[7];      // 预留字段
} SegmentHeader_t;
// ==================== 统一结构体结束 ====================

// 函数声明
void Sensor_ConvertToOptimized(OptData_t* out);
uint8_t ADC_12to8(uint16_t adc12);
int16_t FloatToEuler_0_1(float angle);
int16_t FloatToQuat_0_001(float q);
uint8_t ClearSegmentHeader(uint8_t segment_id);
uint8_t CheckSegmentHasData(uint8_t Collection);
uint8_t ModifyKeyFrame(uint8_t segment_id, uint16_t new_key_frame);

#endif /* __SENSOR_CONVERT_H */
