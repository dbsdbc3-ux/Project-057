// Sensor_Recording.c
#include "Sensor_Recording.h"
#include "Sensor_Convert.h"
#include "CAT24M01.h"
#include "Menu.h"
#include "GY_95T.h"
#include "AD.h"
#include <string.h>

// ==================== 录制状态变量 ====================
static uint8_t is_recording = 0;           // 录制状态标志
static uint8_t current_segment = 0;        // 当前录制片段
static uint16_t frame_count = 0;           // 已录制帧数
static uint32_t start_timestamp = 0;       // 录制开始时间

// 方案一：添加帧保存标志
static uint8_t need_save_frame = 0;        // 需要保存帧的标志
static uint8_t last_save_result = 0;       // 上次保存的结果
static uint8_t consecutive_save_errors = 0; // 连续保存错误计数

// ==================== 录制开始函数 ====================
/**
  * @brief  开始录制指定片段
  * @param  segment_id: 片段ID (0-3)
  * @retval 数字直观反馈:
  *         0 - 成功
  *         1 - 已在录制
  *         2 - 参数错误 (segment_id>3)
  *         3 - 清除失败
  *         4 - 检测失败
  *         5 - 片段头写入失败
  */
uint8_t Record_Start(uint8_t segment_id)
{
    // 1. 参数检查
    if (segment_id > 3) return 2;  // 参数错误
    
    // 2. 状态检查
    if (is_recording) return 1;  // 已在录制
    
    // 3. 检查并准备片段
    uint8_t has_data = CheckSegmentHasData(segment_id);
    
    if (has_data == 1) {
        // 片段有数据，先清除
        if (ClearSegmentHeader(segment_id) != 0) {
            return 3;  // 清除失败
        }
    } else if (has_data == 255) {
        return 4;  // 检测失败
    }
    
    // 4. 初始化片段头
    SegmentHeader_t header;
    header.start_timestamp = systemTickCount;
    header.frame_count = 0;
    header.segment_id = segment_id;
    header.key_frame = 0;  // 修改：初始设置为0，录制结束时再调整
    for (uint8_t i = 0; i < 7; i++) {
        header.reserved[i] = 0xFF;
    }
    
    uint32_t header_addr = 0x10000 + (segment_id * 0x4000);
    
    if (CAT24M01_WriteBuffer(header_addr, (uint8_t*)&header, 16) != 0) {
        return 5;  // 片段头写入失败
    }
    
    // 5. 设置录制状态
    is_recording = 1;
    current_segment = segment_id;
    frame_count = 0;
    start_timestamp = systemTickCount;
    
    // 重置保存标志和错误计数
    need_save_frame = 0;
    last_save_result = 0;
    consecutive_save_errors = 0;
    
    return 0;  // 成功
}

// ==================== 录制保存函数 ====================
/**
  * @brief  保存当前帧（主循环调用）
  * @param  无
  * @retval 数字直观反馈:
  *         0 - 成功
  *         1 - 未在录制
  *         2 - 片段已满
  *         3 - 帧写入失败
  */
uint8_t Record_SaveFrame(void)
{
    if (!is_recording) {
        last_save_result = 1;  // 未在录制
        return 1;
    }
    
    // 检查容量
    if (frame_count >= 960) {
        Record_Stop();  // 自动停止录制
        last_save_result = 2;  // 片段已满
        return 2;
    }
    
    // 准备数据
    OptData_t frame_data;
    frame_data.frame_seq = frame_count;
    Sensor_ConvertToOptimized(&frame_data);
    
    // 计算地址
    uint32_t addr = 0x10000;
    addr += current_segment * 0x4000;  // 片段
    addr += 16;                        // 跳过片段头
    addr += frame_count * 17;          // 帧偏移
    
    // 保存到EEPROM
    if (CAT24M01_WriteBuffer(addr, (uint8_t*)&frame_data, 17) != 0) {
        last_save_result = 3;  // 帧写入失败
        consecutive_save_errors++;
        
        // 连续错误过多，自动停止录制
        if (consecutive_save_errors >= 5) {
            Record_Stop();  // 自动停止
        }
        
        return 3;
    }
    
    // 保存成功
    frame_count++;
    last_save_result = 0;               // 成功
    consecutive_save_errors = 0;        // 重置错误计数
    
    return 0;  // 成功
}

// ==================== 录制停止函数 ====================
/**
  * @brief  停止录制
  * @param  无
  * @retval 数字直观反馈:
  *         0 - 成功
  *         1 - 未在录制
  *         2 - 片段头读取失败
  *         3 - 片段头写入失败
  */
uint8_t Record_Stop(void)
{
if (!is_recording) return 1;  // 未在录制
    
    // 更新片段头
    SegmentHeader_t header;
    uint32_t header_addr = 0x10000 + (current_segment * 0x4000);
    
    // 读取当前片段头
    if (CAT24M01_ReadBuffer(header_addr, (uint8_t*)&header, 16) != 0) {
        return 2;  // 片段头读取失败
    }
    
    // 更新帧数
    header.frame_count = frame_count;
    
    // ============ 新增：动态设置关键帧 ============
    if (frame_count > 84) {
        // 录制超过84帧，设置关键帧为44
        // 确保关键帧不超过总帧数-1
        if (44 < frame_count) {
            header.key_frame = 44;
        } else {
            // 如果总帧数不足44帧，设置为frame_count-1
            header.key_frame = frame_count - 1;
        }
    } else {
        // 录制84帧或更少，设置关键帧为0
        header.key_frame = 0;
    } 
    // 写回片段头
    if (CAT24M01_WriteBuffer(header_addr, (uint8_t*)&header, 16) != 0) {
        return 3;  // 片段头写入失败
    }
    
    // 重置状态
    is_recording = 0;
    current_segment = 0;
    frame_count = 0;
    start_timestamp = 0;
    need_save_frame = 0;  // 清除保存标志
    last_save_result = 0;
    consecutive_save_errors = 0;
    
    return 0;  // 成功
}

