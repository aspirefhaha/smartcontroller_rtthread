/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 */

/**
 * @addtogroup STM32
 */
/*@{*/

#include <board.h>
#include <rtthread.h>

#ifdef RT_USING_DFS
/* dfs init */
#include <dfs_init.h>
/* dfs filesystem:ELM filesystem init */
#include <dfs_elm.h>
/* dfs Filesystem APIs */
#include <dfs_fs.h>
#endif

#ifdef RT_USING_LWIP
#include <lwip/sys.h>
#include <lwip/api.h>
#include <netif/ethernetif.h>
#include "stm32f2x7_eth.h"

#endif

//fhaha modify
#ifdef RT_USING_RTGUI
#include <rtgui/rtgui.h>
#include <rtgui/driver.h>
#include <rtgui/rtgui_server.h>
#include <rtgui/event.h>
#include "LCD/LCD.h"
#include "TouchPanel/TouchPanel.h"
#ifdef RT_USING_LWIP
void rt_hw_stm32_eth_init(void);
#endif
#define RTGUI_EVENT_MOUSE_MOTION_INIT(e)    RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_MOUSE_MOTION)

void tp_thread_entry(void * parameter)
{
	struct rtgui_event_mouse emouse;
/*	static struct _touch_previous
    {
        rt_uint32_t x;
        rt_uint32_t y;
    } touch_previous;*/
	RTGUI_EVENT_MOUSE_MOTION_INIT(&emouse);
    emouse.wid = RT_NULL;
	rt_kprintf("start tp_thread\n");
	while (1)	
	{
		getDisplayPoint(&display, Read_Ads7846(), &matrix ) ;
		if(display.x!=0 && display.y != 0 ){
			rt_kprintf("get x %d y %d\n",display.x,display.y);
			emouse.button = (RTGUI_MOUSE_BUTTON_LEFT |RTGUI_MOUSE_BUTTON_UP);

	        /* use old value */
	        emouse.x = display.x;
	        emouse.y = display.y;
			rtgui_server_post_event(&emouse.parent, sizeof(struct rtgui_event_mouse));
		}
		rt_thread_delay(RT_TICK_PER_SECOND/10);
	//TP_DrawPoint(display.x,display.y);
	} 	
}
#endif
//modify end
void msg(void);
void rt_init_thread_entry(void* parameter)
{
#ifdef RT_USING_RTGUI
	{
	 	rt_device_t lcd = rt_device_find("lcd");
		rt_thread_t tp_thread = RT_NULL;
		struct rt_device_rect_info info; 
		TouchPanel_Calibrate();
		tp_thread = rt_thread_create("tp",tp_thread_entry,RT_NULL,1024,RT_THREAD_PRIORITY_MAX/2,20);
		if(tp_thread!=RT_NULL){
			rt_thread_startup(tp_thread);
		}
		else
			rt_kprintf("create tp thread failed!\n");
		if(lcd!=RT_NULL){
			info.width = 320;        
	        info.height = 240;        
	        /* set graphic resolution */        
	        rt_device_control(lcd, RTGRAPHIC_CTRL_SET_MODE, &info);    
			rtgui_graphic_set_device(lcd);
			rtgui_font_system_init();
	 		rt_kprintf("rtgui starting...\n");
			rtgui_server_init();
			app_mgr_init();
			rt_thread_delay(10);
		}
		else
			rt_kprintf("can not find lcd device\n");
	}
#endif
/* Filesystem Initialization */
#ifdef RT_USING_DFS
	{
		/* init the device filesystem */
		dfs_init();

#ifdef RT_USING_DFS_ELMFAT
		/* init the elm chan FatFs filesystam*/
		elm_init();

		/* mount sd card fat partition 1 as root directory */
		if (dfs_mount("sd0", "/", "elm", 0, 0) == 0)
		{
			rt_kprintf("File System initialized!\n");
		}
		else
			rt_kprintf("File System initialzation failed!\n");
#endif
	}
#endif

/* LwIP Initialization */
#ifdef RT_USING_LWIP
	{
		extern void lwip_sys_init(void);

		/* register ethernetif device */
		eth_system_device_init();
	
		/* initialize eth interface */
		rt_hw_stm32_eth_init();
	
		/* re-init device driver */
		rt_device_init_all();

		/* init lwip system */
		lwip_sys_init();
		rt_kprintf("TCP/IP initialized!\n");
	}
#endif
#if 1
	//rt_kprintf("start msg\n");
	//msg();
#else
	tp_thread_entry(RT_NULL);
#endif

}

int rt_application_init()
{
	rt_thread_t tid;

	tid = rt_thread_create("init",
								rt_init_thread_entry, RT_NULL,
								2048, RT_THREAD_PRIORITY_MAX/3, 20);

	if (tid != RT_NULL)
		rt_thread_startup(tid);

	return 0;
}

/*@}*/
