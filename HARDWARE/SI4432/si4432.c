//2015.02.20 433mhz testing program
#include "si4432.h"
#include "spi2.h"
#include "key.h"
#include "usart.h"
#include "delay.h"
// #include "string.h"

#define  TX1_RX0	SPI_RW_Reg(0x0e|0x80, 0x01)		// 发射状态的天线开关定义
#define  TX0_RX1	SPI_RW_Reg(0x0e|0x80, 0x02)		// 接收状态的天线开关定义
#define  TX0_RX0	SPI_RW_Reg(0x0e|0x80, 0x00)

FlagType Flag;

u8 RxBuf[RxBuf_Len]={0};
u8 TxBuf[TxBuf_Len] = {0};  //发射的固定内容的测试信号，第10个数据是前9个数据的校验和，分别为65，66，67，68，69，70，71，72，73，109
u8 ItStatus;  //发送接收中断
u8 RSSI;      //RSSI

u8 count_50hz;
u8 ItStatus1,ItStatus2;
u8 rf_timeout;

//SI4432与STM32电气连接信息
//STM32的IO口配置信息
void Si4432_IO_Init(void)
{
	RCC->APB2ENR|=1<<3;     //使能PORTB时钟  
	GPIOB->CRH&=0XFFF000FF;//PA10设置成上拉输入	  
	GPIOB->CRH|=0X00033800;//PA11\PA12设置成推挽输出高				   
	GPIOB->ODR|=0x07<<10;	   //PA10上拉,默认下拉
	SPI2_Init();			 //初始化SPI2口 
	SPI2_SetSpeed(SPI2_SPEED_8);	
}


//SPI读写SI4432的寄存器函数
//参数：u8 add 要读写的寄存器的地址
//		u8 data_ 要写入寄存器的值
//返回：u8 status 读写成功的状态
u8 SPI_RW_Reg(u8 addr, u8 data_)
{
	u8 status;
	
	NSS = 0;
	SPI2_ReadWriteByte(addr);
	status = SPI2_ReadWriteByte(data_);
	NSS = 1;
	
	return(status);
}


//从SI4432的地址add处读取长度为number字节的数据存于指针*data_所指向的地址空间
//参数：add 要读取的SI4432的起始地址
//      *data_数据读取后存放的地址
//		number读取的信息长度
//		返回读到的字节数
void SPI_Read_Buf(u8 addr, u8 *data_, u8 number)
{
	int number_ctr;
	
	NSS = 0;
	SPI2_ReadWriteByte(addr);
	for(number_ctr=0;number_ctr<number;number_ctr++)
	{
		data_[number_ctr] = SPI2_ReadWriteByte(0);
	}	
		NSS = 1;
}


//向SI4432的地址add处写入存于*data_处的长度为number的数据
//参数：add 要写入的地址
//		*data_待写入值存放的地址
//		number要写入的数据的长度
void SPI_Write_Buf(u8 addr, u8 *data_, u8 number)
{
	u8 number_ctr;
	NSS = 0;  
	SPI2_ReadWriteByte(addr);   
	for(number_ctr=0; number_ctr<number; number_ctr++)
	{
// 		send_byte(*data_);
		SPI2_ReadWriteByte(*data_++);
		
	}
	NSS = 1;
}

