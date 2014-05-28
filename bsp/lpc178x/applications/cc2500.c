//#include <stm32f10x.h>
//#include <stm32f10x_conf.h>
//#include <stm32f10x_tim.h>
#include <rtthread.h>
#include <stdio.h>
//#include "sc_lib.h"
#include "cc2500.h"
#include "led.h"

static const char *sStatus[] = {"IDLE","RX","TX","FSTXON","CALIBRATE","SETTING","RX_OVERFLOW","TX_UNDERFLOW"};
static const char *sRegNs[]= {"PARTNUM","VERSION","FREQEST","LQI","RSSI","MARCSTATE","WORTIME1","WORTIME0",
							"PKTSTATUS","VCO_VC","TXBYTES","RXBYTES","RCCTRL1","RCCTRL0"};

#define CC2500RXBUFLEN	64	  //缓存收到的无线数据的信箱大小 即能缓存CC2500RXBUFLEN/4个邮件
#define CC2500TXBUFLEN	32	  //用于缓存发送无线数据的mailbox的大小为CC2500TXBUFLEN字节，即能缓存C2500TXBUFLEN/4个邮件

static char myRxMode = 1;
static unsigned char cc2500tx_buf[CC2500TXBUFLEN]={0};	//用于缓存发送数据mailbox

static unsigned char cc2500rx_buf[CC2500RXBUFLEN]={0};	//用于缓存收到的数据mailbox

struct rt_mailbox cc2500recvmailbox;
struct rt_mailbox cc2500sendmailbox;
#define CC2500_GDO0_EXTI_LINE			EXTI_Line15
#define CC2500_GDO2_EXTI_LINE		EXTI_Line13	
#define CC2500_IRQ_EXTI_PORT_SOURCE                  GPIO_PortSourceGPIOE
#define CC2500_GDO0_IRQ_EXTI_PIN_SOURCE              GPIO_PinSource15
#define CC2500_GDO2_IRQ_EXTI_PIN_SOURCE              GPIO_PinSource13
#define CC2500_IRQ_EXTI_IRQn                         EXTI15_10_IRQn	
		  


#define WAIT do{}while(0)
#define RF433_CS_DN	  GPIO_OutputValue(CC2500_CSN_PORT,CC2500_CSN_MASK,0)//CC2500_LOW(CSN)
#define RF433_CS_UP	  GPIO_OutputValue(CC2500_CSN_PORT,CC2500_CSN_MASK,1)//CC2500_HIGH(CSN)

extern unsigned char reset_rf;
extern unsigned char b_rf_check;

#define Delay_us(time)	rt_thread_delay(1);
//使用频段不同时候，配置不同的无线参数
//定义一个配置无线模块的参数类


typedef struct S_RF_SETTINGS{
    BYTE FSCTRL1;   // Frequency synthesizer control.
    BYTE FSCTRL0;   // Frequency synthesizer control.
    BYTE FREQ2;     // Frequency control word, high byte.
    BYTE FREQ1;     // Frequency control word, middle byte.
    BYTE FREQ0;     // Frequency control word, low byte.
    BYTE MDMCFG4;   // Modem configuration.
    BYTE MDMCFG3;   // Modem configuration.
    BYTE MDMCFG2;   // Modem configuration.
    BYTE MDMCFG1;   // Modem configuration.
    BYTE MDMCFG0;   // Modem configuration.
    BYTE CHANNR;    // Channel number.
    BYTE DEVIATN;   // Modem deviation setting (when FSK modulation is enabled).
    BYTE FREND1;    // Front end RX configuration.
    BYTE FREND0;    // Front end RX configuration.
    BYTE MCSM0;     // Main Radio Control State Machine configuration.
    BYTE FOCCFG;    // Frequency Offset Compensation Configuration.
    BYTE BSCFG;     // Bit synchronization Configuration.
    BYTE AGCCTRL2;  // AGC control.
	BYTE AGCCTRL1;  // AGC control.
    BYTE AGCCTRL0;  // AGC control.
    BYTE FSCAL3;    // Frequency synthesizer calibration.
    BYTE FSCAL2;    // Frequency synthesizer calibration.
	BYTE FSCAL1;    // Frequency synthesizer calibration.
    BYTE FSCAL0;    // Frequency synthesizer calibration.
    BYTE FSTEST;    // Frequency synthesizer calibration control
    BYTE TEST2;     // Various test settings.
    BYTE TEST1;     // Various test settings.
    BYTE TEST0;     // Various test settings.
    BYTE IOCFG2;    // GDO2 output pin configuration
	BYTE IOCFG1;	// GDO1 output pin configuration
    BYTE IOCFG0;    // GDO0 output pin configuration
    BYTE PKTCTRL1;  // Packet automation control.
    BYTE PKTCTRL0;  // Packet automation control.
    BYTE ADDR;      // Device address.
    BYTE PKTLEN;    // Packet length.
}RF_SETTINGS ;

