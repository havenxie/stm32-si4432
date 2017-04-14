#ifndef __SI4432_H
#define __SI4432_H
#include "sys.h"

#define  DestID  0XFF
#define  MyID	 0XAA

#define READREG        0x00  	//读寄存器指令
#define WRITEREG       0x80 	//写寄存器指令

// #define Max_Tx_Data_Len 64//最大的发送数据长度
// #define Max_Rx_Data_Len 64//最大的接收数据长度
#define TxBuf_Len 64 
#define RxBuf_Len 64     //定义RF4432数据包长度
extern u8 RxBuf[RxBuf_Len];
extern u8 TxBuf[TxBuf_Len];

//端口定义
#define NSS PBout(12)//NSEL  SPI片选信号
#define NIRQ PBin(10)
#define SDN PBout(11)//内部电源开关

#define SI4432_PWRSTATE_READY		0x01		// 模块 Ready 状态定义
#define SI4432_PWRSTATE_TX		0x09		// 模块 发射状态定义
#define SI4432_PWRSTATE_RX		0x05		// 模块 接收状态定义
#define SI4432_PACKET_SENT_INTERRUPT	0x04		// 模块 发射完成中断
#define SI4432_Rx_packet_received_interrupt   0x02      // 模块 收到数据包中断

 
// extern u8 RxBuf[RxBuf_Len];
// extern u8 TxBuf[TxBuf_Len];  //每秒发射的固定内容的测试信号，第10个数据是前9个数据的校验和，分别为65，66，67，68，69，70，71，72，73，109
extern u8 ItStatus;  //发送接收中断
extern u8 RSSI;      //RSSI
extern u8 count_50hz;
extern u8 ItStatus1,ItStatus2;
extern u8 rf_timeout;

typedef struct 
{
	
	u8 reach_1s				: 1;
	u8 rf_reach_timeout			: 1;
	u8 is_tx				: 1;
	
	
}	FlagType;



void Si4432_IO_Init(void);
void Si4432_init(void);
u8 SPI_RW_Reg(u8 addr, u8 data_);
void SPI_Read_Buf(u8 addr, u8 *data_, u8 number);
void SPI_Write_Buf(u8 addr, u8 *data_, u8 number);
u8 RF4432_RSSI(void);
u8 RF4432_RxPacket(u8 *rxBuffer);
void RF4432_TxPacket(u8 *dataBuffer);
void SetRX_Mode(void);
void SetTX_Mode(void);
extern void test_al(void);
#endif

