#include <stm32f10x_lib.h>
#include "sys.h"
#include "usart.h"		
#include "delay.h"	
#include "led.h" 
#include "key.h"
#include "24cxx.h"
#include "timer.h"
#include "oled.h"
#include "si4432.h"
#include "usmart.h"
#include "string.h"
// #include "exti.h"
// #include "wdg.h"
// #include "rtc.h"
// #include "wkup.h"
// #include "adc.h"
// #include "dma.h"
// #include "flash.h"
// #include "touch.h"
// #include "24l01.h"
// #include "mmc_sd.h"
// #include "remote.h"
// #include "ds18b20.h"

//STM32F103C8T6-SI4432_SMT测试程序

//时间 2015年 2月20日
//测试者：谢斌 上海旗升电气有限公司
//SI4432型号:XL_4432-SMT（板载弹簧天线）
//底板电路：借用上海旗升电气有限公司 QS_TS1630_CTRL_V1.2_MB电路板

//程序可配置0.96′OLED
//OLED型号：0.96寸 四线SPI接口（模拟SPI）  七脚排针

//实现功能：首次实现SI4432的无线通信功能，上电后模块皆为接受状态，当PA8
//被触发到低电平后模块向外发射数据，只是基本通信功能的实现，不存在地址码
//与通信协议。

 //最终目地：
 //1.采用串口1与外部进行通信，可传输接收任意长度数据;
 //2.将接收后的数据无条件发送给指定地址的si4422模块;
 //3.接收模块收到数据后无条件的将有效数据通过串口1的DMA传出；
 //即：基于SI4432的点对点串口透传模块
 
//采用串口1与PC通信
//2月27日 调通串口1的任意数据长度接收
//2月27日 调通串口1的DMA发送
//2月27日 首次完成串口透传（没有地址）
/*SI4432电气连接信息
GND------------>GND
SDN------------>PB11
NIRQ----------->PB10
NSEL----------->PB12
SCLK----------->PB13(SPI2)
SDI------------>PB15(SPI2)
SDO------------>PB14(SPI2)
VCC------------>VCC3.3(1.8~3.6v)

*/

/*OLED接线信息
GND------------>GND
VCC------------>VCC3.3
D0(SCLK) ------>PB0
D1(SDIN) ------>PB1
RST------------>PB14
DC------------->PC8
CS------------->PC9
*/




int main(void)
{		
	u8 FLAG=0;
  	Stm32_Clock_Init(9);//系统时钟设置
	delay_init(72);		//延时初始化
	USART1_Init(72,115200); //串口1初始化 
	KEY_Init();
	LED_Init();         //LED初始化	
	Si4432_init();
	SetRX_Mode();
	
	while(1)
	{

		if((USART_RX_STA&0x80)==0x80)//串口有数据收到 ==的优先级高于&的优先级
		{
			FLAG=USART_RX_STA;
// 			printf("已经接收到数据%d个\n",(u8)(USART_RX_STA&0X3F));
// 			Usart1_Printf((char *)USART_RX_BUF);//将收到的数据返回给串口
			memcpy(&TxBuf[3],USART_RX_BUF ,USART_RX_STA&0X3F);//将串口收到的数据放在SI4432发送缓冲区里
			memset(USART_RX_BUF,0,USART_RX_STA&0X3F);
			USART_RX_STA=0;			//将串口数据读出后 串口标志位清零
			
			
// 			printf("通过SI4432发送数据\n");
			TxBuf[0]=DestID;//目的地址
			TxBuf[1]=MyID;//转载源地址
			TxBuf[2]=FLAG&0X3F;///装载要发送的数据长度
			SetTX_Mode();		// 
			RF4432_TxPacket(TxBuf);//将SI4432发送缓冲区中的数据发送出去
			memset(TxBuf,0x00,TxBuf_Len);
		    SetRX_Mode();
			
		}//end if((USART_RX_STA&0x80)==0x80)
		
		if(RF4432_RxPacket(RxBuf)!=0)  //有数据接收到 返回1 其余返回0
		{
// 		    printf("收到数据:%d个\n",RxBuf[2]);
		    Usart1_Printf((char *)&RxBuf[3]);//将收到的有效数据发送到串口
			memset(RxBuf,0x00,RxBuf_Len);
			SetRX_Mode();				
		}//end if(RF4432_RxPacket())
	}	
}


				 