RF_SETTINGS   rfSettings250k = {
    0x08, 	//0x09  // FSCTRL1   Frequency synthesizer control.
    0x00,   		// FSCTRL0   Frequency synthesizer control.
    0x5D,   		// FREQ2     Frequency control word, high byte.
    0x93,   		// FREQ1     Frequency control word, middle byte.
    0xB1,   		// FREQ0     Frequency control word, low byte.
    0x86, //0x2D,   // MDMCFG4   Modem configuration.
    0x83, //0x3B,   // MDMCFG3   Modem configuration.
    0x03, //0x73,   // MDMCFG2   Modem configuration.
    0x22,   		// MDMCFG1   Modem configuration.
    0xF8,   		// MDMCFG0   Modem configuration.
    0x00,   		// CHANNR    Channel number.
    0x44, //0x01,   // DEVIATN   Modem deviation setting (when FSK modulation is enabled).
    0x56, //0xB6,   // FREND1    Front end RX configuration.
    0x10,   		// FREND0    Front end RX configuration.
    0x18,   		// MCSM0     Main Radio Control State Machine configuration.
    0x16, //0x1D,   // FOCCFG    Frequency Offset Compensation Configuration.
    0x6C, //0x1C,   // BSCFG     Bit synchronization Configuration.
    0x03, //0xC7,   // AGCCTRL2  AGC control.
    0x40, //0x00,   // AGCCTRL1  AGC control.
    0x91, //0xB2,   // AGCCTRL0  AGC control.
    0xac, //0xEA,   // FSCAL3    Frequency synthesizer calibration.
    0xc,   			// FSCAL2    Frequency synthesizer calibration.
    0x1b,   		// FSCAL1    Frequency synthesizer calibration.
    0x11,   		// FSCAL0    Frequency synthesizer calibration.
    0x59,  			// FSTEST    Frequency synthesizer calibration.
    0x88,   		// TEST2     Various test settings.
    0x31,   		// TEST1     Various test settings.
    0x0B,   		// TEST0     Various test settings.
    0x7,   		// IOCFG2    GDO2 output pin configuration.
	0x2e,			// IOCFG1	 GDO1 output pin configuration.
    0x06,   		// IOCFG0   GDO0 output pin configuration. Refer to SmartRF?Studio User Manual for detailed pseudo register explanation.
    0x04,   		// PKTCTRL1  Packet automation control.
    0x05,   		// PKTCTRL0  Packet automation control.
    0x00,   		// ADDR      Device address.
    0xFF    		// PKTLEN    Packet length.
};
//********************************
//写1个字节的时序，同时读出1个字节
rt_inline unsigned char  CC_SpiWr(unsigned char byte)
{
	unsigned char  i;
   	for(i=0; i<8; i++)          // 循环8次
   	{		
		GPIO_WriteBit(CC2500_MOSI_PORT, CC2500_MOSI_PIN, (byte & 0x80)?Bit_SET:Bit_RESET);// byte最高位输出到MOSI
   		byte <<= 1;// 低一位移位到最高位
		GPIO_WriteBit(CC2500_SCK_PORT,CC2500_SCK_PIN,Bit_SET); // 拉高SCK，nRF24L01从MOSI读入1位数据，同时从MISO输出1位数据
		byte |= GPIO_ReadInputDataBit(CC2500_MISO_PORT,CC2500_MISO_PIN);// 读MISO到byte最低位
		GPIO_WriteBit(CC2500_SCK_PORT,CC2500_SCK_PIN,Bit_RESET); // SCK置低
   	}
    return(byte);           	// 返回读出的一字节
}

//**********************************************************************************
//等待芯片SO脚输出低表示Ready
void CC_Rdy()
{
	BYTE i;
   for (i=0;i<10;i++)
   { 
     //if (CHKRFIN==0) return; 
	 if (CC2500_GETVALUE(MISO)==Bit_RESET) return;
     rt_thread_delay(1);//掉电是最大200uS，正常20nS
   }
}
//************************************************************************************
//写命令码
BYTE CC_Cmd(BYTE Cmd)
{
	BYTE Status;
    
	RF433_CS_DN;
	CC_Rdy();
	Status=CC_SpiWr(Cmd);
	RF433_CS_UP;
    WAIT;	
	return Status;
}
//*************************************************************************************
//指定地址，写配置字
BYTE CC_WrReg(BYTE Addr,BYTE Data)
{
	BYTE Status;

    RF433_CS_DN;
	CC_Rdy();
	Status=CC_SpiWr(Addr);
	CC_SpiWr(Data);
	RF433_CS_UP;
	WAIT;
	return Status;
}
//************************************************************************************
//指定地址，连续写配置
BYTE CC_WrRegs(uchar Addr,uchar *Buf,uchar Count)
{
	BYTE Status,i;

    RF433_CS_DN;
	CC_Rdy();
	Status=CC_SpiWr(Addr|0x40);
	for (i=0;i<Count;i++)
	{ CC_SpiWr(Buf[i]);
	}
	RF433_CS_UP;
	WAIT;
	return Status;
}
//*************************************************************************************
//读状态寄存器
BYTE CC_RdStatus(BYTE Addr)
{
	BYTE Data=0;   
    
	RF433_CS_DN;
	CC_Rdy();
	Data=CC_SpiWr(Addr|0xC0);//
	Data=CC_SpiWr(0);
	RF433_CS_UP;
	WAIT;
	return Data;
}
//*****************************************************************************************
//读配置
BYTE CC_RdReg(BYTE Addr)
{
	BYTE Data;   
    
	RF433_CS_DN;
	CC_Rdy();
	Data=CC_SpiWr(Addr|0x80);
	Data=CC_SpiWr(0);
	RF433_CS_UP;
	WAIT;
	return Data;
}
//*******************************************************************************************
//连续读配置字
BYTE CC_RdRegs(uchar Addr,uchar *Buf,uchar Count)
{
	BYTE Status,i;

    RF433_CS_DN;
	CC_Rdy();
	Status=CC_SpiWr(Addr|0xC0);//******
	for (i=0;i<Count;i++)
	{ 
	 Buf[i]=CC_SpiWr(0); //******
	 rt_thread_delay(1);
	}
	RF433_CS_UP;
	WAIT;
	return Status;

}
//******************************************************************************************
//读状态
BYTE CC_GetStatus()
{
  BYTE Data;   
  RF433_CS_DN;
  CC_Rdy();
  Data=CC_SpiWr(0x80);
  CC_SpiWr(0);
  RF433_CS_UP;
  WAIT;
  return Data;
}
//**********************************************************************************
//芯片上电复位
void CC_RESET()
{
   RF433_CS_UP;
   RF433_CS_DN;
   RF433_CS_UP;
   Delay_us(500);//>40us
   
   //CC_Cmd(CCxxx0_SRES);
   RF433_CS_DN;
	CC_Rdy();
	CC_SpiWr(CCxxx0_SRES);
		
   while(CC2500_GETVALUE(MISO) !=Bit_RESET);
   RF433_CS_UP;
    WAIT;
}

