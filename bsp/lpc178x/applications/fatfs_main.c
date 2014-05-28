/*---------------------------------------------------------------------------*/
/* FAT file system sample project for FatFs (LPC17xx)    (C)ChaN, NXP, 2010  */
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <rtthread.h>
#include "LPC177x_8x.h"
#include "system_LPC177x_8x.h"
#include "lpc177x_8x_uart.h"
#include "lpc177x_8x_rtc.h"
#include "monitor.h"
//#include "integer.h"
//#include "diskio.h"
//#include "ff.h"
#include "debug_frmwrk.h"
#include "lpc177x_8x_mci.h"
#define BUFFER_SIZE     4096
/** @defgroup MCI_FatFS	MCI FatFS 
 * @ingroup MCI_FS
 * Refer to @link Examples/MCI/Mci_FS/FatFs/doc/00index_e.htm @endlink
 * @{
 */
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned char BYTE;
typedef unsigned int UINT;
	
char debugBuf[64];
#if _USE_XSTRFUNC==0
#include <string.h>
#define xstrlen(x)      strlen(x)
#define xstrcpy(x,y)    strcpy(x,y)
#define xmemset(x,y,z)  memset(x,y,z)
#define xstrchr(x,y)    strchr(x,y)
#endif

/* LPC Definitions */
#define _LPC_RTC		(LPC_RTC)


/* buffer size (in byte) for R/W operations */
#define BUFFER_SIZE     4096 

DWORD acc_size;				/* Work register for fs command */
WORD acc_files, acc_dirs;

#if _USE_LFN
char Lfname[_MAX_LFN+1];
#endif

char Line[128];				/* Console input buffer */


/* Increase the buff size will get faster R/W speed. */
#if 1 /* use local SRAM */
#ifdef __IAR_SYSTEMS_ICC__
#pragma data_alignment=4
static BYTE Buff[BUFFER_SIZE] ;		/* Working buffer */
#else
static BYTE Buff[BUFFER_SIZE] __attribute__ ((aligned (4))) ;		/* Working buffer */
#endif
static UINT blen = sizeof(Buff);
#else   /* use 16kB SRAM on AHB0 */
static BYTE *Buff = (BYTE *)0x20000000;	 
static UINT blen = 16*1024;
#endif

volatile UINT Timer = 0;		/* Performance timer (1kHz increment) */


#if 0
/* SysTick Interrupt Handler (1ms)    */
void SysTick_Handler (void) 
{           
	static uint32_t div10;

	Timer++;

	if (++div10 >= 10) {
		div10 = 0;
		disk_timerproc();		/* Disk timer function (100Hz) */
	}
}
#endif

/* Get RTC time */
void rtc_gettime (RTC_TIME_Type *rtc)
{
	RTC_GetFullTime( _LPC_RTC, rtc);	
}

/* Set RTC time */
void rtc_settime (const RTC_TIME_Type *rtc)
{
	/* Stop RTC */
	RTC_Cmd(_LPC_RTC, DISABLE);

	/* Update RTC registers */
	RTC_SetFullTime (_LPC_RTC, (RTC_TIME_Type *)rtc);

	/* Start RTC */
	RTC_Cmd(_LPC_RTC, ENABLE);
}

/*---------------------------------------------------------*/
/* User Provided Timer Function for FatFs module           */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support a real time clock.          */
/* This is not required in read-only configuration.        */
DWORD get_fattime ()
{
	RTC_TIME_Type rtc;

	/* Get local time */
	rtc_gettime(&rtc);

	/* Pack date and time into a DWORD variable */
	return	  ((DWORD)(rtc.YEAR - 1980) << 25)
			| ((DWORD)rtc.MONTH << 21)
			| ((DWORD)rtc.DOM << 16)
			| ((DWORD)rtc.HOUR << 11)
			| ((DWORD)rtc.MIN<< 5)
			| ((DWORD)rtc.SEC>> 1);

}




