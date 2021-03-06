/***
 * @file		fs_mci.c
 * @purpose		Drivers for SD
 * @version		1.0
 * @date		23. February. 2012
 * @author		NXP MCU SW Application Team
 *---------------------------------------------------------------------
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors'
 * relevant copyright in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 **********************************************************************/

#include "LPC177x_8x.h"
#include "lpc_types.h"
#include "lpc177x_8x_mci.h"
#include "lpc177x_8x_gpdma.h"
#include "lpc177x_8x_rtc.h"
#include "lpc177x_8x_clkpwr.h"
#include "fs_mci.h"
#include <dfs_fs.h>
#include <rtthread.h>

/* Example group ----------------------------------------------------------- */
/** @defgroup MCI_FS	MCI File System
 * @ingroup MCI_Examples
 * @{
 */
 
#define DMA_SIZE        (1000UL)
#define DMA_SRC			LPC_PERI_RAM_BASE		/* This is the area original data is stored
										or data to be written to the SD/MMC card. */
#define DMA_DST			(DMA_SRC+DMA_SIZE)		/* This is the area, after writing to the SD/MMC,
										data read from the SD/MMC card. */

/* treat WriteBlock as a constant address */
volatile uint8_t *WriteBlock = (uint8_t *)(DMA_SRC);

/* treat ReadBlock as a constant address */
volatile uint8_t *ReadBlock  = (uint8_t *)(DMA_DST);

/* Disk Status */
static volatile uint8_t Stat = STA_NOINIT;	

/* 100Hz decrement timer stopped at zero (disk_timerproc()) */
static volatile uint16_t Timer1, Timer2;	


/* Card Configuration */
CARDCONFIG CardConfig;      /* Card configuration */

#if MCI_DMA_ENABLED
/******************************************************************************
**  DMA Handler
******************************************************************************/
/*void DMA_IRQHandler (void)
{
   MCI_DMA_IRQHandler();
}*/
#endif
/*-----------------------------------------------------------------------*/
/* Get bits from an array
/----------------------------------------------------------------------*/
uint32_t unstuff_bits(uint8_t* resp,uint32_t start, uint32_t size)
{                                   
     uint32_t byte_idx_stx;                                        
     uint8_t bit_idx_stx, bit_idx;
     uint32_t ret, byte_idx;

     byte_idx_stx = start/8;
     bit_idx_stx = start - byte_idx_stx*8;

     if(size < (8 - bit_idx_stx))       // in 1 byte
     {
        return ((resp[byte_idx_stx] >> bit_idx_stx) & ((1<<size) - 1));        
     }

     ret = 0;
     
     ret =  (resp[byte_idx_stx] >> bit_idx_stx) & ((1<<(8 - bit_idx_stx)) - 1);
     bit_idx = 8 - bit_idx_stx;
     size -= bit_idx;

     byte_idx = 1;
     while(size > 8)
     {
        ret |= resp[byte_idx_stx + byte_idx] << (bit_idx);
        size -= 8;
        bit_idx += 8;
        byte_idx ++;
     }
     
    
     if(size > 0)
     {
        ret |= (resp[byte_idx_stx + byte_idx] & ((1<<size) - 1)) << bit_idx;
     }

     return ret;
}
/*-----------------------------------------------------------------------*/
/* Swap buffer
/----------------------------------------------------------------------*/
static void swap_buff(uint8_t* buff, uint32_t count)
{
    uint8_t tmp;
    uint32_t i;

    for(i = 0; i < count/2; i++)
    {
        tmp = buff[i];
        buff[i] = buff[count-i-1];
        buff[count-i-1] = tmp;
    }
}
/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                  */
/*-----------------------------------------------------------------------*/
uint8_t sd_initialize (

)
{
//	if (Stat & STA_NODISK) return Stat;	/* No card in the socket */

	/* Reset */
	Stat = STA_NOINIT;

#if MCI_DMA_ENABLED
	/* on DMA channel 0, source is memory, destination is MCI FIFO. */
	/* On DMA channel 1, source is MCI FIFO, destination is memory. */
	GPDMA_Init();
#endif

    /* For the SD card I tested, the minimum required block length is 512 */
	/* For MMC, the restriction is loose, due to the variety of SD and MMC
	card support, ideally, the driver should read CSD register to find the
	right speed and block length for the card, and set them accordingly.
	In this driver example, it will support both MMC and SD cards, and it
	does read the information by send SEND_CSD to poll the card status,
	however, to simplify the example, it doesn't configure them accordingly
	based on the CSD register value. This is not intended to support all
	the SD and MMC cards. */

	if(MCI_Init(BRD_MCI_POWERED_ACTIVE_LEVEL) != MCI_FUNC_OK)
	{
        return Stat;
	}
	

	if(mci_read_configuration() == TRUE)
	{
		Stat &= ~STA_NOINIT;
	}
	else
	{
		Stat |=  STA_NODISK;
	}
	return Stat;		
	
}
/**
  * @brief  Read card configuration and fill structure CardConfig.
  *
  * @param  None
  * @retval TRUE or FALSE. 
  */
