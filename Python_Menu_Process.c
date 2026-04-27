#include "stm32f10x.h"
#include "Python_Menu_Process.h"
#include "Python_Menu_Level_1.h"
#include "Python_Menu_Level_3.h"
#include "Program_Editor.h"
#include "Sensor_Collection.h"
#include "Sensor_Recording.h"
#include "Sensor_Convert.h" 
#include "CAT24M01.h"
#include "Menu.h"
#include "Delay.h"
#include "Key.h"
#include "OLED.h"
#include <stdio.h>

extern uint8_t current_menu_handler;
#define EEPROM_PYTHON_PROGRAM_ADDR 0x0002  // 添加这行
uint8_t current_program = 0;
// 全局菜单状态
static MenuLevel_t current_menu_level = MENU_LEVEL_PROGRAM;
static uint8_t current_Function_Condition = 0;
// 添加条件层级状态保存数组
static uint8_t condition_states[7] = {0}; // 对应7个条件层级（从FUNCTION到CONDITION_6）
static uint8_t current_option = 0;
static uint8_t current_edit_option = 0;

// 全局变量区域添加
static MenuLevel_t menu_level_before_edit = MENU_LEVEL_FUNCTION;

// 录制片段清除说明
uint8_t ClearSegmentHeader(uint8_t segment_id);


/**
	* 函	数： 程序菜单初始化
	* 参	数： 无
	* 返 回 值： 无
	* 说	明： 无
	*/
void Python_Menu_Init(void)
{
    OLED_Clear();
    current_menu_level = MENU_LEVEL_PROGRAM;
    current_option = 0;
    current_Function_Condition = 0;
    
    // 初始化所有条件状态为0
    for(int i = 0; i < 7; i++) {
        condition_states[i] = 0;
    }
	Menu_LoadPythonProgram();  // 这会根据记忆模式设置current_program
    Python_options_selection_Show(current_program, current_option);
}
/**
	* 函	数： 程序菜单处理
	* 参	数： 无
	* 返 回 值： 无
	* 说	明： 无
	*/
void Python_Menu_Handler(void)
{
    uint8_t key = Key_GetNum();
    
    // 处理传感器数据更新
    if (sensor_update_flag) {
        sensor_update_flag = 0;
        
	// 优化：在回放菜单中不读取传感器数据
        if (current_menu_level != 13) {  // 13是MENU_LEVEL_Editor_6
            GY95T_ReadEulerAngle(&sensor_data);
            GY95T_ReadQuaternion(&sensor_data);
        }
    }
    
    // 方案一：处理录制帧保存（不在中断中执行）
    if (Record_CheckNeedSave()) {
        Record_ClearSaveFlag();  // 清除标志
        
        if (Record_IsActive()) {  // 检查是否仍在录制中
            uint8_t save_result = Record_SaveFrame();
            
            // 可选：处理保存错误
            if (save_result != 0) {
                // 连续错误过多，自动停止录制
                static uint8_t consecutive_errors = 0;
                if (save_result == 3) {  // 写入失败
                    consecutive_errors++;
                    if (consecutive_errors >= 5) {  // 连续5次失败
                        Record_Stop();  // 自动停止
                        consecutive_errors = 0;
                    }
                } else {
                    consecutive_errors = 0;
                }
            }
        }
    }
	if (current_menu_level == MENU_LEVEL_Editor_5 &&  // 在采集菜单中
        current_option == 6 &&                         // 当前是录制中状态
        Record_IsActive() == 0) {                     // 但录制已停止
        // 录制已自动停止，切换到录制已满状态
        current_option = 8;
        
        // 立即更新显示
        Sensor_Data_Collection(current_option, current_edit_option);
        
        // 返回，等待用户确认
        return;
    }
    if (!key) return;
    
    switch (current_menu_level) {
        case MENU_LEVEL_PROGRAM:
            Handle_Program_Level(key);
            break;
        case MENU_LEVEL_FUNCTION:
        case MENU_LEVEL_CONDITION_1:
        case MENU_LEVEL_CONDITION_2:
        case MENU_LEVEL_CONDITION_3:
        case MENU_LEVEL_CONDITION_4:
        case MENU_LEVEL_CONDITION_5:
        case MENU_LEVEL_CONDITION_6:
            Handle_Function_Condition_Level(key);
            break;
		case MENU_LEVEL_Editor_1:
            Handle_Editor_Level(key); 
            break;
		case MENU_LEVEL_Editor_2:
			Handle_Editor_Statement_Level(key);
			break;
		case MENU_LEVEL_Editor_3:
			Handle_Editor_Logical_Statement_Level(key);
			break;
		case MENU_LEVEL_Editor_4:
			Handle_Editor_Break_Statement_Level(key);
			break;
		case MENU_LEVEL_Editor_5:
			Handle_Collection_Level(key);
			break;
		case MENU_LEVEL_Editor_6:
			Handle_Replay_Submenu_Level(key);
			break;

    }
    
    OLED_Update();
    Delay_ms(50);
}


