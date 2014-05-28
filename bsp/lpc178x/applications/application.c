/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2010-05-02     Aozima       add led function
 */

#include <rtthread.h>

#include <board.h>

#ifdef RT_USING_DFS
/* dfs init */
#include <dfs_init.h>
/* dfs filesystem:ELM FatFs filesystem init */
#include <dfs_elm.h>
/* dfs Filesystem APIs */
#include <dfs_fs.h>
#endif

#ifdef RT_USING_LWIP
#include <lwip/sys.h>
#include <lwip/api.h>
#include <netif/ethernetif.h>
#endif

#ifdef RT_USING_RTGUI
#include <rtgui/driver.h>
#include "touchpanel.h"
#endif

#include "led.h"
#include "sh_app.h"

#ifdef __CC_ARM
extern int Image$$RW_IRAM1$$ZI$$Base;
extern int Image$$RW_IRAM1$$ZI$$Limit;
extern int Image$$RW_IRAM2$$ZI$$Base;
extern int Image$$RW_IRAM2$$ZI$$Limit;
#endif
/* thread phase init */
void rt_init_thread_entry(void *parameter)
{
	rt_hw_led_init();
    /* Filesystem Initialization */
#ifdef RT_USING_DFS
    {
        /* init the device filesystem */
        dfs_init();

        /* init the elm FAT filesystam*/
        elm_init();

        /* mount sd card fat partition 1 as root directory */
        if (dfs_mount("sd0", "/", "elm", 0, 0) == 0)
            rt_kprintf("File System initialized!\n");
        else
            rt_kprintf("File System init failed!\n");
    }
#endif

    /* LwIP Initialization */
#ifdef RT_USING_LWIP
    {
        extern void lwip_sys_init(void);
        extern void lpc17xx_emac_hw_init(void);

        eth_system_device_init();

        /* register ethernetif device */
        lpc17xx_emac_hw_init();
        /* init all device */
        rt_device_init_all();

        /* init lwip system */
        lwip_sys_init();
        rt_kprintf("TCP/IP initialized!\n");
    }
#endif

#ifdef RT_USING_RTGUI
    {
    	extern void rtgui_system_server_init(void);
		extern void application_init(void);
		extern void rt_hw_lcd_init(void);
		rt_device_t lcd;

		/* init lcd */
		rt_hw_lcd_init();
		/* re-init device driver */
		rt_device_init_all();

		/* find lcd device */
		lcd = rt_device_find("lcd");
		if (lcd != RT_NULL)
		{
			/* set lcd device as rtgui graphic driver */
			rtgui_graphic_set_device(lcd);

			/* init rtgui system server */
			rtgui_system_server_init();

			/* startup rtgui in demo of RT-Thread/GUI examples */
			//application_init();
#ifdef RTGUI_USING_TOUCH
			{
				rt_thread_t tpth = RT_NULL;
			 	tp_init();
				tpth = rt_thread_create("tp",tp_entry,RT_NULL,2048,10,20);
				if(tpth!=RT_NULL){
				 	rt_thread_startup(tpth);
				}
				else{
				 	rt_kprintf("create touchpanel thread failed\n");
				}
			}
#endif
		}
    }
#endif
#ifdef RT_USING_LWIP	
	netapp_start();
#endif
}



int rt_application_init(void)
{
	rt_thread_t tid;

	tid = rt_thread_create("init",
			rt_init_thread_entry, RT_NULL,
			2048, RT_THREAD_PRIORITY_MAX/3, 20);
	if (tid != RT_NULL) rt_thread_startup(tid);

	return 0;
}

#if defined(RT_USING_RTGUI) && defined(RT_USING_FINSH)
#include <rtgui/rtgui_server.h>
#include <rtgui/event.h>
#include <rtgui/kbddef.h>

#include <finsh.h>
void key(rt_uint32_t key)
{
	struct rtgui_event_kbd ekbd;

	RTGUI_EVENT_KBD_INIT(&ekbd);
	ekbd.mod  = RTGUI_KMOD_NONE;
	ekbd.unicode = 0;
	ekbd.key = key;

	ekbd.type = RTGUI_KEYDOWN;
	rtgui_server_post_event((struct rtgui_event*)&ekbd, sizeof(ekbd));

	rt_thread_delay(2);

	ekbd.type = RTGUI_KEYUP;
	rtgui_server_post_event((struct rtgui_event*)&ekbd, sizeof(ekbd));
}
FINSH_FUNCTION_EXPORT(key, send a key to gui server);
#endif
