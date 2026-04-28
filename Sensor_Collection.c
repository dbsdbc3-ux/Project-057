#include "stm32f10x.h" 
#include "OLED.h"
#include "Program_Editor.h"
#include "AD.h"
#include "Sensor_Collection.h"
#include "Sensor_Convert.h"
#include "GY_95T.h"
#include "stdio.h"
const char *Collection_names[] = {"DATA0", "DATA1", "DATA2", "DATA3", "返回"};
const char *Collection_option_names[] = { "此地址无数据", "清除", "录制", "回放", "返回", "开始", "停止", "清除", "退出"};
/**
	* 函	数： 采集菜单显示
	* 参	数： Collection 选择显示Collection_names[]的元素
	* 返 回 值： 无
	* 说	明： 无
	*/
void Sensor_Data_Collection(uint8_t option_names,uint8_t Collection)
{

	Edit_Menu_Frame(Collection,4);
	if(option_names > 0){
		OLED_ShowImage(2, 31, 124, 22, Condition_box_2);
	}
	if(Collection==4){
		OLED_ShowString(48, 34, (char *)Collection_names[Collection], OLED_8X16);
	}
	else{
		OLED_ShowString(44, 34, (char *)Collection_names[Collection], OLED_8X16);
	}
	if(option_names == 0){
	if(Collection == 4){
		OLED_ShowString(16, 2, "  退出采集  ", OLED_8X16);
		}
    else{        
        // 检测当前片段是否有数据
        uint8_t has_data = CheckSegmentHasData(Collection);
        
        if (has_data == 0) {
            // 该片段无数据
            OLED_ShowString(16, 2, "此地址无数据", OLED_8X16);
        } 
        else if (has_data == 1) {
            // 该片段有数据
            OLED_ShowString(16, 2, "选中管理数据", OLED_8X16);
        } 
        else {
            // 检测失败
            OLED_ShowString(16, 2, "检测数据失败", OLED_8X16);
        }
    }
    OLED_Update();
    return;
	}
	if(option_names > 4){
		if(option_names < 7){
		OLED_ShowString(40, 2, ">", OLED_8X16);
		OLED_ShowString(80, 2, "<", OLED_8X16);
		}			
		OLED_ShowString(48, 2,(char *)Collection_option_names[option_names], OLED_8X16);
		if(option_names == 6){
			OLED_ShowString(40, 34, "录制中", OLED_8X16);
		}
		else if(option_names == 7){
			OLED_ShowString(40, 34, "清除中", OLED_8X16);
		}
		else if(option_names == 8){
			OLED_ShowString(32, 34, "录制已满", OLED_8X16);
		}
		OLED_Update();
        return;
	}
    // 显示前一个选项（option_num-1）
    if (option_names - 1 >= 1) { // 确保索引>=1
        OLED_ShowString(4, 2, (char *)Collection_option_names[option_names - 1], OLED_8X16);
    } else {
        OLED_ShowString(4, 2, "  ", OLED_8X16); // 显示两个空格
    }	
	
    OLED_ShowString(40, 2, ">", OLED_8X16);
    OLED_ShowString(48, 2, (char *)Collection_option_names[option_names], OLED_8X16);
    OLED_ShowString(80, 2, "<", OLED_8X16);
	
	    // 显示后一个选项（option_num+1）
    if (option_names + 1 < 5) {
        OLED_ShowString(92, 2, (char *)Collection_option_names[option_names + 1], OLED_8X16);
    } else {
        OLED_ShowString(92, 2, "  ", OLED_8X16); // 显示两个空格
    }
	OLED_Update();	
}


const char *Collection_Option_names[] = {"手势", "偏移", "旋转", "返回"};
const char *ADC_Selection[] = {"ADC  ","A1 ", "A2 ", "A3 ", "A4 ", "A5 "};
const char *EulerAngle_Selection[] = {"Euler","Yaw  ", "Roll ", "Pitch"};
const char *Quaternion_Selection[] = {"Quat ","Q0  ", "Q1  ", "Q2  ", "Q3  "};
/**
	* 函	数： 回放曲线显示
	* 参	数： Sensor 要显示的数据类型(Collection_Option_names[])，Data_name 要显示那些数据
	* 返 回 值： 无
	* 说	明： 无
	*/
void Sensor_Data_Playback(uint8_t Sensor, uint8_t Data_name, uint8_t segment_id, uint16_t current_key_frame)			
{
	OLED_ShowImage(0, 0, 128, 20, Prompt_box);
	if(Sensor == 0){
		OLED_ShowString(4, 2, "  ",OLED_8X16);	
	}
	else{
		OLED_ShowString(4, 2, (char *)Collection_Option_names[Sensor - 1], OLED_8X16);
	}
		OLED_ShowString(48, 2, (char *)Collection_Option_names[Sensor], OLED_8X16);
	if(Sensor == 3){
		OLED_ShowString(92, 2, "  ", OLED_8X16);	
	}
	else {
		OLED_ShowString(92, 2, (char *)Collection_Option_names[Sensor + 1], OLED_8X16);
	}
	OLED_ShowString(40, 2, ">", OLED_8X16);
	OLED_ShowString(80, 2, "<", OLED_8X16);	

	if(Sensor < 3){
        // 直接使用传入的current_key_frame参数，不再从EEPROM读取
        char key_frame_str[10];
        sprintf(key_frame_str, "T:%u", (unsigned int)current_key_frame); 
        OLED_ShowString(43, 21, "     ", OLED_6X8);  // 清除5个字符宽度
        OLED_ShowString(43, 21, key_frame_str, OLED_6X8);
		OLED_ShowString(80, 21, "       ", OLED_6X8);       
        // 保持原有的"xx.xxxx"占位符
		if(Data_name == 0){
			OLED_ShowString(80, 21, (char *)Collection_names[segment_id], OLED_6X8);
        }
		else{
			OLED_ShowString(80, 21, "xx.xxxx", OLED_6X8);		
		}
        OLED_DrawLine(44, 29, 44, 63);
    } else {
        OLED_ClearArea(0, 20, 127, 63);
    }
	switch(Sensor){

		case 0:
			OLED_ShowString(7, 21, (char *)ADC_Selection[Data_name], OLED_6X8);
//			ADC_Data(Data_name);
			break;
		case 1:
			OLED_ShowString(7, 21, (char *)EulerAngle_Selection[Data_name], OLED_6X8);
//			EulerAngle_Data(Data_name);
			break;
		case 2:
			OLED_ShowString(7, 21, (char *)Quaternion_Selection[Data_name], OLED_6X8);
//			Quaternion_Data(Data_name);
			break;
	}

	OLED_Update();
}

