#ifndef __SPI2_H
#define __SPI2_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板
//SPI驱动 代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/9/9
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////

				    
// SPI总线速度设置 
#define SPI2_SPEED_2   		0
#define SPI2_SPEED_4   		1
#define SPI2_SPEED_8   		2
#define SPI2_SPEED_16  		3
#define SPI2_SPEED_32 		4
#define SPI2_SPEED_64 		5
#define SPI2_SPEED_128 		6
#define SPI2_SPEED_256 		7
						  	    													  
void SPI2_Init(void);			 //初始化SPI2口
void SPI2_SetSpeed(u8 SpeedSet); //设置SPI2速度   
u8 SPI2_ReadWriteByte(u8 TxData);//SPI2总线读写一个字节
		 
#endif