// 程序选择层级处理函数
void Handle_Program_Level(uint8_t key)
{
    if (current_option == 0) {
        // 程序选择界面
        switch (key) {
            case 3:  // 左移程序
                current_program = (current_program + 7) % 8;
                Python_selection_Show(current_program);
				Menu_SavePythonProgram();  // 保存变化
                break;
            case 1:  // 右移程序
                current_program = (current_program + 1) % 8;
                Python_selection_Show(current_program);
				Menu_SavePythonProgram();  // 保存变化
                break;
            case 2:  // 进入选项选择
					current_option = 3;
					Python_options_selection_Show(current_program, current_option);
				break;
        }
    } else {
        // 程序选项界面
        switch (key) {
            case 3:  // 左移选项
                if (current_option > 1) {
                    current_option--;
                    Python_options_selection_Show(current_program, current_option);
                }
                break;
            case 1:  // 右移选项
                if (current_option < 5) {
                    current_option++;
                    Python_options_selection_Show(current_program, current_option);
                }
                break;
            case 2:  // 执行选项
                switch (current_option) {
                    case 2:  // "编辑"选项 - 进入功能选择
                        current_menu_level = MENU_LEVEL_FUNCTION;
                        current_Function_Condition = 0;
                        current_option = 0;
                        OLED_Clear();
                        Condition_options_selection_Show(0,current_Function_Condition, current_option);
                        break;
                    case 4:  // "返回"选项 - 返回程序选择
                        current_option = 0;
                        OLED_Clear();
                        Python_options_selection_Show(current_program, current_option);
                        break;
                    case 5:  // "退出"选项
						current_menu_handler = 0;
						Set_CurrentMenuHandler(0);
					// 强制保存程序状态，无论记忆模式如何
						CAT24M01_WriteByte(EEPROM_PYTHON_PROGRAM_ADDR, current_program);
						CAT24M01_WaitForWriteComplete();
						Save_CurrentMenuHandler();  // 保存退出状态
						Menu_Init();
                        // 退出处理
                        break;
                }
                break;
            case 5:  // 快速跳转到退出
                current_option = 5;
                Python_options_selection_Show(current_program, current_option);
                break;
            case 6:  // 快速跳转到清除
                current_option = 1;
                Python_options_selection_Show(current_program, current_option);
                break;
        }
    }
}

/**
	* 函	数： 功能、条件菜单初处理
	* 参	数： condition_loop_level -层级数组 current_option -选项 current_Function_Condition -功能、条件
	* 返 回 值： 无
	* 说	明： 使用condition_states数组保存每个菜单层级的状态
	*/
