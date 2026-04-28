#include "stm32f10x.h" 
#include "Python_Menu_Process.h"
#include "Program_Editor.h"
#include "OLED.h"
#include "Key.h"
extern MenuLevel_t current_menu_level;
const char *Edit_Option_names[] = {"语句", "运算", "返回", "采集", "退出"};
const char *Logical_Option_names[] = {"+", "-", " ++", " --", "返回"};//{1,1,2,2}
const char *Statement_Option_names[] = {" if", "else", "else if", "  for", " while", "返回"};//{2,4,7,3,5}
const char *Break_Option_names[] = {"break", "continue", " return", "返回"};//{5,8,6,4}

const char *Comparison_Option_names[] = {">", "<", " ==", " !=", " &&", " ||", "返回"};//{1,1,2,2,2,4}

void Edit_Menu_Frame(uint8_t Edit_num,uint8_t Option_num)
{
	OLED_ShowImage(0, 0, 128, 20, Prompt_box);
	OLED_ShowString(16, 34, ">", OLED_8X16);
	OLED_ShowString(104, 34, "<", OLED_8X16);		
	OLED_ShowImage(32, 31, 64, 22, Condition_box);
	if (Edit_num == 0){
	OLED_ShowImage(32, 20, 64, 9, Condition_box_5);
	} else {
	OLED_ShowImage(32, 20, 64, 9, Condition_box_3);
	}
	if (Edit_num == Option_num) {
		OLED_ShowImage(32, 54, 64, 9, Condition_box_5);
	} else {
		OLED_ShowImage(32, 54, 64, 9, Condition_box_4);
	}
	
}

void Edit_selection_Show(uint8_t Edit_Option_num)
{
	Edit_Menu_Frame(Edit_Option_num,4);
	if(Edit_Option_num == 3)
		OLED_ShowString(32, 2, "数据采集",OLED_8X16);
	else if(Edit_Option_num == 4)
		OLED_ShowString(32, 2, "退出添加",OLED_8X16);	
	else
	OLED_ShowString(16, 2, "选择添加类型",OLED_8X16);
	OLED_ShowString(48, 34, (char *)Edit_Option_names[Edit_Option_num], OLED_8X16);
}

void Edit_Statement_Option_selection_Show(uint8_t Statement_num)
{
	Edit_Menu_Frame(Statement_num,5);
	if(Statement_num == 5)
		OLED_ShowString(32, 2, "退出添加",OLED_8X16);
	else
	OLED_ShowString(32, 2, "选中添加",OLED_8X16);
	if(Statement_num > 1 && Statement_num != 5)	//奇数
		OLED_ShowString(36, 34, (char *)Statement_Option_names[Statement_num], OLED_8X16);
	else //偶数、返回
		OLED_ShowString(48, 34, (char *)Statement_Option_names[Statement_num], OLED_8X16);
}

void Edit_Break_Option_selection_Show(uint8_t Break_num)
{
	Edit_Menu_Frame(Break_num,3);
	if(Break_num == 3)
		OLED_ShowString(32, 2, "退出添加",OLED_8X16);
	else
		OLED_ShowString(32, 2, "选中添加",OLED_8X16);
	if(Break_num == 0)	//奇数
		OLED_ShowString(49, 38, (char *)Break_Option_names[Break_num], OLED_6X8);
	else if(Break_num == 3)	//返回
		OLED_ShowString(48, 34, (char *)Break_Option_names[Break_num], OLED_8X16);
	else	//偶数
		OLED_ShowString(40, 38, (char *)Break_Option_names[Break_num], OLED_6X8);
}

void Edit_Comparison_Option_selection_Show(uint8_t Comparison_num)
{
	Edit_Menu_Frame(Comparison_num,6);
	if(Comparison_num == 6)
		OLED_ShowString(32, 2, "退出添加",OLED_8X16);
	else
	OLED_ShowString(32, 2, "选中添加",OLED_8X16);
	if(Comparison_num < 2)	//奇数
		OLED_ShowString(60, 34, (char *)Comparison_Option_names[Comparison_num], OLED_8X16);
	else	//偶数、返回
		OLED_ShowString(48, 34, (char *)Comparison_Option_names[Comparison_num], OLED_8X16);
}

void Edit_Logical_Option_selection_Show(uint8_t Logical_num)
{
	Edit_Menu_Frame(Logical_num,4);
	if(Logical_num == 4)
		OLED_ShowString(32, 2, "退出添加",OLED_8X16);
	else
	OLED_ShowString(32, 2, "选中添加",OLED_8X16);
	if(Logical_num < 2)	//奇数
		OLED_ShowString(60, 34, (char *)Logical_Option_names[Logical_num], OLED_8X16);
	else	//偶数、返回
		OLED_ShowString(48, 34, (char *)Logical_Option_names[Logical_num], OLED_8X16);
}



//比较运算
void Comparison_Operation(void)
{
	
}

//逻辑运算
void Logical_Operation(void)
{
	
}

//if语句
void IF_statement(void)
{
	
}

//for语句
void FOR_statement(void)
{

}	

//while语句
void while_statement(void)
{
	
}