void put_char(unsigned char ch)
{
    _DBC(ch);    
}
unsigned char get_char(void)
{
   return _DG;    
}
static void IoInit(void) 
{
	RTC_TIME_Type  current_time;

	SysTick_Config(SystemCoreClock/1000 - 1); /* Generate interrupt each 1 ms   */

	RTC_Init(_LPC_RTC);
	current_time.SEC = 0;
	current_time.MIN = 0;
	current_time.HOUR= 0;
	current_time.DOM= 1;
	current_time.DOW= 0;
	current_time.DOY= 0;		/* current date 01/01/2010 */
	current_time.MONTH= 1;
	current_time.YEAR= 2010;
	RTC_SetFullTime (_LPC_RTC, &current_time);		/* Set local time */
	RTC_Cmd(_LPC_RTC,ENABLE);

	debug_frmwrk_init();	
    xfunc_out = put_char;
//	xfunc_in  = get_char;    	
}
void print_commands()
{
    xputs("Commands: \n");
    xputs("          dd <phy_drv#> [<sector>] - Dump secrtor\n");
    xputs("          di <phy_drv#> - Initialize disk\n");
    xputs("          ds <phy_drv#> - Show disk status\n");
    xputs("          bd <addr> - Dump R/W buffer \n");
    xputs("          be <addr> [<data>] ... - Edit R/W buffer \n");
    xputs("          br <phy_drv#> <sector> [<n>] - Read disk into R/W buffer \n");
    xputs("          bw <phy_drv#> <sector> [<n>] - Write R/W buffer into disk \n");
    xputs("          bf <val> - Fill working buffer  \n");
    xputs("          fi <log drv#> - Initialize logical drive   \n");
    xputs("          fs - Show logical drive status   \n");
    xputs("          fl [<path>] - Directory listing   \n");
    xputs("          fo <mode> <file> - Open a file    \n");
    xputs("          fc - Close a file    \n");
    xputs("          fe - Seek file pointer    \n");
    xputs("          fd <len> - read and dump file from current fp    \n");
    xputs("          fr <len> - read file    \n");
    xputs("          fw <len> <val> - write file    \n");
    xputs("          fn <old_name> <new_name> - Change file/dir name     \n");
    xputs("          fu <name> - Unlink a file or dir     \n");
    xputs("          fv - Truncate file      \n");
    xputs("          fk <name> - Create a directory      \n");
    xputs("          fa <atrr> <mask> <name> - Change file/dir attribute       \n");
    xputs("          ft <year> <month> <day> <hour> <min> <sec> <name> - Change timestamp       \n");
    xputs("          fx <src_name> <dst_name> - Copy file       \n");
    xputs("          fg <path> - Change current directory       \n");
    #if _USE_MKFS
    xputs("          fm <partition rule> <cluster size> - Create file system       \n");
    #endif
    xputs("          fz [<rw size>] - Change R/W length for fr/fw/fx command       \n");
    xputs("          t [<year> <mon> <mday> <hour> <min> <sec>]        \n");
}

/*-----------------------------------------------------------------------*/
/* Program Main                                                          */
/*-----------------------------------------------------------------------*/
int main ()
{

	UINT br;
    uint8_t *ptr;
	UINT bw,i;
	uint8_t secbuf[512]={0};
			/* File objects */
	register rt_base_t level = 		rt_hw_interrupt_disable();
			//rt_hw_board_init();
	IoInit();
    //xprintf("%s",mciFsMenu);
//rt_hw_sdcard_init();
rt_hw_interrupt_enable(level);
if(sd_initialize() != 0 )
	{

		while(1);
	}
	else
	{
	  //xprintf("INIT ok\r\n");
	}
	my_disk_read(0,secbuf,0,1);
	xprintf("%02x %02x %02x\n",secbuf[0],secbuf[1],secbuf[2]);
	
	while(1);
}
/**
 * @}
 */
 

