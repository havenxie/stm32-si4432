#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "stdarg.h"	 	 
#include "stdio.h"	 	 
#include "string.h"	   
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板
//串口2驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2013/2/22
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	   

//////////////////////////////////////////////////////////////////
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
	/* Whatever you require here. If the only file you are using is */ 
	/* standard output using printf() for debugging, no file handling */ 
	/* is required. */ 
}; 
/* FILE is typedef’ d in stdio.h. */ 
FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
_sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
	USART1->DR = (u8) ch;      
	return ch;
}

void send_byte(u8 ch) 
{
	USART1->DR = (u8) ch;
	while((USART1->SR&0X40)==0);//????,??????
		
}
#endif 
//end
//////////////////////////////////////////////////////////////////


//串口发送缓存区 	
__align(8) u8 USART_TX_BUF[USART1_MAX_SEND_LEN]; 	//发送缓冲,最大USART1_MAX_SEND_LEN字节
#ifdef USART1_RX_EN   								//如果使能了接收   	  
//串口接收缓存区 	
u8 USART_RX_BUF[USART1_MAX_RECV_LEN]; 				//接收缓冲,最大USART1_MAX_RECV_LEN个字节.


//通过判断接收连续2个字符之间的时间差不大于10ms来决定是不是一次连续的数据.
//如果2个字符接收间隔超过10ms,则认为不是1次连续数据.也就是超过10ms没有接收到
//任何数据,则表示此次接收完毕.
//接收到的数据状态
//[7]:0,没有接收到数据;1,接收到了一批数据.
//[6:0]:接收到的数据长度
u8 USART_RX_STA=0;   	 
void USART1_IRQHandler(void)
{
	u8 res;	    
	if(USART1->SR&(1<<5))//接收到数据
	{	 
		res=USART1->DR; 			 
		if(USART_RX_STA<USART1_MAX_RECV_LEN)		//还可以接收数据
		{
			TIM4->CNT=0;         					//计数器清空
			if(USART_RX_STA==0)TIM4_Set(1);	 	//使能定时器4的中断 
			USART_RX_BUF[USART_RX_STA++]=res;		//记录接收到的值	 
		}else 
		{
			USART_RX_STA|=1<<7;					//强制标记接收完成
		} 
	}  											 
}   
//初始化IO 串口1
//pclk1:PCLK1时钟频率(Mhz)
//bound:波特率	  
void USART1_Init(u32 pclk1,u32 bound)
{  	 		 
		
	RCC->APB2ENR|=1<<2;   	//使能PORTA口时钟  
	RCC->APB2ENR|=1<<14;  	//使能串口时钟
	GPIOA->CRH&=0XFFFFF00F; 
	GPIOA->CRH|=0X000008B0;//IO????	 
	 	 
	RCC->APB2RSTR|=1<<14;   //复位串口2
	RCC->APB2RSTR&=~(1<<14);//停止复位	   	   
	//波特率设置
 	USART1->BRR=(pclk1*1000000)/(bound);// 波特率设置	 
	USART1->CR1|=0X200C;  	//1位停止,无校验位.
	USART1->CR3=1<<7;   	//使能串口1的DMA发送
	UART_DMA_Config(DMA1_Channel4,(u32)&USART1->DR,(u32)USART_TX_BUF);//DMA1通道7,外设为串口2,存储器为USART2_TX_BUF 
#ifdef USART1_RX_EN		  	//如果使能了接收
	//使能接收中断
	USART1->CR1|=1<<8;    	//PE中断使能
	USART1->CR1|=1<<5;    	//接收缓冲区非空中断使能	    	
	MY_NVIC_Init(3,3,USART1_IRQChannel,2);//组2，最低优先级 
	TIM4_Init(50,7199);		//5ms中断
	USART_RX_STA=0;		//清零
	TIM4_Set(0);			//关闭定时器4
#endif										  	
}
//串口1,printf 函数
//确保一次发送数据不超过USART1_MAX_SEND_LEN字节
void Usart1_Printf(char* fmt,...)  
{  
	va_list ap;
	va_start(ap,fmt);
	vsprintf((char*)USART_TX_BUF,fmt,ap);
	va_end(ap);
	while(DMA1_Channel4->CNDTR!=0);	//等待通道4传输完成   
	UART_DMA_Enable(DMA1_Channel4,strlen((const char*)USART_TX_BUF)); 	//通过dma发送出去
}
//定时器4中断服务程序		    
void TIM4_IRQHandler(void)
{ 	
	if(TIM4->SR&0X01)//是更新中断
	{	 			   
		USART_RX_STA|=1<<7;	//标记接收完成
		TIM4->SR&=~(1<<0);		//清除中断标志位		   
		TIM4_Set(0);			//关闭TIM4  
	}	    
}
//设置TIM4的开关
//sta:0，关闭;1,开启;
void TIM4_Set(u8 sta)
{
	if(sta)
	{
    	TIM4->CNT=0;         //计数器清空
		TIM4->CR1|=1<<0;     //使能定时器4
	}else TIM4->CR1&=~(1<<0);//关闭定时器4	   
}
//通用定时器中断初始化
//这里始终选择为APB1的2倍，而APB1为36M
//arr：自动重装值。
//psc：时钟预分频数		 
void TIM4_Init(u16 arr,u16 psc)
{
	RCC->APB1ENR|=1<<2;	//TIM4时钟使能    
 	TIM4->ARR=arr;  	//设定计数器自动重装值   
	TIM4->PSC=psc;  	//预分频器
 	TIM4->DIER|=1<<0;   //允许更新中断				
 	TIM4->CR1|=0x01;  	//使能定时器4	  	   
   	MY_NVIC_Init(1,3,TIM4_IRQChannel,2);//抢占2，子优先级3，组2	在2中优先级最低								 
}
#endif		 
///////////////////////////////////////USART1 DMA发送配置部分//////////////////////////////////	   		    
//DMA1的各通道配置
//这里的传输形式是固定的,这点要根据不同的情况来修改
//从存储器->外设模式/8位数据宽度/存储器增量模式
//DMA_CHx:DMA通道CHx
//cpar:外设地址
//cmar:存储器地址    
void UART_DMA_Config(DMA_Channel_TypeDef*DMA_CHx,u32 cpar,u32 cmar)
{
 	RCC->AHBENR|=1<<0;			//开启DMA1时钟
	delay_us(5);
	DMA_CHx->CPAR=cpar; 		//DMA1 外设地址 
	DMA_CHx->CMAR=cmar; 		//DMA1,存储器地址	 
	DMA_CHx->CCR=0X00000000;	//复位
	DMA_CHx->CCR|=1<<4;  		//从存储器读
	DMA_CHx->CCR|=0<<5;  		//普通模式
	DMA_CHx->CCR|=0<<6;  		//外设地址非增量模式
	DMA_CHx->CCR|=1<<7;  		//存储器增量模式
	DMA_CHx->CCR|=0<<8;  		//外设数据宽度为8位
	DMA_CHx->CCR|=0<<10; 		//存储器数据宽度8位
	DMA_CHx->CCR|=1<<12; 		//中等优先级
	DMA_CHx->CCR|=0<<14; 		//非存储器到存储器模式		  	
} 
//开启一次DMA传输
void UART_DMA_Enable(DMA_Channel_TypeDef*DMA_CHx,u8 len)
{
	DMA_CHx->CCR&=~(1<<0);       //关闭DMA传输 
	DMA_CHx->CNDTR=len;          //DMA1,传输数据量 
	DMA_CHx->CCR|=1<<0;          //开启DMA传输
}	   
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 									 





