Bool mci_read_configuration (void)
{
	uint32_t c_size, c_size_mult, read_bl_len;
    uint8_t csd_struct = 0;
	
	do
	{
		/* Get Card Type */
		CardConfig.CardType= MCI_GetCardType();
		if(CardConfig.CardType == MCI_CARD_UNKNOWN)
		{
			break;
		}

		/* Read OCR */
		/*if(MCI_ReadOCR(&CardConfig.OCR) != MCI_FUNC_OK)
		{
			break;
		} */
	    /* Read CID */
		if (MCI_GetCID(&CardConfig.CardID) != MCI_FUNC_OK)
		{
			break;
		}

		/* Set Address */
		if(MCI_SetCardAddress() != MCI_FUNC_OK)
		{
			break;
		}
		CardConfig.CardAddress = MCI_GetCardAddress();

		/* Read CSD */
		if(MCI_GetCSD((uint32_t*)CardConfig.CSD) != MCI_FUNC_OK)
		{
			break;
		}
        swap_buff(&CardConfig.CSD[0], sizeof(uint32_t));
        swap_buff(&CardConfig.CSD[4], sizeof(uint32_t));
        swap_buff(&CardConfig.CSD[8], sizeof(uint32_t));
        swap_buff(&CardConfig.CSD[12], sizeof(uint32_t));
		swap_buff(CardConfig.CSD, 16);	

		/* sector size */
    	CardConfig.SectorSize = 512;
		
        csd_struct = CardConfig.CSD[15] >> 6;
		 /* Block count */
	    if (csd_struct == 1)    /* CSD V2.0 */
	    {
	        /* Read C_SIZE */
	        c_size =  unstuff_bits(CardConfig.CSD, 48,22);
	        /* Calculate block count */
	       CardConfig.SectorCount  = (c_size + 1) * 1024;

	    } else   /* CSD V1.0 (for Standard Capacity) */
	    {
	        /* C_SIZE */
	        c_size = unstuff_bits(CardConfig.CSD, 62,12);
	        /* C_SIZE_MUTE */
	        c_size_mult = unstuff_bits(CardConfig.CSD, 47,3);
	        /* READ_BL_LEN */
	        read_bl_len = unstuff_bits(CardConfig.CSD, 80,4);
	        /* sector count = BLOCKNR*BLOCK_LEN/512, we manually set SECTOR_SIZE to 512*/
	        CardConfig.SectorCount = (((c_size+1)* (0x01 << (c_size_mult+2))) * (0x01<<read_bl_len))/512;
	    }

        /* Get erase block size in unit of sector */
        switch (CardConfig.CardType)
        {
            case MCI_MMC_CARD:
                 CardConfig.BlockSize = unstuff_bits(CardConfig.CSD, 42,5) + 1;
				 CardConfig.BlockSize <<=  unstuff_bits(CardConfig.CSD, 22,4);
                 CardConfig.BlockSize /= 512;
                break;
            case MCI_SDHC_SDXC_CARD:
            case MCI_SDSC_V2_CARD:
                CardConfig.BlockSize = 1;
                break;
            case MCI_SDSC_V1_CARD:
                if(unstuff_bits(CardConfig.CSD, 46,1))
                {
                     CardConfig.BlockSize = 1;
                }
                else
                {
                    CardConfig.BlockSize = unstuff_bits(CardConfig.CSD, 39,7) + 1;
                    CardConfig.BlockSize <<=  unstuff_bits(CardConfig.CSD, 22,4);
                    CardConfig.BlockSize /= 512;
                }
                break;
            default:
                break;                
        }

		/* Select Card */
		if(MCI_Cmd_SelectCard() != MCI_FUNC_OK)
		{
			break;
		}

        MCI_Set_MCIClock( MCI_NORMAL_RATE );
		if ((CardConfig.CardType== MCI_SDSC_V1_CARD) ||
			(CardConfig.CardType== MCI_SDSC_V2_CARD) ||
			(CardConfig.CardType== MCI_SDHC_SDXC_CARD)) 
		{		
				if (MCI_SetBusWidth( SD_4_BIT ) != MCI_FUNC_OK )
				{
					break;
				}
		}
        else
        {
                if (MCI_SetBusWidth( SD_1_BIT ) != MCI_FUNC_OK )
				{
					break;
				}
        }
        
		/* For SDHC or SDXC, block length is fixed to 512 bytes, for others,
         the block length is set to 512 manually. */
        if (CardConfig.CardType == MCI_MMC_CARD ||
            CardConfig.CardType == MCI_SDSC_V1_CARD ||
            CardConfig.CardType == MCI_SDSC_V2_CARD )
        {
    		if(MCI_SetBlockLen(BLOCK_LENGTH) != MCI_FUNC_OK)
    		{
    			break;
    		}
        }
        
        return TRUE;
	}
	while (FALSE);

    return FALSE;
}