//***********************************************************************************
//配置芯片特性和无线参数
void CC_RfConfig(RF_SETTINGS *pRfSettings) 
{    // Write register settings
	
    CC_WrReg(CCxxx0_IOCFG2,   pRfSettings->IOCFG2);
    CC_WrReg(CCxxx0_IOCFG1,   pRfSettings->IOCFG1);
	CC_WrReg(CCxxx0_IOCFG0,   pRfSettings->IOCFG0);
    CC_WrReg(CCxxx0_FSCTRL1,  pRfSettings->FSCTRL1);
    CC_WrReg(CCxxx0_FSCTRL0,  pRfSettings->FSCTRL0);
    CC_WrReg(CCxxx0_FREQ2,    pRfSettings->FREQ2);
    CC_WrReg(CCxxx0_FREQ1,    pRfSettings->FREQ1);
    CC_WrReg(CCxxx0_FREQ0,    pRfSettings->FREQ0);
    CC_WrReg(CCxxx0_MDMCFG4,  pRfSettings->MDMCFG4);
    CC_WrReg(CCxxx0_MDMCFG3,  pRfSettings->MDMCFG3);
    CC_WrReg(CCxxx0_MDMCFG2,  pRfSettings->MDMCFG2);
    CC_WrReg(CCxxx0_MDMCFG1,  pRfSettings->MDMCFG1);
    CC_WrReg(CCxxx0_MDMCFG0,  pRfSettings->MDMCFG0);
    CC_WrReg(CCxxx0_CHANNR,   pRfSettings->CHANNR);
    CC_WrReg(CCxxx0_DEVIATN,  pRfSettings->DEVIATN);
    CC_WrReg(CCxxx0_FREND1,   pRfSettings->FREND1);
    CC_WrReg(CCxxx0_FREND0,   pRfSettings->FREND0);
    CC_WrReg(CCxxx0_MCSM0 ,   pRfSettings->MCSM0 );
    CC_WrReg(CCxxx0_FOCCFG,   pRfSettings->FOCCFG);
    CC_WrReg(CCxxx0_BSCFG,    pRfSettings->BSCFG);
    CC_WrReg(CCxxx0_AGCCTRL2, pRfSettings->AGCCTRL2);
	CC_WrReg(CCxxx0_AGCCTRL1, pRfSettings->AGCCTRL1);
    CC_WrReg(CCxxx0_AGCCTRL0, pRfSettings->AGCCTRL0);    
    CC_WrReg(CCxxx0_PKTCTRL1, pRfSettings->PKTCTRL1);
    CC_WrReg(CCxxx0_PKTCTRL0, pRfSettings->PKTCTRL0);
    CC_WrReg(CCxxx0_ADDR,     pRfSettings->ADDR);
    CC_WrReg(CCxxx0_PKTLEN,   pRfSettings->PKTLEN);
    CC_WrReg(CCxxx0_FSCAL3,   pRfSettings->FSCAL3);
    CC_WrReg(CCxxx0_FSCAL2,   pRfSettings->FSCAL2);
	CC_WrReg(CCxxx0_FSCAL1,   pRfSettings->FSCAL1);
    CC_WrReg(CCxxx0_FSCAL0,   pRfSettings->FSCAL0);
	
    /*CC_WrReg(CCxxx0_FSTEST,   pRfSettings->FSTEST);
    CC_WrReg(CCxxx0_TEST2,    pRfSettings->TEST2);
    CC_WrReg(CCxxx0_TEST1,    pRfSettings->TEST1);
    CC_WrReg(CCxxx0_TEST0,    pRfSettings->TEST0);*/
}
//****************************************************************************************
//设置射频功率
void CC_PaTable(BYTE paTable)
{
  CC_WrReg(CCxxx0_PATABLE, paTable);
}
//****************************************************************************************
//设置频道
void CC_Chan(BYTE Chan)
{
  CC_Idle();//为方便
  CC_WrReg(CCxxx0_CHANNR,Chan);
}
//****************************************************************************************
//打开或关闭FEC前向纠错
void CC_FEC(BYTE On)
{
	BYTE Reg;
  Reg=CC_RdReg(CCxxx0_MDMCFG1);
  if (On==1) 
     CC_WrReg(CCxxx0_MDMCFG1, Reg | 0x80);
  else
  	 CC_WrReg(CCxxx0_MDMCFG1, Reg & 0x7F);
}
//*****************************************************************************************
//打开或关闭WHITE数据功能（使数据01均衡）
void CC_WHITE(BYTE On)
{
	BYTE Reg;
  Reg=CC_RdReg(CCxxx0_PKTCTRL0);
  if (On==1) 
     CC_WrReg(CCxxx0_PKTCTRL0, Reg | 0x40);
  else
  	 CC_WrReg(CCxxx0_PKTCTRL0, Reg & 0xBF);
}


//****************************************************************************************
//清除接收缓冲区和接收错误相关标志
void CC_ClrRx() 
{
    CC_Idle();//!!必须在Idle状态
	CC_Cmd(CCxxx0_SFRX);
}	  

