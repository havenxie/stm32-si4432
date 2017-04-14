#include "remote.h"
#include "delay.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//Mini STM32开发板
//红外遥控接收 驱动代码		   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2010/6/17 
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 正点原子 2009-2019
//All rights reserved
////////////////////////////////////////////////////////////////////////////////// 	  
 	  		  
u32 Remote_Odr=0;  	 //命令暂存处
u8  Remote_Cnt=0;    //按键次数,此次按下键的次数
u8  Remote_Rdy=0;    //红外接收到数据    
//初始化红外接收引脚的设置
//开启中断,并映射 
void Remote_Init(void)
{							 
	RCC->APB2ENR|=1<<2;       //PA时钟使能		  
	GPIOA->CRL&=0XFFFFFF0F;
	GPIOA->CRL|=0X00000080;	//PA1输入	 
	GPIOA->ODR|=1<<1;		//PA.1上拉      
	Ex_NVIC_Config(GPIO_A,1,FTIR);//将line1映射到PA.1，下降沿触发.
	MY_NVIC_Init(2,1,EXTI1_IRQChannel,2);
}   
//检测脉冲宽度
//最长脉宽为5ms
//返回值:x,代表脉宽为x*20us(x=1~250);
u8 Pulse_Width_Check(void)
{
    u8 t=0;	 
    while(RDATA)
    {	 
		t++;delay_us(20);					 
        if(t==250)return t; //超时溢出
    }
    return t;
}			   
//处理红外接收  
/*-------------------------协议--------------------------
开始拉低9ms,接着是一个4.5ms的高脉冲,通知器件开始传送数据了
接着是发送4个8位二进制码,第一二个是遥控识别码(REMOTE_ID),第一个为
正码(0),第二个为反码(255),接着两个数据是键值,第一个为正码
第二个为反码.发送完后40ms,遥控再发送一个9ms低,2ms高的脉冲,
表示按键的次数,出现一次则证明只按下了一次,如果出现多次,则可
以认为是持续按下该键.
---------------------------------------------------------*/		 
//外部中断服务程序	   
void EXTI1_IRQHandler(void)
{       
	u8 res=0;
    u8 OK=0; 
    u8 RODATA=0;   		 
	while(1)
    {        
        if(RDATA)//有高脉冲出现
        {
            res=Pulse_Width_Check();//获得此次高脉冲宽度       
            if(res==250)break;//非有用信号
            if(res>=200&&res<250)OK=1; //获得前导位(4.5ms)
            else if(res>=85&&res<200)  //按键次数加一(2ms)
            {  							    		 
                Remote_Rdy=1;//接受到数据
                Remote_Cnt++;//按键次数增加
                break;
            }
            else if(res>=50&&res<85)RODATA=1;//1.5ms
            else if(res>=10&&res<50)RODATA=0;//500us
            if(OK)
            {
                Remote_Odr<<=1;
                Remote_Odr+=RODATA; 
                Remote_Cnt=0; //按键次数清零
            }   
        }			 						 
    } 	 	    
	EXTI->PR=1<<1;      //清除中断标志位        
}  
//处理红外键盘
//返回相应的键值
u8 Remote_Process(void)
{               
    u8 t1,t2;   
    t1=Remote_Odr>>24; //得到地址码
    t2=(Remote_Odr>>16)&0xff;//得到地址反码 
    Remote_Rdy=0;//清除标记 		      
    if(t1==(u8)~t2&&t1==REMOTE_ID)//检验遥控识别码(ID)及地址 
    { 
        t1=Remote_Odr>>8;
        t2=Remote_Odr; 	
        if(t1==(u8)~t2)return t1; //处理键值  
    }     
    return 0;
}