//SI4432初始化函数
void Si4432_init(void)
{  

	Si4432_IO_Init();	//SI4432接口初始化
	SDN = 1;			//关闭芯片
 	delay_ms(10);	// RF 模块上电复位
 	
 	SDN = 0;
	delay_ms(500);
	SPI_RW_Reg(READREG + 0x03, 0x00);	 //清RF模块中断	
	SPI_RW_Reg(READREG + 0x04, 0x00);
	
	SPI_RW_Reg(WRITEREG + 0x06, 0x80);  //使能同步字侦测
	SPI_RW_Reg(WRITEREG + 0x07, 0x01);  //进入 Ready 模式
	
//	SPI_RW_Reg(WRITEREG + 0x09, 0x7f);  //负载电容= 12P
	SPI_RW_Reg(WRITEREG + 0x09, 0xD7);  //负载电容= 9P
	SPI_RW_Reg(WRITEREG + 0x69, 0x60);  //AGC
	SPI_RW_Reg(WRITEREG + 0x0a, 0x05);  //关闭低频输出
	SPI_RW_Reg(WRITEREG + 0x0b, 0xea);  //GPIO 0 当做普通输出口
	SPI_RW_Reg(WRITEREG + 0x0c, 0xea);  //GPIO 1 当做普通输出口
//	SPI_RW_Reg(WRITEREG + 0x0b, 0x15);  //GPIO 0 当做RX
//	SPI_RW_Reg(WRITEREG + 0x0c, 0x12);  //GPIO 1 当做TX
	SPI_RW_Reg(WRITEREG + 0x0d, 0xf4);  //GPIO 2 输出收到的数据
//	SPI_RW_Reg(WRITEREG + 0x0c, 0xea);  //GPIO 1 当做普通输出口
//	SPI_RW_Reg(WRITEREG + 0x0d, 0xf4);  //GPIO 2 输出收到的数据
	SPI_RW_Reg(WRITEREG + 0x70, 0x2c);  
	SPI_RW_Reg(WRITEREG + 0x1d, 0x40);  //使能 afc
	
	// 1.2K bps setting
//-------rx bw------------------------
	SPI_RW_Reg(WRITEREG + 0x1c, 0x16);  //发射16个Nibble的Preamble
	SPI_RW_Reg(WRITEREG + 0x20, 0x83);   
	SPI_RW_Reg(WRITEREG + 0x21, 0xc0);  //
	SPI_RW_Reg(WRITEREG + 0x22, 0x13);  // 
	SPI_RW_Reg(WRITEREG + 0x23, 0xa9);  //
	SPI_RW_Reg(WRITEREG + 0x24, 0x00);  //
//	SPI_RW_Reg(WRITEREG + 0x25, 0x04);  //
	SPI_RW_Reg(WRITEREG + 0x25, 0x03);  //
	SPI_RW_Reg(WRITEREG + 0x2a, 0x14);
//----data rate------------------
	SPI_RW_Reg(WRITEREG + 0x6e, 0x09);
	SPI_RW_Reg(WRITEREG + 0x6f, 0xd5);
	//1.2K bps setting end		
	
	SPI_RW_Reg(WRITEREG + 0x30, 0x8c);  //使能PH+ FIFO模式，高位在前面，使能CRC校验
	SPI_RW_Reg(WRITEREG + 0x32, 0xff);  //byte0, 1,2,3 作为头码
	SPI_RW_Reg(WRITEREG + 0x33, 0x42);  //byte 0,1,2,3 是头码，同步字3,2 是同步字
	SPI_RW_Reg(WRITEREG + 0x34, 16);    //发射16个Nibble的Preamble
	SPI_RW_Reg(WRITEREG + 0x35, 0x20);  //需要检测4个nibble的Preamble
	SPI_RW_Reg(WRITEREG + 0x36, 0x2d);  //同步字为 0x2dd4
	SPI_RW_Reg(WRITEREG + 0x37, 0xd4);
	SPI_RW_Reg(WRITEREG + 0x38, 0x00);
	SPI_RW_Reg(WRITEREG + 0x39, 0x00);
	SPI_RW_Reg(WRITEREG + 0x3a, 'H');   //发射的头码为： “HAHA"
	SPI_RW_Reg(WRITEREG + 0x3b, 'A');
	SPI_RW_Reg(WRITEREG + 0x3c, 'H');
	SPI_RW_Reg(WRITEREG + 0x3d, 'A');
	SPI_RW_Reg(WRITEREG + 0x3e, 64);    //总共发射64个字节的数据
	SPI_RW_Reg(WRITEREG + 0x3f, 'H');   //需要校验的头码为：”HAHA"
	SPI_RW_Reg(WRITEREG + 0x40, 'A');
	SPI_RW_Reg(WRITEREG + 0x41, 'H');
	SPI_RW_Reg(WRITEREG + 0x42, 'A');
	SPI_RW_Reg(WRITEREG + 0x43, 0xff);  //头码1,2,3,4 的所有位都需要校验
	SPI_RW_Reg(WRITEREG + 0x44, 0xff);  // 
	SPI_RW_Reg(WRITEREG + 0x45, 0xff);  // 
	SPI_RW_Reg(WRITEREG + 0x46, 0xff);  // 
//--------------------------------------------------------------------------
//	SPI_RW_Reg(WRITEREG + 0x6d, 0x06);  //发射功率设置  0x00:+0dBM  0x01:+3dBM  0x02:+6dBM  0x03:+9dBM  0x04:+11dBM  0x05:+14dBM  0x06:+17dBM  0x07:20dBM
    SPI_RW_Reg(WRITEREG + 0x6d, 0x01);
	SPI_RW_Reg(WRITEREG + 0x79, 0x0);   //不需要跳频
	SPI_RW_Reg(WRITEREG + 0x7a, 0x0);   //不需要跳频
//	SPI_RW_Reg(WRITEREG + 0x71, 0x22);  //发射不需要CLK，FiFo，FSK模式
	SPI_RW_Reg(WRITEREG + 0x71, 0x2B); 	// 发射不需要CLK，FiFo GFSK模式
	SPI_RW_Reg(WRITEREG + 0x72, 0x30);  //频偏为 30KHz
	SPI_RW_Reg(WRITEREG + 0x73, 0x0);   //没有频率偏差
	SPI_RW_Reg(WRITEREG + 0x74, 0x0);   //没有频率偏差
//----------------------------------------------------------------
	SPI_RW_Reg(WRITEREG + 0x75, 0x53);  //频率设置 434
	SPI_RW_Reg(WRITEREG + 0x76, 0x64);  //
	SPI_RW_Reg(WRITEREG + 0x77, 0x00);
	TX0_RX0;	// 天线开关不在发射、接收状态
}


