// Python_Menu_Process.h
#ifndef __PYTHON_MENU_PROCESS_H
#define __PYTHON_MENU_PROCESS_H

#include "stm32f10x.h"

// 菜单层级定义
typedef enum {
    MENU_LEVEL_PROGRAM = 0,    // 程序选择菜单
    MENU_LEVEL_FUNCTION = 1,   // 功能选择菜单
    MENU_LEVEL_CONDITION_1 = 2,    // 条件选择菜单1
	MENU_LEVEL_CONDITION_2 = 3,   // 条件选择菜单2
	MENU_LEVEL_CONDITION_3 = 4,    // 条件选择菜单3
	MENU_LEVEL_CONDITION_4 = 5,    // 条件选择菜单4
	MENU_LEVEL_CONDITION_5 = 6,    // 条件选择菜单5
	MENU_LEVEL_CONDITION_6 = 7,    // 条件选择菜单6
	MENU_LEVEL_Editor_1 = 8,		// 编辑菜单
	MENU_LEVEL_Editor_2 = 9,		// 语句选择菜单
	MENU_LEVEL_Editor_3 = 10,		// 运算选择菜单
	MENU_LEVEL_Editor_4 = 11,		// 返回（语句）选择菜单
	MENU_LEVEL_Editor_5 = 12,		// 采集选择菜单
	MENU_LEVEL_Editor_6 = 13,		// 回放选择菜单
} MenuLevel_t;


// 函数声明
void Python_Menu_Init(void);
void Python_Menu_Handler(void);
void Handle_Program_Level(uint8_t key);
void Handle_Function_Condition_Level(uint8_t key);
void Handle_Editor_Level(uint8_t key);
void Handle_Editor_Statement_Level(uint8_t key);
void Handle_Editor_Logical_Statement_Level(uint8_t key);
void Handle_Editor_Break_Statement_Level(uint8_t key);
void Handle_Collection_Level(uint8_t key);
void Handle_Replay_Submenu_Level(uint8_t key);

#endif