Bool mci_wait_for_ready (void)
{
	int32_t  cardSts;
    Timer2 = 50;    // 500ms
    while(Timer2)
	{	
		if((MCI_GetCardStatus(&cardSts) == MCI_FUNC_OK) &&
            (cardSts & CARD_STATUS_READY_FOR_DATA ))
		{
			return TRUE;
		}
			
	}
    return FALSE;    
}


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
DRESULT my_disk_ioctl (
	uint8_t drv,		/* Physical drive number (0) */
	uint8_t ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	uint8_t n, *ptr = buff;

	if (drv) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;

	res = RES_ERROR;

	switch (ctrl) {
	case CTRL_SYNC :		/* Make sure that no pending write process */
		if(mci_wait_for_ready() == TRUE)
		{
			res = RES_OK;
		}
		
		break;

	case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (uint32_t) */
		*(uint32_t*)buff = CardConfig.SectorCount;
		res = RES_OK;
		break;

	case GET_SECTOR_SIZE :	/* Get R/W sector size (uint16_t) */
		*(uint16_t*)buff = CardConfig.SectorSize;	//512;
		res = RES_OK;
		break;

	case GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (uint32_t) */
		*(uint32_t*)buff = CardConfig.BlockSize;
		res = RES_OK;
		break;

	case MMC_GET_TYPE :		/* Get card type flags (1 byte) */
		*ptr = CardConfig.CardType;
		res = RES_OK;
		break;

	case MMC_GET_CSD :		/* Receive CSD as a data block (16 bytes) */
		for (n=0;n<16;n++)
			*(ptr+n) = CardConfig.CSD[n]; 
		res = RES_OK;
		break;

	case MMC_GET_CID :		/* Receive CID as a data block (16 bytes) */
        {
            uint8_t* cid = (uint8_t*) &CardConfig.CardID;
    		for (n=0;n<sizeof(st_Mci_CardId);n++)
    			*(ptr+n) = cid[n];
        }
		res = RES_OK;
		break;

	case MMC_GET_SDSTAT :	/* Receive SD status as a data block (64 bytes) */
		{
			int32_t cardStatus;
			if(MCI_GetCardStatus(&cardStatus) == MCI_FUNC_OK) 
			{
                uint8_t* status = (uint8_t*)&cardStatus;
				for (n=0;n<2;n++)
           			 *(ptr+n) = ((uint8_t*)status)[n];
				res = RES_OK;   
			}
		}
		break;

	default:
		res = RES_PARERR;
	}

	return res;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
DRESULT my_disk_read (
	uint8_t drv,			/* Physical drive number (0) */
	uint8_t *buff,			/* Pointer to the data buffer to store read data */
	uint32_t sector,		/* Start sector number (LBA) */
	uint8_t count			/* Sector count (1..255) */
)
{
    volatile uint32_t tmp;
	if (drv || !count) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;
	if (MCI_ReadBlock (buff, sector, count) == MCI_FUNC_OK)	
	{
		//while(MCI_GetBlockXferEndState() != 0);
		while(MCI_GetDataXferEndState() != 0);

		if(count > 1)
		{
			if(MCI_Cmd_StopTransmission()  != MCI_FUNC_OK)
				return RES_ERROR;
		}
        if(MCI_GetXferErrState())
          return RES_ERROR;
        
		return RES_OK;
	}
	else
		return RES_ERROR;
}

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/
uint8_t my_disk_status (
	uint8_t drv		/* Physical drive number (0) */
)
{
	if (drv) return STA_NOINIT;		/* Supports only single drive */

	return Stat;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
#if _READONLY == 0
DRESULT my_disk_write (
	uint8_t drv,			/* Physical drive number (0) */
	const uint8_t *buff,	/* Pointer to the data to be written */
	uint32_t sector,		/* Start sector number (LBA) */
	uint8_t count			/* Sector count (1..255) */
)
{
    volatile uint32_t tmp;

	if (drv || !count) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;
//	if (Stat & STA_PROTECT) return RES_WRPRT;

	if ( MCI_WriteBlock((uint8_t*)buff, sector, count) == MCI_FUNC_OK)
	{
		//while(MCI_GetBlockXferEndState() != 0);
		while(MCI_GetDataXferEndState() != 0);

		if(count > 1)
		{
			if(MCI_Cmd_StopTransmission()  != MCI_FUNC_OK)
				return RES_ERROR;
		}
        if(MCI_GetXferErrState())
          return RES_ERROR;
        
		return RES_OK;
	}
	else
		return 	RES_ERROR;

}
#endif /* _READONLY == 0 */


/*-----------------------------------------------------------------------*/
/* Device timer function  (Platform dependent)                           */
/*-----------------------------------------------------------------------*/
/* This function must be called from timer interrupt routine in period
/  of 10 ms to generate card control timing.
*/							
void disk_timerproc (void)
{
    uint16_t n;

	n = Timer1;						/* 100Hz decrement timer stopped at 0 */
	if (n) Timer1 = --n;
	n = Timer2;
	if (n) Timer2 = --n;               
}

struct rt_device sdcard_device = {0};

static rt_err_t rt_sdcard_init(rt_device_t dev)
{
	return RT_EOK;
}

static rt_err_t rt_sdcard_open(rt_device_t dev, rt_uint16_t oflag)
{
	return RT_EOK;
}

static rt_err_t rt_sdcard_close(rt_device_t dev)
{
	return RT_EOK;
}

static rt_size_t rt_sdcard_read(rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
	//rt_kprintf("sd read %x %x\n",pos,size);
	if(my_disk_read(0,buffer,pos,size)==RES_OK)
		return size;
 	return 0;
}

static rt_size_t rt_sdcard_write (rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size)
{
	//rt_kprintf("sd write %x %x\n",pos,size);
	if(my_disk_write(0,buffer,pos,size)==RES_OK)
		return size;
 	return 0;
}

static rt_err_t rt_sdcard_control(rt_device_t dev, rt_uint8_t cmd, void *args)
{
    RT_ASSERT(dev != RT_NULL);

    if (cmd == RT_DEVICE_CTRL_BLK_GETGEOME)
    {
        struct rt_device_blk_geometry *geometry;

        geometry = (struct rt_device_blk_geometry *)args;
        if (geometry == RT_NULL) return -RT_ERROR;

        geometry->bytes_per_sector = CardConfig.SectorSize;
        geometry->block_size = CardConfig.BlockSize;
		
		geometry->sector_count = CardConfig.SectorCount;
    }

	return RT_EOK;
}


void rt_hw_sdcard_init(void)
{ 
	//uint8_t secbuf[512]={0};
	//struct dfs_partition part;
	if(sd_initialize()!= 0){
		rt_kprintf("sd_initialize failed\n");
	}
	//my_disk_read(0,secbuf,0,1);
	
	//dfs_filesystem_get_partition(&part,secbuf,RT_NULL);
	sdcard_device.type  = RT_Device_Class_Block;
	sdcard_device.init 	= rt_sdcard_init;
	sdcard_device.open 	= rt_sdcard_open;
	sdcard_device.close = rt_sdcard_close;
	sdcard_device.read 	= rt_sdcard_read;
	sdcard_device.write = rt_sdcard_write;
	sdcard_device.control = rt_sdcard_control;
	sdcard_device.user_data = &CardConfig;
	rt_device_register(&sdcard_device, "sd0",
			RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_REMOVABLE | RT_DEVICE_FLAG_STANDALONE);
	 
	//rt_kprintf("sdcard hw init failed\n");
}


#if 0
void WDT_IRQHandler	  (void)
{
  while(1);
}
void TIMER0_IRQHandler 	  (void)
{
  while(1);
}
void TIMER1_IRQHandler	  (void)
{
  while(1);
}
void TIMER2_IRQHandler	(void)
{
  while(1);
}
void TIMER3_IRQHandler(void)
{
  while(1);
}

void UART1_IRQHandler(void)
{
  while(1);
}
void UART2_IRQHandler(void)
{
  while(1);
}
void UART3_IRQHandler(void)
{
  while(1);
}
void PWM1_IRQHandler(void)
{
  while(1);
}
void I2C0_IRQHandler(void)
{
  while(1);
}
void I2C1_IRQHandler(void)
{
  while(1);
}
void I2C2_IRQHandler(void)
{
  while(1);
}
void SPIFI_IRQHandler(void)
{
  while(1);
}
void SSP0_IRQHandler  (void)
{
  while(1);
}
void SSP1_IRQHandler(void)
{
  while(1);
}
void PLL0_IRQHandler(void)
{
  while(1);
}
void RTC_IRQHandler(void)
{
  while(1);
}
void EINT0_IRQHandler(void)
{
  while(1);
}
void EINT1_IRQHandler(void)
{
  while(1);
}
void EINT2_IRQHandler(void)
{
  while(1);
}
void EINT3_IRQHandler(void)
{
  while(1);
}
void ADC_IRQHandler(void)
{
  while(1);
}
void BOD_IRQHandler(void)
{
  while(1);
}
void USB_IRQHandler(void)
{
  while(1);
}
void CAN_IRQHandler(void)
{
  while(1);
}

void I2S_IRQHandler(void)
{
  while(1);
}


void MCPWM_IRQHandler(void)
{
  while(1);
}
void QEI_IRQHandler(void)
{
  while(1);
}
void PLL1_IRQHandler(void)
{
  while(1);
}
void USBActivity_IRQHandler(void)
{
  while(1);
}
void CANActivity_IRQHandler(void)
{
  while(1);
}
void UART4_IRQHandler(void)
{
  while(1);
}
void SSP2_IRQHandler(void)
{
  while(1);
}
void LCD_IRQHandler(void)
{
  while(1);
}
void GPIO_IRQHandler(void)
{
  while(1);
}
void PWM0_IRQHandler(void)
{
  while(1);
}
void EEPROM_IRQHandler(void)
{
  while(1);
}
#endif


/******************************************************************************
**                            End Of File
******************************************************************************/

/**
 * @}
 */
 
