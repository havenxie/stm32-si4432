#include "ds18b20.h"
#include "delay.h"	
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//Mini STM32开发板
//DS18B20 驱动代码		   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2010/6/17 
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 正点原子 2009-2019
//All rights reserved
//////////////////////////////////////////////////////////////////////////////////	 
u8 RomCode[8]; 
u8 noskip=1;//跳过ROM检测 1不跳过rom 0跳过rom

u8 DS18B20_ROM[10][8]={
40,255,110,126,98,20,3,153,//DS18B20节点1的ROM
40,255,162,112,98,20,3,143,//DS18B20节点2的ROM值
40,255,110,126,98,20,3,153,//DS18B20节点1的ROM
40,255,162,112,98,20,3,143,//DS18B20节点2的ROM值
40,255,110,126,98,20,3,153,//DS18B20节点1的ROM
40,255,162,112,98,20,3,143,//DS18B20节点2的ROM值
40,255,110,126,98,20,3,153,//DS18B20节点1的ROM
40,255,162,112,98,20,3,143,//DS18B20节点2的ROM值
40,255,110,126,98,20,3,153,//DS18B20节点1的ROM
40,255,162,112,98,20,3,143//DS18B20节点2的ROM值	
};
//复位DS18B20
void DS18B20_Rst(void)	   
{                 
	DS18B20_IO_OUT(); //SET PA0 OUTPUT
    DS18B20_DQ_OUT=0; //拉低DQ
    delay_us(750);    //拉低750us
    DS18B20_DQ_OUT=1; //DQ=1 
	delay_us(15);     //15US
}
//等待DS18B20的回应
//返回1:未检测到DS18B20的存在
//返回0:存在
u8 DS18B20_Check(void) 	   
{   
	u8 retry=0;
	DS18B20_IO_IN();//SET PA0 INPUT	 
    while (DS18B20_DQ_IN&&retry<200)
	{
		retry++;
		delay_us(1);
	};	 
	if(retry>=200)return 1;
	else retry=0;
    while (!DS18B20_DQ_IN&&retry<240)
	{
		retry++;
		delay_us(1);
	};
	if(retry>=240)return 1;	    
	return 0;
}
//从DS18B20读取一个位
//返回值：1/0
u8 DS18B20_Read_Bit(void) 			 // read one bit
{
    u8 data;
	DS18B20_IO_OUT();//SET PA0 OUTPUT
    DS18B20_DQ_OUT=0; 
	delay_us(2);
    DS18B20_DQ_OUT=1; 
	DS18B20_IO_IN();//SET PA0 INPUT
	delay_us(12);
	if(DS18B20_DQ_IN)data=1;
    else data=0;	 
    delay_us(50);           
    return data;
}
//从DS18B20读取一个字节
//返回值：读到的数据
u8 DS18B20_Read_Byte(void)    // read one byte
{        
    u8 i,j,dat;
    dat=0;
	for (i=1;i<=8;i++) 
	{
        j=DS18B20_Read_Bit();
        dat=(j<<7)|(dat>>1);
    }						    
    return dat;
}
//写一个字节到DS18B20
//dat：要写入的字节
void DS18B20_Write_Byte(u8 dat)     
 {             
    u8 j;
    u8 testb;
	DS18B20_IO_OUT();//SET PA0 OUTPUT;
    for (j=1;j<=8;j++) 
	{
        testb=dat&0x01;
        dat=dat>>1;
        if (testb) 
        {
            DS18B20_DQ_OUT=0;// Write 1
            delay_us(2);                            
            DS18B20_DQ_OUT=1;
            delay_us(60);             
        }
        else 
        {
            DS18B20_DQ_OUT=0;// Write 0
            delay_us(60);             
            DS18B20_DQ_OUT=1;
            delay_us(2);                          
        }
    }
}
//开始温度转换
void DS18B20_Start(void)// ds1820 start convert
{   	
// 	u8 i;
    DS18B20_Rst();	   
	DS18B20_Check();
	
    DS18B20_Write_Byte(0xcc);// skip rom
    DS18B20_Write_Byte(0x44);// convert
} 
//初始化DS18B20的IO口 DQ 同时检测DS的存在
//返回1:不存在
//返回0:存在    	 
u8 DS18B20_Init(void)
{
	RCC->APB2ENR|=1<<2;    //使能PORTA口时钟 
	GPIOA->CRL&=0XFFFFFFF0;//PORTA.0 推挽输出
	GPIOA->CRL|=0X00000003;
	GPIOA->ODR|=1<<0;      //输出1
	DS18B20_Rst();
	return DS18B20_Check();
}  
//从ds18b20得到温度值
//精度：0.1C
//返回值：温度值 （-550~1250） 
short DS18B20_Get_Temp(u8 Channel)
{
    u8 temp ,i;
    u8 TL,TH;
	short tem;
//     DS18B20_Start ();                    // ds1820 start convert
    DS18B20_Rst();
    DS18B20_Check();
	if(noskip==1)//不跳过
	{
		DS18B20_Write_Byte(0x55);
		for(i=0;i<8;i++)DS18B20_Write_Byte(DS18B20_ROM[Channel][i]); //??64?ROM?
	}
	else
        DS18B20_Write_Byte(0xcc);// skip rom
    DS18B20_Write_Byte(0xbe);// convert	    
    TL=DS18B20_Read_Byte(); // LSB   
    TH=DS18B20_Read_Byte(); // MSB  
	    	  
    if(TH>7)
    {
        TH=~TH;
        TL=~TL; 
        temp=0;//温度为负  
    }else temp=1;//温度为正	  	  
    tem=TH; //获得高八位
    tem<<=8;    
    tem+=TL;//获得底八位
    tem=(float)tem*0.625;//转换     
	if(temp)return tem; //返回温度值
	else return -tem;    
} 
 
void Read_RomCord(void)//读取ROM码
{
	u8 j;
	DS18B20_Init();
	DS18B20_Write_Byte(0x33);//读取序列码操作
	for(j=0;j<8;j++)
	{
		RomCode[j] = DS18B20_Read_Byte() ;
	}	
}

u8 crc;
u8 CRC8(void)//CRC校验
{
	u8 i,x;
	u8 crcbuff;
	crc=0;
	for(x=0;x<8;x++)
	{
		crcbuff=RomCode[x];
		for(i=0;i<8;i++)
		{
			if(((crc^crcbuff)&0x01)==0)
			crc >>= 1; 	
			else { 
              crc ^= 0x18;   //CRC=X8+X5+X4+1
              crc >>= 1; 
              crc |= 0x80; 
            }
			crcbuff >>= 1; 			
		}
	}
	return crc;
	
}