void Handle_Function_Condition_Level(uint8_t key)
{
    uint8_t current_level_index = current_menu_level - MENU_LEVEL_FUNCTION;
    uint8_t next_level_index = 0;
	// 判断是否为第七级条件（最大层级）
	uint8_t is_seventh_level = (current_level_index == 6); // 索引6对应CONDITION_6

    if (current_option == 0) {
        // 功能选择界面（保持不变）
        switch (key) {
            case 3:  // 左移功能
                if (condition_states[current_level_index] > 0) {
                    condition_states[current_level_index]--;
                    current_Function_Condition = condition_states[current_level_index];
                    Condition_options_selection_Show(current_level_index, 0, condition_states[current_level_index]);
                }
                break;
            case 1:  // 右移功能
                if (condition_states[current_level_index] < 7) {
                    condition_states[current_level_index]++;
                    current_Function_Condition = condition_states[current_level_index];
                    Condition_options_selection_Show(current_level_index, 0, condition_states[current_level_index]);
                }
                break;
            case 2:  // 进入功能选项选择
                current_option = 3;
                OLED_Clear();
                Condition_options_selection_Show(current_level_index, current_option, condition_states[current_level_index]);
                break;
        }
    } else {
        // 功能选项界面
        switch (key) {
            case 3:  // 左移选项
                if (current_option > 1) {
                    current_option--;
                    // 第七级条件下，左移最小只能到"清除"（选项2），因为"扩展"选项被隐藏
                    if (is_seventh_level && current_option < 2) {
                        current_option = 2;
                    }
					
                    Condition_options_selection_Show(current_level_index, current_option, condition_states[current_level_index]);
                }
                break;
            case 1:  // 右移选项
                if (current_option < 6) {
                    current_option++;
                    Condition_options_selection_Show(current_level_index, current_option, condition_states[current_level_index]);
                }
                break;
            case 2:  // 执行选项
                switch (current_option) {
                    case 1:  // "扩展"选项 - 修复：每次进入都重置为初始状态
						if (current_menu_level >= MENU_LEVEL_CONDITION_6) {
                            // 如果已经是最大层级，不允许继续扩展
                            // 可以添加提示或直接返回
                            break;
                        }
						// 保存当前状态
                        condition_states[current_level_index] = current_Function_Condition;
                        
                        // 进入下一级
                        current_menu_level = (MenuLevel_t)(current_menu_level + 1);
                        next_level_index = current_level_index + 1;
                        
                        if (next_level_index < 7) {
                            // 关键修改：总是重置下一级为初始状态（功能一/条件一）
                            condition_states[next_level_index] = 0;  // 重置为0（第一个选项）
                            current_Function_Condition = 0;           // 重置显示值为0
                        }
                        
                        current_option = 0;
                        OLED_Clear();
                        // 显示下一级菜单，始终从第一个选项开始
                        Condition_options_selection_Show(next_level_index, current_option, 0);
                        break;
                        
					case 3:  // "添加"选项
						// 保存进入编辑菜单前的层级
						menu_level_before_edit = current_menu_level;
						
						current_menu_level = MENU_LEVEL_Editor_1;
						current_edit_option = 0;
						OLED_Clear();
						Edit_selection_Show(current_edit_option);
						break;
											
                    case 5:  // "返回"选项
                        current_option = 0;
                        OLED_Clear();
                        Condition_options_selection_Show(current_level_index, current_option, condition_states[current_level_index]);
                        break;
                        
                    case 6:  // "退出"选项
                        if (current_level_index > 0) {
                            current_menu_level = (MenuLevel_t)(current_menu_level - 1);
                            uint8_t prev_level_index = current_level_index - 1;
                            current_Function_Condition = condition_states[prev_level_index];
                            current_option = 0;
                            OLED_Clear();
                            Condition_options_selection_Show(prev_level_index, current_option, condition_states[prev_level_index]);
                        } else {
                            current_menu_level = MENU_LEVEL_PROGRAM;
                            current_option = 0;
                            OLED_Clear();
                            Python_options_selection_Show(current_program, current_option);
                        }
                        break;
                }
                break;
            case 5:  // 快速跳转到退出
                current_option = 6;
                Condition_options_selection_Show(current_level_index, current_option, condition_states[current_level_index]);
                break;
            case 6:  // 快速跳转到清除
                current_option = 1;
				if (is_seventh_level && current_option < 2) {
					current_option = 2;
				}
                Condition_options_selection_Show(current_level_index, current_option, condition_states[current_level_index]);
                break;
        }
    }
}



