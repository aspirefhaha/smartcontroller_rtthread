#include <stdio.h>
#include <rtthread.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/sockets.h> 
#include <lwip/netif.h>
#include <netif/ethernetif.h>
#include "netcnf.h"
#include <dfs_posix.h>
#include "config_init.h"
#include "main_win.h"


const char hellostr[]="helloworld!";

void udp_hello(void * parameter)
{
 	int sockd;
	int i = 60;
	struct sockaddr_in taraddr;
	struct netif * netif = (struct netif*)parameter;
	rt_kprintf("ip address: %s\n", inet_ntoa(*((struct in_addr*)&(netif->ip_addr))));
	sockd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	rt_kprintf("create UDP socket %d\n",sockd);
	taraddr.sin_family = AF_INET;
	taraddr.sin_addr.s_addr = netif->ip_addr.addr | 0xff000000;
	//taraddr.sin_addr.s_addr = inet_addr("192.168.38.3");
	taraddr.sin_port = htons(9001);
	while(i-->0){
		rt_thread_delay(RT_TICK_PER_SECOND);
		sendto(sockd,(const void *)hellostr,sizeof(hellostr),0,(struct sockaddr*)&taraddr,sizeof(taraddr));
		
	}
	closesocket(sockd);
	return;
}
long reset(void)
{
 	rt_kprintf("now show reset board\n");
	return 0;
}
void tcp_get_conf_entry(void * parameter)
{
	int sockd,peersock;
	int ret = 0;
	struct sockaddr_in taraddr,selfaddr;
	struct netif * netif = (struct netif*)parameter;
	rt_kprintf("ip address: %s\n", inet_ntoa(*((struct in_addr*)&(netif->ip_addr))));
	
	sockd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	rt_kprintf("create TCP socket %d\n",sockd);
	selfaddr.sin_family = AF_INET;
	selfaddr.sin_addr.s_addr = htons(INADDR_ANY);
	selfaddr.sin_port = htons(9001);

	ret = bind(sockd,(struct sockaddr*)&selfaddr,sizeof(selfaddr));
	if(ret != 0){
	 	rt_kprintf("TCP Bind port %d failed!",9001);
		closesocket(sockd);
		return;
	}
	ret = listen(sockd,2);
	if(ret != 0){
	 	rt_kprintf("TCP listen port %d failed!",9001);
		closesocket(sockd);
		return;
	}
	while(1){
		int getsize = sizeof(taraddr);
		peersock = lwip_accept(sockd,(struct sockaddr*)&taraddr,(socklen_t *)&getsize);
		if(peersock < 0){
		 	rt_kprintf("Server accept failed!\n");
			closesocket(sockd);
			return;
		}
		else{
			int filesize = 0 ;
			int  filed  = 0;
			#define RECVBUFSIZE 512
			char recvbuf[RECVBUFSIZE]={0};	 
			int len=0;	
			int cmd = 0;					   
			getsize = 0;
			rt_kprintf("get one client %s connect\n",inet_ntoa(taraddr.sin_addr.s_addr));
			len = recv(peersock, & filesize,sizeof(filesize),0);
			if(len != sizeof(filesize)){
			 	rt_kprintf("get filesize failed!\n");
				closesocket(peersock);
				continue;
			}
			cmd = ((filesize & 0xff000000) >> 24);
			rt_kprintf("get net cmd %d",cmd);
			if(cmd == 0x01){
			 	filed = open("/config.xml",O_WRONLY | O_CREAT | O_TRUNC,0);
				filesize ^= 0x01000000;
			}
			else if(cmd == 0x00){
				filed = open("/topology.xml",O_WRONLY | O_CREAT | O_TRUNC,0);

			}
			else if(cmd == 0x02){
				closesocket(peersock);
				closesocket(sockd);
			 	reset();
			}
			do{
				len = recv(peersock,recvbuf,RECVBUFSIZE,0);
				getsize += len;
				if(len >0 ){
				 	//接收成功，写数据
					write(filed,recvbuf,len);
					if(getsize >= filesize){
					 	close(filed);
						 break;
					}
				}
				else {
				 	//接收失败了
					break;
				}
					
			}while(len >0 && getsize < filesize );
			closesocket(peersock);
		}
	}
	return;	
}


void testxml_fn(struct netif *netif)
{
	rt_thread_t unet_thread, shnet_thread ;
	unet_thread = rt_thread_create("uhello",udp_hello,(void*)netif,1024,12,10);
	if(unet_thread!= RT_NULL){
		 if(RT_EOK!=rt_thread_startup(unet_thread)){
		 	rt_kprintf("start udp hello thread failed!!!!\n");
		}
	}
	else{
		rt_kprintf("create udp hello thread failed!!!!\n");
	}
	shnet_thread = rt_thread_create("netcnf",tcp_get_conf_entry,(void *)netif,2048,11,20);
	if(shnet_thread!=RT_NULL){
		if(RT_EOK!=rt_thread_startup(shnet_thread)){
		 	rt_kprintf("start tcp confif thread failed!!!!\n");
		}
	}
	else{
	 	rt_kprintf("create shnet thread failed!!!!\n");
	}
	config_init();
	topoligy_init();
#ifdef RT_USING_RTGUI
	{
		rt_thread_t tid;
	
	    tid = rt_thread_create("mgui",
	                           showgui, RT_NULL,
	                           2048 * 2, 25, 10);
	
	    if (tid != RT_NULL)
	        rt_thread_startup(tid);
	
	}	
#endif
}