//射频信号强度指示函数
u8 RF4432_RSSI(void)
{	
	if(!(NIRQ))
	{
		ItStatus = SPI_RW_Reg(READREG + 0x04,0x00);  //读中断寄存器
	 	if((ItStatus&0x80)==0x80)
		{
			RSSI = SPI_RW_Reg(READREG + 0x26, 0x00);
		    return 1;
		}
		else
		{
		    return 0;
		}
	}
	else
	{
	return 0;
	}
}

//SI4432接收信号包函数
//参数：*rxBuffer 接收到的数据要存储的地址
//返回：!=0 接收成功
//		0 接收失败
u8 RF4432_RxPacket(u8 *rxBuffer)
{	
	if(!(NIRQ))
	{		
		ItStatus = SPI_RW_Reg(READREG + 0x03,0x00);  //读中断寄存器
	 	if((ItStatus&0x02)==0x02)
		{
		    SPI_Read_Buf(0x7f, rxBuffer, RxBuf_Len);
		    return 1;
		}
		else
		{
		    return 0;
		}
	}
	else
	{
	return 0;
	}
}

//SI4432发送信号包函数  自动计算发送长度
//参数：*dataBuffer 待发送的数据存放的地址
void RF4432_TxPacket(u8 *dataBuffer)
{
    u8 TX_Timeout;

	SPI_RW_Reg(WRITEREG + 0x34, 40);   //发射16个Nibble 的前导码
	SPI_RW_Reg(WRITEREG + 0x3e, TxBuf_Len);  //总共发射Tx_Data_Len个字节的数据
	
	SPI_Write_Buf(WRITEREG + 0x7f, dataBuffer, TxBuf_Len);   //将要发送的数据写入寄存器
    SPI_RW_Reg(WRITEREG + 0x07, 0x09); //进入发射模式	
	
	TX_Timeout = 0;
	while(NIRQ)		//等待中断
	{
	   TX_Timeout++;
		if(TX_Timeout>=150)
		{
		    printf("发送失败...TX ERROR\n");
		    TX_Timeout=0;
			delay_ms(200);
			
			Si4432_init();
			
			break;		//则强制跳出
		}
		delay_ms(5);
	}

	if(!(NIRQ))
	{
		ItStatus = SPI_RW_Reg(READREG + 0x03,0x00);  //读中断寄存器
		if((ItStatus&0x04)==0x04)
		{
		  ItStatus=0  ;//Uart_sentstr("发送成功...TX OK\r\n");
		}
	}
}

//设置SI4432为接受模式
void SetRX_Mode(void)
{	
	SPI_RW_Reg(WRITEREG + 0x07, 0x01);	//进入 Ready 模式
	delay_ms(5);
	TX0_RX1;		//设置天线开关
//    TX1_RX0;
    delay_ms(5);

	SPI_RW_Reg(WRITEREG + 0x08, SPI_RW_Reg(READREG + 0x08, 0x00)|0x02); //接收FIFO清0
	SPI_RW_Reg(WRITEREG + 0x08, SPI_RW_Reg(READREG + 0x08, 0x00)&0xFD);
	
	SPI_RW_Reg(READREG + 0x03, 0x00);	 //清掉现有的中断标志
	SPI_RW_Reg(READREG + 0x04, 0x00);
	
	SPI_RW_Reg(WRITEREG + 0x05, 0x02);  //RF模块收到整包数据后，产生中断
	SPI_RW_Reg(WRITEREG + 0x07, 0x05);  //RF 模块进入接收模式
}