//void ADC_Data(uint8_t Adc_Value)
//{
//	uint16_t A1 = AD_Value[0];
//	uint16_t A2 = AD_Value[1]; 	
//	uint16_t A3 = AD_Value[2];	
//	uint16_t A4 = AD_Value[3];	
//	uint16_t A5 = AD_Value[4];	
//	switch(Adc_Value){
//		case 0:
//			OLED_ShowNum(20, 22, A1, 4, OLED_6X8);  // 显示4位数字
//			OLED_ShowNum(20, 30, A2, 4, OLED_6X8);  // 显示4位数字
//			OLED_ShowNum(20, 38, A3, 4, OLED_6X8);  // 显示4位数字
//			OLED_ShowNum(20, 46, A4, 4, OLED_6X8);  // 显示4位数字
//			OLED_ShowNum(20, 54, A5, 4, OLED_6X8);  // 显示4位数字	
//			break;		
//		case 1:
//			OLED_ShowNum(20, 22, A1, 4, OLED_6X8);  // 显示4位数字
//			break;
//		case 2:
//			OLED_ShowNum(20, 22, A2, 4, OLED_6X8);  // 显示4位数字
//			break;	
//		case 3:
//			OLED_ShowNum(20, 22, A3, 4, OLED_6X8);  // 显示4位数字
//			break;	
//		case 4:
//			OLED_ShowNum(20, 22, A4, 4, OLED_6X8);  // 显示4位数字	
//			break;
//		case 5:
//			OLED_ShowNum(20, 22, A5, 4, OLED_6X8);  // 显示4位数字	
//			break;
//	}	
//	    OLED_Update();
//}
//void EulerAngle_Data(uint8_t Data_name)
//{
//    extern GY95T_Data_t sensor_data;  // 声明外部传感器数据结构
//    
//    char value_str[16];

//    switch(Data_name) {
//        case 0:  // 显示所有欧拉角
//            // 直接显示数字，不格式化
//            sprintf(value_str, "%.2f", sensor_data.yaw);
//            OLED_ShowString(20, 22, value_str, OLED_6X8);
//            
//            sprintf(value_str, "%.2f", sensor_data.roll);
//            OLED_ShowString(20, 32, value_str, OLED_6X8);
//            
//            sprintf(value_str, "%.2f", sensor_data.pitch);
//            OLED_ShowString(20, 42, value_str, OLED_6X8);
//            break;
//            
//        case 1:  // 显示Yaw（航向角）
//            sprintf(value_str, "%.2f", sensor_data.yaw);
//            OLED_ShowString(20, 22, value_str, OLED_6X8);
//            break;
//            
//        case 2:  // 显示Roll（横滚角）
//            sprintf(value_str, "%.2f", sensor_data.roll);
//            OLED_ShowString(20, 22, value_str, OLED_6X8);
//            break;
//            
//        case 3:  // 显示Pitch（俯仰角）
//            sprintf(value_str, "%.2f", sensor_data.pitch);
//            OLED_ShowString(20, 22, value_str, OLED_6X8);
//            break;
//    }
//    
//    OLED_Update();
//}

//void Quaternion_Data(uint8_t Data_name)
//{
//    extern GY95T_Data_t sensor_data;  // 声明外部传感器数据结构
//    
//    char value_str[16];
//    
//    switch(Data_name) {
//        case 0:  // 显示所有四元数
//            // 直接显示数字，不格式化
//            sprintf(value_str, "%.4f", sensor_data.q0);
//            OLED_ShowString(20, 22, value_str, OLED_6X8);
//            
//            sprintf(value_str, "%.4f", sensor_data.q1);
//            OLED_ShowString(20, 32, value_str, OLED_6X8);
//            
//            sprintf(value_str, "%.4f", sensor_data.q2);
//            OLED_ShowString(20, 42, value_str, OLED_6X8);
//            
//            sprintf(value_str, "%.4f", sensor_data.q3);
//            OLED_ShowString(20, 52, value_str, OLED_6X8);
//            break;
//            
//        case 1:  // 显示Q0
//            sprintf(value_str, "%.4f", sensor_data.q0);
//            OLED_ShowString(20, 22, value_str, OLED_6X8);
//            break;
//            
//        case 2:  // 显示Q1
//            sprintf(value_str, "%.4f", sensor_data.q1);
//            OLED_ShowString(20, 22, value_str, OLED_6X8);
//            break;
//            
//        case 3:  // 显示Q2
//            sprintf(value_str, "%.4f", sensor_data.q2);
//            OLED_ShowString(20, 22, value_str, OLED_6X8);
//            break;
//            
//        case 4:  // 显示Q3
//            sprintf(value_str, "%.4f", sensor_data.q3);
//            OLED_ShowString(20, 22, value_str, OLED_6X8);
//            break;
//    }
//    
//    OLED_Update();
//}
