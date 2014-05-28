
#include <rtthread.h>
#include <string.h>
#include <stdio.h>
#include "sh_app.h"
#include "lwip/netif.h"
#include "httpclient.h"
#include "../test_xml/netcnf.h"

#define SHCONURL "/shcontroller"

unsigned int devguid[4]={0};
static void shnet_entry(void* parameter)
{
	char * result;
	int httpret=0;
	char url[128]={0};
	sprintf(url,"%s/%08x%08x%08x%08x",SHCONURL,devguid[0],devguid[1],devguid[2],devguid[3]);
	testxml_fn(parameter);
	while(httpret != 200){
		httpret = http_get(SHSERVER,SHPORT,url,NULL,&result);
		if(httpret == 200 && result != RT_NULL){
		 printf(result);
		}
		else{
		 rt_kprintf("http failed ret %d\n",httpret);
		}
		if(result!= RT_NULL)
			rt_free(result);
	}
}

#ifdef RT_LWIP_DHCP
static void netifstatus_callback_fn(struct netif *netif)
{
	rt_thread_t shnet_thread = rt_thread_create("shnet",shnet_entry,netif,2048,11,20);
	if(shnet_thread!=RT_NULL){
		if(RT_EOK!=rt_thread_startup(shnet_thread)){
		 	rt_kprintf("start smart home net thread failed!!!!\n");
		}
	}
	else{
	 	rt_kprintf("create shnet thread failed!!!!\n");
	}	
}

static void netiflink_callback_fn(struct netif *netif)
{
	rt_kprintf("netif link callback\n");
}
#endif
void netapp_start(void)
{
	struct netif * eth0 = netif_find("e0");
#ifdef RT_LWIP_DHCP
 	 if(eth0){
  		netif_set_status_callback(eth0, netifstatus_callback_fn);
		netif_set_link_callback(eth0,netiflink_callback_fn);
	 }
	else{
		rt_kprintf("find netif e0 failed!!!!!\n");
	}
#else
	if(eth0){
		while(eth0->flags & NETIF_FLAG_UP==0)
			rt_thread_delay(RT_TICK_PER_SECOND);
		{
			rt_thread_t shnet_thread = rt_thread_create("shnet",shnet_entry,eth0,2048,11,20);
			if(shnet_thread!=RT_NULL){
				if(RT_EOK!=rt_thread_startup(shnet_thread)){
				 	rt_kprintf("start smart home net thread failed!!!!\n");
				}
			}
			else{
			 	rt_kprintf("create shnet thread failed!!!!\n");
			}
		}
	}
	else{
		rt_kprintf("find netif e0 failed!!!!!\n");
	}

#endif

}