// ==================== 方案一：帧保存标志管理函数 ====================
/**
  * @brief  检查是否需要保存帧
  * @param  无
  * @retval 1-需要保存, 0-不需要保存
  */
uint8_t Record_CheckNeedSave(void)
{
    return need_save_frame;
}

/**
  * @brief  设置需要保存帧的标志
  * @param  flag: 1-需要保存, 0-不需要保存
  * @retval 无
  */
void Record_SetNeedSaveFlag(uint8_t flag)
{
    need_save_frame = flag;
}

/**
  * @brief  清除保存标志
  * @param  无
  * @retval 无
  */
void Record_ClearSaveFlag(void)
{
    need_save_frame = 0;
}

/**
  * @brief  获取上次保存结果
  * @param  无
  * @retval 上次保存的错误码
  */
uint8_t Record_GetLastSaveResult(void)
{
    return last_save_result;
}

// ==================== 状态查询函数 ====================
/**
  * @brief  检查是否在录制中
  * @param  无
  * @retval 1-录制中, 0-未录制
  */
uint8_t Record_IsActive(void)
{
    return is_recording;
}

/**
  * @brief  获取当前录制片段
  * @param  无
  * @retval 片段ID, 255-未在录制
  */
uint8_t Record_GetSegment(void)
{
    if (!is_recording) return 255;
    return current_segment;
}

/**
  * @brief  获取已录制帧数
  * @param  无
  * @retval 帧数
  */
uint16_t Record_GetFrameCount(void)
{
    return frame_count;
}

/**
  * @brief  获取录制开始时间
  * @param  无
  * @retval 录制开始的时间戳(ms)
  */
uint32_t Record_GetStartTime(void)
{
    return start_timestamp;
}

/**
  * @brief  获取录制时长
  * @param  无
  * @retval 录制时长(ms)
  */
uint32_t Record_GetDuration(void)
{
    if (!is_recording) return 0;
    return systemTickCount - start_timestamp;
}

/**
  * @brief  获取剩余帧容量
  * @param  无
  * @retval 剩余可录制帧数
  */
uint16_t Record_GetRemainingFrames(void)
{
    if (!is_recording) return 0;
    
    if (frame_count >= 960) {
        return 0;  // 已满
    }
    
    return 960 - frame_count;
}

/**
  * @brief  获取连续保存错误计数
  * @param  无
  * @retval 连续保存错误次数
  */
uint8_t Record_GetConsecutiveErrors(void)
{
    return consecutive_save_errors;
}

/**
  * @brief  修改关键帧序号
  * @param  segment_id: 片段ID
  * @param  new_key_frame: 新的关键帧序号
  * @retval 0-成功，1-片段号无效，2-读取失败，3-写入失败
  */
uint8_t ModifyKeyFrame(uint8_t segment_id, uint16_t new_key_frame)
{
    if (segment_id > 3) return 1;  // 片段号无效
    
    // 1. 读取当前片段头
    SegmentHeader_t header;
    uint32_t header_addr = 0x10000 + (segment_id * 0x4000);
    
    if (CAT24M01_ReadBuffer(header_addr, (uint8_t*)&header, 16) != 0) {
        return 2;  // 读取失败
    }
    
    // 2. 检查新关键帧是否有效
    if (header.frame_count > 0) {
        // 确保关键帧不超过总帧数-1
        if (new_key_frame >= header.frame_count) {
            new_key_frame = header.frame_count - 1;
        }
    } else {
        // 没有数据，关键帧设为0
        new_key_frame = 0;
    }
    
    // 3. 更新关键帧
    header.key_frame = new_key_frame;
    
    // 4. 写回EEPROM
    if (CAT24M01_WriteBuffer(header_addr, (uint8_t*)&header, 16) != 0) {
        return 3;  // 写入失败
    }
    
    return 0;  // 成功
}

/**
  * @brief  读取指定片段的关键帧序号
  * @param  segment_id: 片段ID (0-3)
  * @param  key_frame_ptr: 指向关键帧值的指针
  * @retval 0-成功，1-片段号无效，2-读取失败
  * 说    明：不返回默认值，只返回读取结果
  */
uint8_t ReadKeyFrame(uint8_t segment_id, uint16_t* key_frame_ptr)
{
    if (segment_id > 3) return 1;  // 片段号无效
    if (key_frame_ptr == NULL) return 1;  // 指针无效
    
    // 计算关键帧地址
    uint32_t key_frame_addr = 0x10000 + (segment_id * 0x4000) + 7;
    uint8_t key_frame_bytes[2];
    
    // 读取2字节的关键帧序号
    if (CAT24M01_ReadBuffer(key_frame_addr, key_frame_bytes, 2) != 0) {
        return 2;  // 读取失败
    }
    
    // 小端格式转换为16位整数
    *key_frame_ptr = (key_frame_bytes[1] << 8) | key_frame_bytes[0];
    
    return 0;  // 成功
}