//**********************************************************************************************
//清除发送缓冲区和发送错误相关标志
void  CC_ClrTx() 	
{
  CC_Idle();//!!必须在Idle状态
  CC_Cmd(CCxxx0_SFTX);
}

//**********************************************************************************
//无线模块初始化
static void CC_Init()
{
  //CLI();
  CC_RESET();
  //CC_RfConfig(&rfSettings7680);
  CC_RfConfig(&rfSettings250k);
  //CC_RfConfig(&rfSettings1200);
 // CC_WrReg(CCxxx0_SYNC1,0xa4 );//缺省值是D391
 // CC_WrReg(CCxxx0_SYNC0,0x25 );
  //CC_WrReg(CCxxx0_SYNC1,0x08 );//缺省值是D391
  //CC_WrReg(CCxxx0_SYNC0,0x00 );
  //CC_WrReg(CCxxx0_SYNC1,Sync0 );
  //CC_WrReg(CCxxx0_SYNC0,Sync1 );

  CC_PaTable(PAMAX);

  // CC_WrReg(CCxxx0_MCSM1,0x00 );//0x0f取消CCA，收发总回到RX 不能，否则不能自动校正频率
  //CC_WrReg(CCxxx0_MCSM1,0x30 );//复位值0x30 有CCA，收发回IDLE
 // CC_WHITE(1);
 
  //CC_FEC(1);
  //rt_thread_delay(RT_TICK_PER_SECOND/100*20);
  
  //CC_RxOn();
  //GIFR=0xE0;//clr int
  
 
  //SEI();
}



//**************************************************************************************

//无线模块功能测试，硬件调试用
void CC_Test()
{
  BYTE i=0;
  RF433_CS_DN;
  CC_Rdy();
  RF433_CS_UP;
  printf("cc2500 module ok\n");
  i=CC_RdStatus(CCxxx0_VERSION);//0x03
  printf("cc2500 version %x\n",i);
  /*i=CC_RdReg(CCxxx0_PKTCTRL0);//0x05
  printf("get cc2500 pktctrl0 %x\n",i);
  i=CC_RdReg(CCxxx0_PKTLEN);//0xff
  printf("get cc2500 pktlen %x\n",i);
  i=CC_GetStatus();*/
  
}
//**************************************************************************************
//发送数据包，缓冲区中第1字节是长度
void CC_SendPacket(uchar *txBuffer, uchar size) 
{

    //TCNT1=0;	
	//CC_Idle();//不能直接从RX状态到TX状态?可以，但约1/10概率没有切换到TX状态－－－CCA的原因？
    /*i=(CC_GetStatus()>>4);
	if (i>=2)
	{ CC_Idle();
	}*/
	
	CC_ClrTx();//v1.1保证TxBYTES无以前字节
	
    CC_WrReg(CCxxx0_TXFIFO, size);//len
	CC_WrRegs(CCxxx0_TXFIFO, txBuffer, size);//***
	myRxMode = 0;
	CC_Cmd(CCxxx0_STX);
	
 //RfT=TCNT1;//测试耗时
	//bSendOk=0;
	//reset_rf=2;b_rf_check=0;//40ms
}
#define MAXLEN 16
#define cc_printf(tbuf,tlen)	do{\
								int i = 0;\
								for ( i = 0 ; i<tlen ;i++){\
									rt_kprintf("%02x ",tbuf[i]);\
								}\
								rt_kprintf("\n");\
							}while(0)
//*************************************************************************************************
//读出接收的数据，和LQI,RSSI，并判断CrcOk
void CC_RdPacket()
{
/*
    uchar iLen;
	 //TCNT1=0;
	 //iLen=CC_RdStatus(CCxxx0_RXBYTES) & 0x7f;//read RxFIFO Nums
	 //FIFO中总数据字节，动态变化
     
	 // Read length byte
     iLen= CC_RdReg(CCxxx0_RXFIFO);//第一字节是Len 数据包长度
     if (iLen==0) goto RXERR; 
	 if (iLen>MAXLEN) goto RXERR;//iLen=MAXLEN;
	 
     // Read data from RX FIFO and store in rxBuffer
	 SerData[0]=iLen;    //数据包长度
   
     CC_RdRegs(CCxxx0_RXFIFO, SerData+1, iLen); //读数据
	 printf("get Pkg:");
   	 cc_printf(SerData,iLen+1);
     // Read the 2 appended status bytes (status[0] = RSSI, status[1] = LQI)
     CC_RdReg(CCxxx0_RXFIFO);  //RSSI
     CC_RdReg(CCxxx0_RXFIFO);	//LQI
	 
	 iLen=CC_RdStatus(CCxxx0_RXBYTES) & 0x7f;
	 //RfT=TCNT1;//测试耗时
	 if (iLen==0) 
		 return;
	 
RXERR:	 //应该为0，不为0就不对，要清除RxFiFo
	 CC_ClrRx();
	 //iLen=CC_RdStatus(CCxxx0_RXBYTES) & 0x7f;
	 */	 
}			
//*********************************************************************************************
//把信号强度RSSI值转换成dB值
BYTE CC_RssiCh(BYTE rssi)
{//输出值是正值，但都是负的dBm，例如返回值是55是-55dBm
  if (rssi>=128)
  { 
	  return (128+RSSI0-(rssi>>1));
  }
  else
  { 
	  return (RSSI0-(rssi>>1));
  }
}

//*************************************************************************************
//读信号强度RSSI值
BYTE CC_Rssi()
{
  return CC_RdStatus(CCxxx0_RSSI);
}