// 添加编辑菜单处理函数
void Handle_Editor_Level(uint8_t key)
{
    switch (key) {
        case 3:  // 左移选项
            if (current_edit_option > 0) {
                current_edit_option--;
                Edit_selection_Show(current_edit_option);
            }
            break;
        case 1:  // 右移选项
            if (current_edit_option < 4) {  // 对应Edit_Option_names的4个选项(0-3)
                current_edit_option++;
                Edit_selection_Show(current_edit_option);
            }
            break;
        case 2:  // 确认选择
            switch (current_edit_option) {
                case 0:  // "语句"选项
					current_menu_level = MENU_LEVEL_Editor_2;
					current_edit_option = 0;
                    current_option = 0;
                    OLED_Clear();
                    Edit_Statement_Option_selection_Show(current_option);
                    break;
                case 1:  // "运算"选项  
					current_menu_level = MENU_LEVEL_Editor_3;
					current_edit_option = 0;
                    current_option = 0;
                    OLED_Clear();
                    Edit_Logical_Option_selection_Show(current_option);
                    break;
				case 2:  // "返回（语句）"选项  
					current_menu_level = MENU_LEVEL_Editor_4;
					current_edit_option = 0;
                    current_option = 0;
                    OLED_Clear();
                    Edit_Break_Option_selection_Show(current_option);
                    break;
				case 3:	//"采集"选项
					current_menu_level = MENU_LEVEL_Editor_5;
					current_edit_option = 0;
                    current_option = 0;
                    OLED_Clear();
					Sensor_Data_Collection(current_option,current_edit_option);
					break;
				
				case 4:  // "退出"编辑菜单
					// 精确返回到进入编辑菜单前的层级
					current_menu_level = menu_level_before_edit;
					current_option = 0;  // 总是返回到功能/条件选择界面
					
					// 设置当前功能条件状态
					if (current_menu_level >= MENU_LEVEL_FUNCTION && current_menu_level <= MENU_LEVEL_CONDITION_6) {
						uint8_t level_index = current_menu_level - MENU_LEVEL_FUNCTION;
						current_Function_Condition = condition_states[level_index];
					}
					
					OLED_Clear();
					
					// 显示对应的界面
					if (current_menu_level >= MENU_LEVEL_FUNCTION && current_menu_level <= MENU_LEVEL_CONDITION_6) {
						uint8_t level_index = current_menu_level - MENU_LEVEL_FUNCTION;
						Condition_options_selection_Show(level_index, current_option, condition_states[level_index]);
					} else if (current_menu_level == MENU_LEVEL_PROGRAM) {
						Python_options_selection_Show(current_program, current_option);
					}
					break;
					

            }
            break;
		case 5:  // 快速跳转到退出
            current_edit_option = 4;  // 跳转到"退出"选项
            Edit_selection_Show(current_edit_option);
            break;	
    }
}