//设置SI4432为发送模式
void SetTX_Mode(void)
{
	SPI_RW_Reg(WRITEREG + 0x07, 0x01);	//rf模块进入Ready模式
	delay_ms(5);
	TX1_RX0;		//设置天线开关的方向
//    TX0_RX1;
	delay_ms(5);
	
	SPI_RW_Reg(WRITEREG + 0x08, SPI_RW_Reg(READREG + 0x08, 0x00)|0x01); //接收FIFO清0
	SPI_RW_Reg(WRITEREG + 0x08, SPI_RW_Reg(READREG + 0x08, 0x00)&0xFE);
	
	SPI_RW_Reg(READREG + 0x03, 0x00);  //清掉现有的中断标志
	
	SPI_RW_Reg(WRITEREG + 0x05, 0x04);	//整包数据发射完后，产生中断
	delay_ms(5);
//	TX0_RX0;
}



//SI4432测试程序	
void test_al(void)
{
	u8 i, chksum;
// 	uchar RSSI_Buf[4];
//  uchar RSSI_Timeout=0;
// 	u8 re_re=0;

 	Si4432_init();    //初始化SS4432
  
	printf("SI4432 初始化成功\n");
    //Uart_sentstr("启动成功...................START\r\n");

	SetRX_Mode();
	
	while(1)
	{	
		
	/*	if(RF4432_RSSI())  //读有接收数据时的RSSI数值
		{
		    RSSI_Timeout=0;
		   // Uart_sentstr("信号强度...");
			RSSI_Buf[0]=RSSI/100 + 0x30;
	        RSSI_Buf[1]=RSSI%100/10 + 0x30;
	        RSSI_Buf[2]=RSSI%10 + 0x30;
			RSSI_Buf[3]='\0';
			if(RSSI_Buf[0])
			RSSI_Buf[0]=0;
			//Uart_sentstr(RSSI_Buf);
		    //Uart_sentstr("\r\n");
		}
		else if(RSSI_Timeout>200)
		{
		    RSSI_Timeout=0;
		    //Uart_sentstr("信号强度...");
			RSSI = SPI_RW_Reg(READREG + 0x26, 0x00);
			RSSI_Buf[0]=RSSI/100 + 0x30;
	        RSSI_Buf[1]=RSSI%100/10 + 0x30;
	        RSSI_Buf[2]=RSSI%10 + 0x30;
			RSSI_Buf[3]='\0';
			//Uart_sentstr(RSSI_Buf);
		    //Uart_sentstr("\r\n");
		}
		RSSI_Timeout++;		*/
		
	
		if(KEY==0)  //发送数据	KEY_Scan()
		{
// 		    re_re=0;
			printf("发送数据\n");
			SetTX_Mode();		// 每间隔一段时间，发射一包数据，并接收 Acknowledge 信号
			RF4432_TxPacket(TxBuf);
		    SetRX_Mode();
			
		}
			
		if(RF4432_RxPacket(RxBuf)!=0)  //有数据接收到 返回1 其余返回0
		{
		    printf("收到数据:%d个\n",RxBuf_Len);
		    for(i=0;i<RxBuf_Len;i++)	
			{
				send_byte(RxBuf[i]);
			}
							
			chksum = 0;
			for(i=0;i<RxBuf_Len-1;i++)  //计算Checksum
			{
        	    chksum += RxBuf[i];          	 		 	
        	}
			
			if(( chksum == RxBuf[RxBuf_Len-1] )&&( RxBuf[0] == 0x41 ))  //校验数据
     		{
// 				LED=0;
// 				re_re=1;
				delay_ms(60);
// 		        LED=1;
			    SetRX_Mode();//Uart_sentstr("校验成功...RX OK\r\n\r\n");
     			;//OPEN_RX_OK;	//点亮指定的LED
        	}
			else
			{
			    SetRX_Mode();//Uart_sentstr("校验失败...RX ERROR\r\n\r\n");
			}
		}//end if(RF4432_RxPacket())
    }//end while
}	