//**************************************************************************************
//读打包状态寄存器
BYTE CC_PackStatus()
{
  return CC_RdStatus(CCxxx0_PKTSTATUS);
  //bit0-bit7 GDO0,GDO1,GDO2,SYNC,  CCA,PQT,CS,CRCOK
  //如果MCSM1.CCA＝0没有使用CCA的话，CCA指示位总为1，
  //如果使用CCA，CCA和CS位就相反
}
//***********************************************************************************
//用CCA判断是否有信道冲突
BYTE CC_CheckCCA()
{
	BYTE i;
  
	i=CC_RdStatus(CCxxx0_PKTSTATUS);
  //bit0-bit7 GDO0,GDO1,GDO2,SYNC,  CCA,PQT,CS,CRCOK
  //如果MCSM1.CCA＝0没有使用CCA的话，CCA指示位总为1－－必须使能CCA!!
  //如果使用CCA，CCA和CS位就相反
  //if (CHK(i,6)==0)//使用CS指示，效果相同
//  if CHK(i,4) //使用CCA指示
  if ((i & 4) ==4)
    return 1;
  else
    return 0;
}
//***************************************************************************************
//检测无线芯片是否死机
void RfChk()
{
   BYTE i;
   //CLI();
   i=(CC_GetStatus()>>4);
   if (i!=1)//0IDLE 1RX 2TX 3FSTXON 4CALI 5SETT 6RX_OVER 7TX_UNDER
   { 
	  CC_Idle();
	  Delay_us(100);
	  CC_Cmd(CCxxx0_SFRX);
	  Delay_us(100);
	  CC_Cmd(CCxxx0_SFTX);
	  Delay_us(100);
	  CC_RxOn();
	}	
   //SEI();
}
//***************************************************************************************
//测试不同状态下某些寄存器的值，调试用
long ChkRfState(int RfRunNo)
{
	volatile BYTE i;

   switch (RfRunNo)
   {
     case 1:
	 	  i=CC_GetStatus();
		  //i=CC_RdReg(CCxxx0_MCSM1);
		  break;
	 case 2:
	 	  i=CC_RdStatus(CCxxx0_RXBYTES) & 0x7f;	  
		  i= CC_RdReg(CCxxx0_RXFIFO);
		  break;
	 case 3:
	 	  i=CC_RdStatus(CCxxx0_TXBYTES) & 0x7f;	  
		  CC_ClrTx();
		  i=CC_RdStatus(CCxxx0_TXBYTES) & 0x7f;
		  CC_RxOn();
	  
		  break;
	 case 4:
	 	  i=CC_RdStatus(CCxxx0_RXBYTES) & 0x7f;	  
		  CC_ClrRx();
		  i=CC_RdStatus(CCxxx0_RXBYTES) & 0x7f;
		  CC_RxOn();
		  break;	  	  
	 default:
	 	  break;	 	  
   }
   return i;
}
//******************************************************************************
//修改同步码
void CC_SyncNew(int Sync0,int Sync1)
{ 
 // CLI();
  CC_WrReg(CCxxx0_SYNC1,Sync0 );//
  CC_WrReg(CCxxx0_SYNC0,Sync1 );
 // SEI();
}

//==============================================================================
void RfOff()
{
  CC_Cmd(0x36);//IDLE 必须先IDLE才能进入SLEEP
  CC_Cmd(0x39);//Rf PwrDown
  
  //CLR(GICR,INT0);//!!关闭int0
  //PORTD &= 0xfb;  //!!休眠时候RFGDO0脚输出低电平，INT0脚不要设置上拉，否则耗电100uA

}
rt_inline long cc_res()
{
	CC_RESET();
 	return 0;
}
rt_inline long cc_clrrx()
{
	CC_ClrRx();
 	return 0;
}
rt_inline long cc_clrtx()
{
 	CC_ClrTx();
	return 0;
}
rt_inline long cc_rx()
{
	CC_Idle();//!!必须在Idle状态
	CC_Cmd(CCxxx0_SFRX);
	myRxMode =1;
 	CC_RxOn();
	return 0;
}
rt_inline long cc_tx()
{
	CC_TxOn();
	myRxMode =0;
 	return 0;
}
rt_inline long cc_idle()
{
	CC_Idle();
 	return 0;
}

void cc2500_parse_pkg(unsigned char * buffer,int len)
{
	int pdulen = buffer[0];
	int i = 0;
	unsigned char hasAS=0;	//has append status bytes
	if(len==0)
		return ;
	if(len > pdulen)
		hasAS = 1;
	printf("pkg PDU len %d",buffer[0]);
	printf(", target addr %x,",buffer[1]);
	printf("data:");
	for(i = 2 ; i < pdulen+1 ; i++){
		printf(" %02x",buffer[i]);
	}
	if(hasAS){
		printf(",RSSI:%02x,LQI:%2x,",buffer[i++],buffer[i+1]);
		if(buffer[i]&0x80){
		 	printf("CRC Check ok\n");
		}
		else
			printf("!!!CRC Check failed!!!\n");
	}
	else
		printf("\n");
}