void Handle_Editor_Statement_Level(uint8_t key)
{
    switch (key) {
        case 3:  // 左移选项
            if (current_edit_option > 0) {
                current_edit_option--;
                Edit_Statement_Option_selection_Show(current_edit_option);
            }
            break;
        case 1:  // 右移选项
            if (current_edit_option < 5) {  
                current_edit_option++;
                Edit_Statement_Option_selection_Show(current_edit_option);
            }
            break;
        case 2:  // 确认选择
            switch (current_edit_option) {
                case 0:  // "if语句"选项
                    break;
                case 1:  // "for"选项  
                    break;
				case 2:  // "while"选项  
                    break;
                case 5:  // "返回"语句添加菜单
                    current_menu_level = MENU_LEVEL_Editor_1;
					current_edit_option = 0;
					OLED_Clear();
                    Edit_selection_Show(current_edit_option);
					break;
            }
            break;
		case 5:  // 快速跳转到返回
            current_edit_option = 5;  // 跳转到"返回"选项
            Edit_Statement_Option_selection_Show(current_edit_option);
            break;
    }
}

void Handle_Editor_Logical_Statement_Level(uint8_t key)
{
	    switch (key) {
        case 3:  // 左移选项
            if (current_edit_option > 0) {
                current_edit_option--;
                Edit_Logical_Option_selection_Show(current_edit_option);
            }
            break;
        case 1:  // 右移选项
            if (current_edit_option < 4) {  
                current_edit_option++;
                Edit_Logical_Option_selection_Show(current_edit_option);
            }
            break;
        case 2:  // 确认选择
            switch (current_edit_option) {
                case 0:  // "+"选项
                    break;
                case 1:  // "-"选项 
                    break;
				case 2:  // "++"选项  
                    break;
				case 3:  // "--"选项  
                    break;
                case 4:  // "返回"语句添加菜单
                    current_menu_level = MENU_LEVEL_Editor_1;
					current_edit_option = 1;
					OLED_Clear();
                    Edit_selection_Show(current_edit_option);
					break;
            }
            break;
		case 5:  // 快速跳转到返回
            current_edit_option = 4;  // 跳转到"返回"选项
            Edit_Logical_Option_selection_Show(current_edit_option);
            break;
    }
}

void Handle_Editor_Break_Statement_Level(uint8_t key)
{
	    switch (key) {
        case 3:  // 左移选项
            if (current_edit_option > 0) {
                current_edit_option--;
                Edit_Break_Option_selection_Show(current_edit_option);
            }
            break;
        case 1:  // 右移选项
            if (current_edit_option < 3) {  
                current_edit_option++;
                Edit_Break_Option_selection_Show(current_edit_option);
            }
            break;
        case 2:  // 确认选择
            switch (current_edit_option) {
                case 0:  // "break"选项
                    break;
                case 1:  // "continue"选项  	
                    break;
				case 2:  // "return"选项  	
					break;
                case 3:  // "返回"语句添加菜单
                    current_menu_level = MENU_LEVEL_Editor_1;
					current_edit_option = 2;
					OLED_Clear();
                    Edit_selection_Show(current_edit_option);
					break;
            }
            break;
		case 5:  // 快速跳转到返回
            current_edit_option = 3;  // 跳转到"返回"选项
            Edit_Break_Option_selection_Show(current_edit_option);
            break;
    }
}


