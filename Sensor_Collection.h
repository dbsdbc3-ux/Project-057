#ifndef __SENSOR_COLLECTION_H
#define __SENSOR_COLLECTION_H


// 修改函数声明
void Sensor_Data_Playback(uint8_t Sensor, uint8_t Data_name, uint8_t segment_id, uint16_t current_key_frame);
void Sensor_Data_Collection(uint8_t option_names,uint8_t Collection);
void Example_Direct_Access(void);
void ADC_Data(uint8_t Adc_Value);
void EulerAngle_Data(uint8_t Data_name);
void Quaternion_Data(uint8_t Data_name);

#endif