static void cc2500_gpio_init(void)
{
#if 0
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable GPIO clocks */
	/*!< SPI_CC2500_CSN_GPIO_CLK, SPI_CC2500_CE_GPIO_CLK and  CC2500_IRQ_GPIO_CLK Periph clock enable,
	----add SPI_CC2500_SCK_GPIO_CLK,SPI_CC2500_MOSI_GPIO_CLK,SPI_CC2500_MISO_GPIO_CLK 
	----add SPI_NRF2_CE_GPIO_CLK SPI_NRF2_CSN_GPIO_CLK SPI_NRF2_IRQ_GPIO_CLK */
	RCC_APB2PeriphClockCmd(CC2500_CSN_CLK | CC2500_SCK_CLK| CC2500_MOSI_CLK| CC2500_MISO_CLK |
				 CC2500_GDO2_CLK | CC2500_GDO0_CLK , ENABLE);

	/*!< Configure cc2500 CSN pin */
	GPIO_InitStructure.GPIO_Pin = CC2500_CSN_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(CC2500_CSN_PORT, &GPIO_InitStructure);
	
	/*!< Configure cc2500 SCK pins */
	GPIO_InitStructure.GPIO_Pin = CC2500_SCK_PIN;
	GPIO_Init(CC2500_SCK_PORT, &GPIO_InitStructure);

	/*!< Configure CC2500_MOSI_PIN pin */
	GPIO_InitStructure.GPIO_Pin = CC2500_MOSI_PIN;
	GPIO_Init(CC2500_MOSI_PORT, &GPIO_InitStructure);
	
	/*!< Configure CC2500_GDO2_PIN pin  */
	GPIO_InitStructure.GPIO_Pin = CC2500_GDO2_PIN;
	GPIO_Init(CC2500_GDO2_PORT, &GPIO_InitStructure);

	/*!< Configure CC2500_MISO_PIN pin */
	GPIO_InitStructure.GPIO_Pin = CC2500_MISO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(CC2500_MISO_PORT, &GPIO_InitStructure);

		
	/*!< Configure CC2500_GDO0_PIN pin: input clock */
	GPIO_InitStructure.GPIO_Pin = CC2500_GDO0_PIN;
	GPIO_Init(CC2500_GDO0_PORT, &GPIO_InitStructure);
	
	/* Deselect the cc2500: Chip Select high */
	//CC2500_CSN_HIGH();
	CC2500_HIGH(CSN);
	//CC2500_SCK_LOW();
	CC2500_LOW(SCK);
#endif
}
void cc2500_gpio_init(void)
{
	PINSEL_ConfigPin(CC2500_CSN_PORT,CC2500_CSN_PIN,0);
	GPIO_SetDir(CC2500_CSN_PORT,CC2500_CSN_MASK,GPIO_DIRECTION_OUTPUT);
	PINSEL_SetPinMode(CC2500_CSN_PORT,CC2500_CSN_PIN,IOCON_MODE_PLAIN);

	PINSEL_ConfigPin(CC2500_SCK_PORT,CC2500_SCK_PIN,0);
	GPIO_SetDir(CC2500_SCK_PORT,CC2500_SCK_MASK,GPIO_DIRECTION_OUTPUT);
	PINSEL_SetPinMode(CC2500_SCK_PORT,CC2500_SCK_PIN,IOCON_MODE_PLAIN);

	PINSEL_ConfigPin(CC2500_MOSI_PORT,CC2500_MOSI_PIN,0);
	GPIO_SetDir(CC2500_MOSI_PORT,CC2500_MOSI_MASK,GPIO_DIRECTION_OUTPUT);
	PINSEL_SetPinMode(CC2500_MOSI_PORT,CC2500_MOSI_PIN,IOCON_MODE_PLAIN);

	PINSEL_ConfigPin(CC2500_GDO2_PORT,CC2500_GDO2_PIN,0);
	GPIO_SetDir(CC2500_GDO2_PORT,CC2500_GDO2_MASK,GPIO_DIRECTION_OUTPUT);
	PINSEL_SetPinMode(CC2500_GDO2_PORT,CC2500_GDO2_PIN,IOCON_MODE_PLAIN);

	PINSEL_ConfigPin(CC2500_MISO_PORT,CC2500_MISO_PIN,0);
	GPIO_SetDir(CC2500_MISO_PORT,CC2500_MISO_MASK,GPIO_DIRECTION_INPUT);
	PINSEL_SetPinMode(CC2500_MISO_PORT,CC2500_MISO_PIN,IOCON_MODE_PLAIN);

	PINSEL_ConfigPin(CC2500_GDO0_PORT,CC2500_GDO0_PIN,0);
	GPIO_SetDir(CC2500_GDO0_PORT,CC2500_GDO0_MASK,GPIO_DIRECTION_INPUT);
	PINSEL_SetPinMode(CC2500_GDO0_PORT,CC2500_GDO0_PIN,IOCON_MODE_PLAIN);

	GPIO_OutputValue(CC2500_CSN_PORT,CC2500_CSN_MASK,1);
	GPIO_OutputValue(CC2500_CLK_PORT,CC2500_CLK_MASK,0);
}

static void cc2500_exti_init(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;
    /* Configure Button EXTI line */
    
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;	//以上三项设置为公用设置

	/* Connect  EXTI Line15 to  GPIOE__Pin15 */
    GPIO_EXTILineConfig(CC2500_IRQ_EXTI_PORT_SOURCE, CC2500_GDO0_IRQ_EXTI_PIN_SOURCE);  
	 
	 /* Configure Button EXTI line */
    EXTI_InitStructure.EXTI_Line = CC2500_GDO0_EXTI_LINE;
	EXTI_ClearITPendingBit(CC2500_GDO0_EXTI_LINE);
    EXTI_Init(&EXTI_InitStructure);
	
	/* Connect  EXTI Line13 to  GPIOE__Pin13 */
    //GPIO_EXTILineConfig(CC2500_IRQ_EXTI_PORT_SOURCE, CC2500_GDO2_IRQ_EXTI_PIN_SOURCE); 
	 /* Configure Button EXTI line */
    //EXTI_InitStructure.EXTI_Line = CC2500_GDO2_EXTI_LINE;
	//EXTI_ClearITPendingBit(CC2500_GDO2_EXTI_LINE);
    //EXTI_Init(&EXTI_InitStructure);
	
}