void Handle_Collection_Level(uint8_t key)
{
	 if (current_option == 0) {
	    switch (key) {
        case 3:  // 左移选项
            if (current_edit_option > 0) {
                current_edit_option--;
				current_option = 0;
                Sensor_Data_Collection(current_option,current_edit_option);
            }
            break;
        case 1:  // 右移选项
            if (current_edit_option < 4) {  
                current_edit_option++;
				current_option = 0;
                Sensor_Data_Collection(current_option,current_edit_option);
            }
            break;
		case 2:  // 确认 进入地址数据管理选项
			if(current_edit_option ==4 ){	
				current_menu_level = MENU_LEVEL_Editor_1;
				current_edit_option = 0;
				OLED_Clear();
				Edit_selection_Show(current_edit_option);
			}
			else{
				current_option = 2;
				Sensor_Data_Collection(current_option,current_edit_option);
			}
            break;
		case 5:  // 在DATA选择状态，快速跳转到返回采集菜单
			// 当在DATA选择状态按下按键5，快速进入返回采集菜单
			current_edit_option = 4;  // 跳转到DATA4
			current_option = 0;      // 保持在DATA选择状态
			Sensor_Data_Collection(current_option, current_edit_option);
			break;
		}
	}
	 else	{
		 	    switch (key) {
        case 3:  // 左移选项
            if (current_option > 1 && current_option < 5) {
				current_option--;
                Sensor_Data_Collection(current_option,current_edit_option);
            }
            break;
        case 1:  // 右移选项
            if (current_option < 4) {  
				current_option++;
                Sensor_Data_Collection(current_option,current_edit_option);
            }
            break;
		case 2:  // 确认 进入地址数据管理选项
				if(current_option == 1){  // 清除片段选项
					// 1. 显示"清理"
					current_option = 7;
					Sensor_Data_Collection(current_option, current_edit_option);
					
					// 2. 执行清除
					uint8_t result = ClearSegmentHeader(current_edit_option);
					
					// 3. 统一使用500ms延时
					Delay_ms(300);
					
					// 4. 根据结果设置状态
					if (result == 0) {
						// 清除成功 - 返回到清除选项
						OLED_ShowString(32, 34, "清除成功", OLED_8X16);
						current_option = 2;  // 返回到录制选项
					} else {
						// 清除失败 - 返回到清除选项
						OLED_ShowString(32, 34, "清除失败", OLED_8X16);						
						current_option = 1;  // 返回到清除选项
					}
					OLED_Update();
					Delay_ms(500);
					
					// 5. 更新显示
					Sensor_Data_Collection(current_option, current_edit_option);
				}
				else if(current_option == 4){	//返回选项
					current_menu_level = MENU_LEVEL_Editor_5;
                    current_option = 0;
                    OLED_Clear();
					Sensor_Data_Collection(current_option,current_edit_option);
					}
				else if(current_option == 2){	//录制选项
					current_option = 5;
					Sensor_Data_Collection(current_option,current_edit_option);
				}
				// 在Handle_Collection_Level函数中
				else if(current_option == 5){  // 开始录制选项
					// 开始录制
					uint8_t result = Record_Start(current_edit_option);
					
					if (result == 0) {
						// 录制开始成功
						current_option = 6;  // 切换到录制中状态
					} else {
						// 录制失败，显示数字错误码
						ShowErrorCode(result);
						OLED_Update();
						Delay_ms(1000);
						current_option = 2;  // 返回录制选项
					}
					
					Sensor_Data_Collection(current_option, current_edit_option);
				}
				else if(current_option == 6){  // 停止录制选项
					// 检查录制是否仍在进行
					if (Record_IsActive() == 0) {
						current_option = 8;  
					} else {
						// 正常停止流程
						uint8_t result = Record_Stop();
						if (result == 0) {
							OLED_ShowString(32, 34, "录制结束", OLED_8X16);
							OLED_Update();
							Delay_ms(500);
							current_option = 3;  // 切换到回放选项
						} else {
							ShowErrorCode(result);
							Delay_ms(1000);
						}
					}
					
					Sensor_Data_Collection(current_option, current_edit_option);
				}
				else if(current_option == 8){  // 停止录制选项
					 if (key == 2) {  // 确认键
						current_option = 3;  // 切换到回放
						Sensor_Data_Collection(current_option, current_edit_option);
					}
				}
				else if(current_option == 3){	//回放选项
					current_menu_level = MENU_LEVEL_Editor_6;  // 进入回放菜单
					uint8_t sensor_type = 1;  // 默认选择欧拉角
					uint8_t data_item = 0;
					
					// 修改：立即读取关键帧
					uint16_t initial_key_frame = 0;
					uint8_t result = ReadKeyFrame(current_edit_option, &initial_key_frame);
					
					if (result == 0) {
						// 读取成功，使用实际关键帧
						OLED_Clear();
						Sensor_Data_Playback(sensor_type, data_item, current_edit_option, initial_key_frame);
					} 
				}
				break;
			case 5:  // 快速跳转到返回选项
                if (current_option >= 1 && current_option <= 4) {
                    // 只有当前是功能选项（1-4）时才跳转到返回
                    current_option = 4;  // 跳转到"返回"选项
                    Sensor_Data_Collection(current_option, current_edit_option);
                }
                // 如果已经在其他状态（5,6,7,8），不处理
                break;
		}
	}
}


