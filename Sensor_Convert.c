#include "Sensor_Convert.h"
#include "GY_95T.h"
#include "AD.h"
#include <string.h>  // 确保这行存在
#include <stddef.h>
#include <math.h>

/**
  * @brief  ADC精度转换：12位转8位
  * @param  adc12: 12位ADC值
  * @retval 8位ADC值
  */
uint8_t ADC_12to8(uint16_t adc12) {
    return (uint8_t)(adc12 >> 4);  // 右移4位
}

/**
  * @brief  欧拉角转换：浮点数转0.1°精度
  * @param  angle: 原始角度（浮点数）
  * @retval 0.1°精度整数值
  */
int16_t FloatToEuler_0_1(float angle) {
    // 乘以10得到0.1°精度，四舍五入
    int16_t val = (int16_t)(angle * 10.0f + 0.5f);
    
    // 限制范围
    if (val < -1800) val = -1800;  // -180.0°
    if (val > 1799) val = 1799;    // +179.9°
    
    return val;
}

/**
  * @brief  四元数转换：浮点数转0.001精度
  * @param  q: 原始四元数分量
  * @retval 0.001精度整数值
  */
int16_t FloatToQuat_0_001(float q) {
    // 限制范围
    if (q < -1.0f) q = -1.0f;
    if (q > 1.0f) q = 1.0f;
    
    // 乘以1000得到0.001精度，四舍五入
    int16_t val = (int16_t)(q * 1000.0f + 0.5f);
    
    // 限制范围
    if (val < -1000) val = -1000;  // -1.000
    if (val > 999) val = 999;      // +0.999
    
    return val;
}

/**
  * @brief  转换为优化精度（用于存储）
  * @param  out: 输出优化数据结构
  * @retval 无
  */
void Sensor_ConvertToOptimized(OptData_t* out) {
    if (out == NULL) return;
    
    // 1. ADC转换
    for (int i = 0; i < 5; i++) {
        out->adc[i] = ADC_12to8(AD_Value[i]);
    }
    
    // 2. 欧拉角转换（需要外部提供sensor_data）
    extern GY95T_Data_t sensor_data;  // 从Menu.h中引用
    
    out->euler[0] = FloatToEuler_0_1(sensor_data.yaw);
    out->euler[1] = FloatToEuler_0_1(sensor_data.roll);
    out->euler[2] = FloatToEuler_0_1(sensor_data.pitch);
    
    // 3. 四元数转换
    out->quaternion[0] = FloatToQuat_0_001(sensor_data.q0);
    out->quaternion[1] = FloatToQuat_0_001(sensor_data.q1);
    out->quaternion[2] = FloatToQuat_0_001(sensor_data.q2);
    out->quaternion[3] = FloatToQuat_0_001(sensor_data.q3);
    
    // 注：frame_seq由调用者设置
}

/**
  * @brief  清除指定片段的片段头（快速清理）
  * @param  segment_id: 片段ID (0-3 对应 DATA0-DATA3)
  * @retval 0-成功，1-失败
  */
uint8_t ClearSegmentHeader(uint8_t segment_id) {
    if (segment_id > 3) return 1;
    
    // 创建空的片段头
    SegmentHeader_t empty_header;
    empty_header.start_timestamp = 0xFFFFFFFF;
    empty_header.frame_count = 0;
    empty_header.segment_id = segment_id;
    empty_header.key_frame = 0;  // 新增：清除关键帧
    
    // 手动初始化
    for (uint8_t i = 0; i < 7; i++) {  // 修改：从9改为7
        empty_header.reserved[i] = 0xFF;
    }
    
    // 计算地址
    uint32_t header_addr = 0x10000 + (segment_id * 0x4000);
    
    // 使用WriteBuffer写入
    if (CAT24M01_WriteBuffer(header_addr, (uint8_t*)&empty_header, 16) != 0) {
        return 1;  // 写入失败
    }
    
    CAT24M01_WaitForWriteComplete();
    return 0;
}

/**
  * @brief  检测指定片段是否有数据
  * @param  Collection: 片段编号 (0-3 对应 DATA0-DATA3)
  * @retval 检测结果: 1-有数据, 0-无数据, 255-检测失败
  */
uint8_t CheckSegmentHasData(uint8_t Collection)
{
    // 参数验证
    if (Collection > 3) return 255;
    
    // 计算片段头地址
    uint32_t header_addr = 0x10000 + (Collection * 0x4000);
    
    // 准备读取frame_count字段（偏移4-5字节）
    uint8_t frame_count_bytes[2];
    
    // 使用ReadBuffer连续读取2字节
    if (CAT24M01_ReadBuffer(header_addr + 4, frame_count_bytes, 2) != 0) {
        return 255;  // 读取失败
    }
    
    // 组合为16位帧数
    uint16_t frame_count = (frame_count_bytes[1] << 8) | frame_count_bytes[0];
    
    // 判断是否有数据
    if (frame_count > 0) {
        return 1;  // 有数据
    } else {
        return 0;  // 无数据
    }
}