/*******************************************************************************
* Function Name  : cc2500_nvic_init
* Description    : Configures the used IRQ Channels and sets their priority.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static void cc2500_nvic_init(void)
{ 
	NVIC_InitTypeDef NVIC_InitStructure;
	
	/* Set the Vector Table base address at 0x08000000 */
	//NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0000);
	
	/* Configure the Priority Group to 2 bits */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	
	/* Enable the EXTI Interrupt for NRF_SEND */
	NVIC_InitStructure.NVIC_IRQChannel = CC2500_IRQ_EXTI_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

}

int cc2500_read_buf(unsigned char * buf,int buflen)
{
	unsigned char state = CC_GetStatus();


	int len = state & 0xf;
	int truelen = len;
	int i=0;
#if 1
	printf("cc2500 %s State %s FIFO %02d\n",(state & 0x80)?" Not Ready ":" Is Ready ",
				sStatus[(state & 0x70)>>4],state & 0xF );
#endif
	if(len <= 0)   //CRC校验失败或其他原因导致RX FIFO是空的
		return 0;
	
	truelen = CC_RdReg(CCxxx0_RXFIFO);
	if(truelen>SELFCC25MAXLEN-1)
		truelen = SELFCC25MAXLEN-1;
	if(len>=15 && truelen+3 >=len)
		buf[i++]=truelen+3;
	else
		buf[i++]=len-3;
#if 1
	while(i<len+3 && i < SELFCC25MAXLEN){
		buf[i++]=CC_RdReg(CCxxx0_RXFIFO);
	}
#else
	CC_RdRegs(CCxxx0_RXFIFO,buf+i,len+3-i);
#endif		

	return len+1;
}

void cc2500_consumer_thread(void * paratmer)
{
	unsigned char * pbuf = RT_NULL;
	while(rt_mb_recv(&cc2500recvmailbox,(rt_uint32_t *)&pbuf,RT_WAITING_FOREVER) == RT_EOK){
		cc2500_parse_pkg(pbuf,pbuf[0]+3);	
		rt_free(pbuf);
	}
	printf("cc2500 consumer thread run faled!!!!!");
	reboot(); 
}

void cc2500_daemon_rthread(void * parameter)
{
  rt_uint32_t e;
  unsigned char * tmpbuf = RT_NULL;
  cc_rx();
  while(1){
	if(rt_event_recv(&globalevent,(CS_EVENT_CC25RX|CS_EVENT_CC25TX|CS_EVENT_CC25TO),RT_EVENT_FLAG_OR|RT_EVENT_FLAG_CLEAR,
						RT_WAITING_FOREVER,&e)==RT_EOK){
						//RT_TICK_PER_SECOND * 10,&e)==RT_EOK){
		if(e&CS_EVENT_CC25RX){  //2.4GHz有数据
			tmpbuf = rt_malloc(SELFCC25MAXLEN);
			if(tmpbuf == RT_NULL){
				unsigned char dbuf[SELFCC25MAXLEN]={0};
				cc2500_read_buf(dbuf,SELFCC25MAXLEN); //读取数据
			}
			else{
				if(cc2500_read_buf(tmpbuf,SELFCC25MAXLEN)>0) //读取数据			
					rt_mb_send(&cc2500recvmailbox,(rt_uint32_t)tmpbuf);
				else
					rt_free(tmpbuf);
			}
		  	cc_rx();
		}
		if(e&CS_EVENT_CC25TX){
			rt_mb_recv(&cc2500sendmailbox,(rt_uint32_t *)&tmpbuf,0); 
			if(tmpbuf!=RT_NULL){
				CC_SendPacket(tmpbuf+1,tmpbuf[0]);	 	
				rt_free(tmpbuf);
			}
			//cc_rx();
		}
		if(e&CS_EVENT_CC25TO){
		 	printf("cc2500 tx ok!\n");
			cc_rx();
		} 
			
	}
	else
	{
		rt_kprintf("cc2500 recv reset\n");
		cc_rx();	
	}
  }
}

/*******************************************************************************
* Function Name  : cc2500_init
* Description    : Initializes the peripherals used by the SPI FLASH driver.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void cc2500_init(void)
{
#if NEEDSPI1
	SPI_Config();
#endif
	
	rt_mb_init(&cc2500recvmailbox,"c2rmb",cc2500rx_buf,sizeof(cc2500rx_buf)/4,RT_IPC_FLAG_FIFO);
	rt_mb_init(&cc2500sendmailbox,"c2smb",cc2500tx_buf,sizeof(cc2500tx_buf)/4,RT_IPC_FLAG_FIFO);	
	
	cc2500_gpio_init();	
	CC_Test();	
	CC_Init();
	
	cc2500_exti_init();
	cc2500_nvic_init();
	{
		rt_thread_t cc2500_thread = rt_thread_create("cc2500",cc2500_daemon_rthread,RT_NULL,1024,8,21);
		if(cc2500_thread != RT_NULL){
		 	rt_thread_startup(cc2500_thread);
		}
		else
			rt_kprintf("create cc2500 daemon thread failed\n");
	}
	{
	 	rt_thread_t cc2500_consumer = rt_thread_create("c25c",cc2500_consumer_thread,RT_NULL,1024,8,21);
		if(cc2500_consumer != RT_NULL){
		 	rt_thread_startup(cc2500_consumer);
		}
		else
			rt_kprintf("create cc2500 consumer thread failed\n");
	}
	  
}

void EXTI15_10_IRQHandler(void)	 
{
	/*
	Author:
	Time:
	Function:
	*/				  
	static int a=0;
    rt_interrupt_enter();
	if(a==0){
	 	rt_hw_led_on(3);
		a=1;
	}
	else{
		a=0;
		rt_hw_led_off(3);
	}
	if(EXTI_GetITStatus(CC2500_GDO0_EXTI_LINE) != RESET)
  	{
		//EXTI->IMR &= ~CC2500_EXTI_LINE;
		//while(
		//printf("cc2500 int %d\n",GPIO_ReadInputDataBit(CC2500_GDO0_PORT,CC2500_GDO0_PIN));
		if(myRxMode)
			rt_event_send(&globalevent,CS_EVENT_CC25RX);
		else
			rt_event_send(&globalevent,CS_EVENT_CC25TO);
		EXTI_ClearITPendingBit(CC2500_GDO0_EXTI_LINE);
	}
	if(EXTI_GetITStatus(EXTI_Line10) != RESET)
  	{
		EXTI->IMR &= ~EXTI_Line10;
	
		EXTI_ClearITPendingBit(EXTI_Line10);
	}
  	if(EXTI_GetITStatus(EXTI_Line11) != RESET)
  	{  
		//rt_event_send(&globalevent,CS_EVENT_NRF905_DR);
		
		EXTI_ClearITPendingBit(EXTI_Line11);
  	}
	if(EXTI_GetITStatus(CC2500_GDO2_EXTI_LINE) != RESET)
	{
	  //EXTI->IMR &= ~EXTI_Line13;
	  
	  EXTI_ClearITPendingBit(CC2500_GDO2_EXTI_LINE);
	}
	if(EXTI_GetITStatus(EXTI_Line14) != RESET)
	{
	  EXTI->IMR &= ~EXTI_Line14;
	  EXTI_ClearITPendingBit(EXTI_Line14);
	}
	if(EXTI_GetITStatus(EXTI_Line12) != RESET)
	{
	  EXTI->IMR &= ~EXTI_Line12;
	  EXTI_ClearITPendingBit(EXTI_Line12);
	}
	rt_interrupt_leave();
}