/**
  * 函    数： 回放子菜单处理（简化版）
  * 参    数： key - 按键值
  * 返 回 值： 无
  * 说    明： 处理回放子菜单下的按键输入，用于选择传感器类型和数据项
  *            简化逻辑：使用 mode 变量区分状态，0=选传感器，1=选数据
  */
void Handle_Replay_Submenu_Level(uint8_t key)
{
	static uint8_t first_time_state1 = 1;  // 添加静态变量标记首次进入
    static uint8_t sensor_type = 1;      // 传感器类型
    static uint8_t data_item = 0;        // 数据项索引
    static uint8_t state = 0;            // 0=初始(切换数据类型)，1=移动所有曲线，2=选择数据项，3=移动单条曲线
	static uint16_t current_key_frame = 0;  // 16位无符号
    
    uint8_t max_sensor_type = 3;         // 传感器类型最大索引
    uint8_t max_data_item = 0;           // 数据项最大索引
    
    // 根据传感器类型确定最大数据项数量
    switch(sensor_type) {
        case 0: max_data_item = 5; break;  // ADC: All, A1-A5
        case 1: max_data_item = 3; break;  // 欧拉角: All, Y, R, P
        case 2: max_data_item = 4; break;  // 四元数: All, Q0-Q3
    }
	
	if (first_time_state1) {
			// 只在首次进入状态1时读取关键帧
			uint16_t key_frame_value = 0;
			uint8_t result = ReadKeyFrame(current_edit_option, &key_frame_value);
			
			if (result == 0) {
				current_key_frame = key_frame_value;
			} else {
				current_key_frame = 44;  // 默认值
			}
			first_time_state1 = 0;  // 标记已读取
		}
		// 后续切换状态时，保留current_key_frame的当前值
		
    switch (key) {
        case 3:  // 左键
            if (state == 0) {
                // 状态0：左右切换数据类型
                if (sensor_type > 0) {
                    sensor_type--;
                    data_item = 0;  // 切换传感器时重置数据项
                }
            } else if (state == 1) {
        // 状态1：移动所有曲线 - 左移
        if (current_key_frame > 0) {
            current_key_frame--;
			}
				Sensor_Data_Playback(sensor_type, data_item, current_edit_option, current_key_frame);
            } else if (state == 2) {
                // 状态2：选择数据项
                if (data_item > 0) {
                    data_item--;
                }
            } else if (state == 3) {
				if (current_key_frame > 0) {
					current_key_frame--;
				}
				Sensor_Data_Playback(sensor_type, data_item, current_edit_option, current_key_frame);
            }
            Sensor_Data_Playback(sensor_type, data_item, current_edit_option, current_key_frame);
            break;
            
        case 1:  // 右键
            if (state == 0) {
                // 状态0：左右切换数据类型
                if (sensor_type < max_sensor_type) {
                    sensor_type++;
                    data_item = 0;  // 切换传感器时重置数据项
                }
            } else if (state == 1) {
				// 状态1/3：移动曲线 - 右移
				// 只读取总帧数（2字节），优化速度
				uint32_t frame_count_addr = 0x10000 + (current_edit_option * 0x4000) + 4;
				uint8_t frame_count_bytes[2];
				
				if (CAT24M01_ReadBuffer(frame_count_addr, frame_count_bytes, 2) == 0) {
					uint16_t total_frames = (frame_count_bytes[1] << 8) | frame_count_bytes[0];
					
					if (total_frames > 0 && current_key_frame < total_frames - 1) {
						current_key_frame++;
						Sensor_Data_Playback(sensor_type, data_item, current_edit_option, current_key_frame);
					}
				}
            } else if (state == 2) {
                // 状态2：选择数据项
                if (data_item < max_data_item) {
                    data_item++;
                }
            } else if (state == 3) {
		// 状态1/3：移动曲线 - 右移
		// 只读取总帧数（2字节），优化速度
		uint32_t frame_count_addr = 0x10000 + (current_edit_option * 0x4000) + 4;
		uint8_t frame_count_bytes[2];
		
		if (CAT24M01_ReadBuffer(frame_count_addr, frame_count_bytes, 2) == 0) {
			uint16_t total_frames = (frame_count_bytes[1] << 8) | frame_count_bytes[0];
			
			if (total_frames > 0 && current_key_frame < total_frames - 1) {
				current_key_frame++;
				Sensor_Data_Playback(sensor_type, data_item, current_edit_option, current_key_frame);
			}
		}
            }
			Sensor_Data_Playback(sensor_type, data_item, current_edit_option, current_key_frame);
            break;
            
        case 2:  // 确认键
            if (state == 0) {
                // 当前是初始状态
                if (sensor_type == 3) {  // 选择了"返回"
                    // 返回到采集菜单
                    current_menu_level = MENU_LEVEL_Editor_5;
                    current_option = 3;
                    
                    // 重置状态
                    sensor_type = 1;
                    data_item = 0;
                    state = 0;
					first_time_state1 = 1;  // 重置首次进入标志
					
                    OLED_Clear();
                    Sensor_Data_Collection(current_option, current_edit_option);
                    return;
                } else {
                    // 第一次确认：进入移动所有曲线状态
                    state = 1;
					if (current_key_frame == 0) {
						uint16_t key_frame_value = 0;
						uint8_t result = ReadKeyFrame(current_edit_option, &key_frame_value);
						
						if (result == 0) {
							// 读取成功
							current_key_frame = key_frame_value;
						} else {
							// 读取失败，设置默认值
							current_key_frame = 44;  // 默认值
						}
					}

                }
            } else if (state == 1) {
                // 第二次确认：进入选择数据项状态
				if (ModifyKeyFrame(current_edit_option, current_key_frame) == 0) {
                state = 2;
				}
            } else if (state == 2) {
                // 第三次确认：进入移动单条曲线状态
                state = 3;

            } else if (state == 3) {
                // 第四次确认：返回初始状态
				if (ModifyKeyFrame(current_edit_option, current_key_frame) == 0) {
                state = 0;
				}
            }
			
            break;
		case 5:  // 快速跳转到返回选项
				if(state == 0){
					sensor_type = 3;  // 选择"返回"选项
					data_item = 0;    // 重置数据项
					Sensor_Data_Playback(sensor_type, data_item, current_edit_option, current_key_frame);
				}
			break;
			
    }
	//光标显示
    OLED_ShowString(0, 21, " ", OLED_6X8);   // 清除(0,21)位置的光标
    OLED_ShowString(37, 21, " ", OLED_6X8);  // 清除(37,21)位置的光标
    
    switch(state) {
        case 0:  // 初始状态：不显示光标
            // 不显示光标
            break;
            
        case 1:  // 移动所有曲线状态：光标在(37,21)
            OLED_ShowString(37, 21, ">", OLED_6X8);
            break;
            
        case 2:  // 选择数据项状态：光标在(0,21)
            OLED_ShowString(0, 21, ">", OLED_6X8);
            break;
            
        case 3:  // 移动单条曲线状态：光标在(37,21)
            OLED_ShowString(37, 21, ">", OLED_6X8);
            break;
    }
	Sensor_Data_Playback(sensor_type, data_item, current_edit_option, current_key_frame);
}