#ifdef RT_USING_FINSH
#include <finsh.h>
long cc_st()
{	
	unsigned char tmpbuf[16]={0};
	unsigned char buffer[64]={0};
	int i = 0,len=0 ;
	
	unsigned char state = CC_GetStatus();
	printf("cc2500 %s State %s FIFO %02d\n",(state & 0x80)?" Not Ready ":" Is Ready ",
				sStatus[(state & 0x70)>>4],state & 0xF );

	len = state & 0xf;
#if 1
	i=0;
	while(i<len){
		buffer[i++]=CC_RdReg(CCxxx0_RXFIFO);
	}	
	cc2500_parse_pkg(buffer,len);
#else
	if(i > 0){
		printf("rx fifo has %d data:",i);
		while(i-->0){
		 	printf(" %x",CC_RdReg(CCxxx0_RXFIFO));
		}
		printf("\n");
	}
#endif
		
	for(i = 0 ;i < 14;i++){
		CC_RdRegs(CCxxx0_PARTNUM+i,tmpbuf+i,1);
		printf("%s %02X\n",sRegNs[i],tmpbuf[i]);
	}
	
	return 0;
}
long cc_reg()
{
	int i = 0 ;
	BYTE Data=0;   
    
	RF433_CS_DN;
	CC_Rdy();
	Data=CC_SpiWr(0xC0);//
	
	for(i=0 ;i<0x30;i++){
		Data=CC_SpiWr(0);
		printf("reg 0x%02x value 0x%02x\n",i,Data);
	}
	RF433_CS_UP;
	WAIT;
 	return 0;
}

long cc_sd(int data)
{
	unsigned char * pbuf = (unsigned char *)rt_malloc(3);
	pbuf[0]=2;
	pbuf[1]=0;
	pbuf[2]=data&0xff;
	rt_mb_send(&cc2500sendmailbox,(rt_uint32_t)pbuf);
	rt_event_send(&globalevent,	CS_EVENT_CC25TX);
 	return 0;	
}


long cc_recv(int len)
{
	CC_RdPacket();
	return 0;
}
FINSH_FUNCTION_EXPORT(cc_st, get cc2500 state)
FINSH_FUNCTION_EXPORT(cc_reg, get cc2500 reg)
FINSH_FUNCTION_EXPORT(cc_clrrx, cc2500 test)
FINSH_FUNCTION_EXPORT(cc_clrtx, cc2500 test)
FINSH_FUNCTION_EXPORT(cc_rx, cc2500 test)
FINSH_FUNCTION_EXPORT(cc_tx, cc2500 test)
FINSH_FUNCTION_EXPORT(cc_res, cc2500 test)
FINSH_FUNCTION_EXPORT(cc_idle, cc2500 test)	 
FINSH_FUNCTION_EXPORT(cc_recv, cc2500 test)
FINSH_FUNCTION_EXPORT(cc_sd, cc2500 test)
#endif

/*
<p>//头文件为MDK 4.23带的"LPC177x_8x.h",代码中所用宏均在其中可以找到定义  </p><p>#include "LPC177x_8x.h"  </p><p> </p><p>  
LPC_IOCON->P2_10 = (LPC_IOCON->P2_10 & ~0x07) | 1;  //把P2.10设为EINT0  </p><p>  
LPC_SC->EXTMODE &= ~0x01;  //中断为电平触发    
</p><p>LPC_SC->EXTPOLAR &- ~0x01; //低电平触发中断    
</p><p>LPC_SC->EXTINT |= 1； //清空下EINT0中断标记  </p><p> </p><p> </p><p> </p><p>NVIC_EnableIRQ（EINT0_IRQn）; //使能EINT0中断   </p><p> </p><p>-----------------------------------------------------------------------------------------------</p><p> </p><p>//EINT0中断处理函数  </p><p>  
void EINT0_IRQHandler(void){  </p><p>  
 //在这里添加你要的中断处理代码  </p><p>LPC_SC->EXTINT |= 1; //中断处理完毕，清空中断标志  </p><p>}</p>
*/
