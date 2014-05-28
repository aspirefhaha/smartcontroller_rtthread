#include <string.h>
#include <stdio.h>
#include <rtthread.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/sockets.h> 
#include <lwip/netif.h>
#include <lwip/netdb.h>
#include <netif/ethernetif.h>
#include <dfs_fs.h>
#include <dfs_posix.h>
#include "httpclient.h"
#include "debug.h"
#include "sc_ping.h"

#define hc_printf(tbuf,tlen)	do{\
								int i = 0;\
								for ( i = 0 ; i<tlen ;i++){\
									rt_kprintf("%c",(tbuf)[i]);\
								}\
							}while(0)
#define HTTP_TMPBUF_LEN	128
#define HTTP_RESP_LEN	15

static void parseHttpResult(int tmpSock,char * pBuf,const char * filename,int tmpret,char ** retbuf)
{
	int tmpfile = -1;
	int retbufoffset = 0;
	//to find double \r\n\r\n
	do{
		char * pTC = strstr(pBuf,"\r\n\r\n");
#if DEBUGLEVEL > 0
		 hc_printf((const char *)pBuf,tmpret);
#endif
		if(pTC != RT_NULL){	//找到了
			pTC += 4; 	
			tmpret -= pTC - pBuf;	//剩下有用的
			if(tmpret == 0){	  //正好到结尾
				pTC = pBuf;
			 	tmpret = recv(tmpSock,pBuf,HTTP_TMPBUF_LEN,0);
				break;
			}
			else{
			    int leftLen = tmpret + pBuf - pTC;
				tmpret = leftLen;
				while(leftLen-- >= 0)
				{
				 	* pBuf ++  = * pTC ++;
				}
				tmpret += recv(tmpSock,pBuf + tmpret,HTTP_TMPBUF_LEN - tmpret,0);
			}
		}
		else {	//没找到
			pTC = strrchr(pBuf,'\r');
			if(pTC){
				int ttret = 0;
				int leftLen = tmpret + pBuf - pTC;
				tmpret = leftLen;
				while(leftLen-- >= 0)
				{
				 	* pBuf ++  = * pTC ++;
				}
				ttret = recv(tmpSock,pBuf + tmpret,HTTP_TMPBUF_LEN - tmpret,0);
				if(ttret <= 0 ){
					tmpret = 0 ;
					break;
				}
				tmpret += ttret;	
			}
			else{  //不应该到这的
				//rt_kprintf("should not here\n");
				RT_ASSERT(0);
			 	return;
			}	 	
		}
	}while(tmpret > 0);
	if(tmpret > 0 ){
		if(filename)
			tmpfile = open(filename,DFS_O_WRONLY | DFS_O_CREAT | DFS_O_TRUNC,0);
		else if(retbuf){
		 	* retbuf = rt_malloc(tmpret+1);
			RT_ASSERT(*retbuf);
			rt_memset(*retbuf,0,tmpret +1);
			retbufoffset = 0;
		}
	}
	while( tmpret >0){
#if DEBUGLEVEL > 0 		
	 	hc_printf((const char *)pBuf,tmpret);
		rt_kprintf("\n");
#endif
		if(tmpfile>=0){
		 	int writeret = write(tmpfile,pBuf,tmpret);
			if(writeret != tmpret){
			 	while(1)rt_kprintf("write file failed need %d write %d\n",tmpret,writeret);
			}
		}
		else if( * retbuf && retbuf != RT_NULL && *retbuf!= RT_NULL){
		 	rt_memcpy((*retbuf) + retbufoffset,pBuf,tmpret);
			retbufoffset += tmpret;
		}
		tmpret = recv(tmpSock,pBuf,HTTP_TMPBUF_LEN,0);
		if(tmpfile < 0 && retbuf != RT_NULL && *retbuf && tmpret){
		 	*retbuf = rt_realloc(*retbuf,retbufoffset + tmpret +1);
		}
	}
	if(tmpfile >= 0){
		close(tmpfile);
	}
	if(*retbuf){
		(*retbuf)[retbufoffset]=0;
	}
}

int prepareServer(const char * server,struct sockaddr_in * ts)
{
 	int addr1,addr2,addr3,addr4;
 	if(sscanf(server,"%d.%d.%d.%d",&addr1,&addr2,&addr3,&addr4) &&
		addr1 && addr4 && strlen(server)){
 		ts->sin_addr.s_addr = inet_addr(server);
	}
	else{
		struct hostent * tmp_he = RT_NULL;
		tmp_he = gethostbyname(server);
		if(tmp_he != RT_NULL){
			//ts.sin_addr.s_addr = tmp_he->h_addr_list[0];
			rt_memcpy(&ts->sin_addr,tmp_he->h_addr_list[0],tmp_he->h_length);
		}
		else {
			rt_kprintf(	"bad server name %s\n",server);
			return -1;
		}
	}
	return 0;
}

int st(int param)
{
	int tmpSock = 0;
	struct sockaddr_in ts = {0};	 //short name for target server	 
	int respCode = 0;
	ts.sin_family = AF_INET;
	ts.sin_port = htons(8080);
	while(1){
		//list_mem();
		tmpSock = socket(AF_INET,SOCK_STREAM,0);
		if(prepareServer("192.168.1.124",&ts)<0)
			return  respCode;
		if(connect(tmpSock,(struct sockaddr *)&ts,sizeof(struct sockaddr)) >= 0)
		{
			char * tmpbuf = rt_malloc(HTTP_TMPBUF_LEN+4);
			if(tmpbuf == RT_NULL){
			 	rt_kprintf("malloc failed in http get\n");
			}
			else{
				rt_memset(tmpbuf,0,HTTP_TMPBUF_LEN+4);
				rt_sprintf(tmpbuf,"GET /device HTTP/1.0\r\nHost:%s\r\n\r\n",inet_ntoa(*((struct in_addr*)&(netif_default->ip_addr))));
				if(send(tmpSock,tmpbuf,strlen(tmpbuf),0)==strlen(tmpbuf)){								   
					int tmpret = recv(tmpSock,tmpbuf,HTTP_TMPBUF_LEN,0); 
					if(tmpret > HTTP_RESP_LEN){
						int ver1=-1,ver2=-1;
						char tmpStr[24]={0};
					 	if(sscanf(tmpbuf,"HTTP/%d.%d %d %s\r\n",&ver1,&ver2,&respCode,tmpStr) &&
							ver1!= -1 && ver2!=-1 ){
						}
					}
					if(respCode == 200){
						char * retbuff = RT_NULL;
						parseHttpResult(tmpSock,tmpbuf,RT_NULL,tmpret,&retbuff);
						if(retbuff){
						 	rt_free(retbuff);
						}
					}
				}
				else
				{
				 	rt_kprintf("send get request failed\n");
				}
				rt_free(tmpbuf);
			}
			shutdown(tmpSock,0);
			closesocket(tmpSock);
			rt_kprintf("GET sock %d connect suc\n",tmpSock);
		}
		else
		{
			rt_kprintf("GET sock %d onnect failed\n",tmpSock);
			closesocket(tmpSock);
		}
#ifdef 	  RT_USING_MEMHEAP_AS_HEAP
		list_memheap();
#else
		list_mem();
#endif
	}
	return respCode;
}

int http_get(const char * server,unsigned int port,const char * url,const char * filename,char ** retbuf)
{
	int tmpSock = 0;
	struct sockaddr_in ts = {0};	 //short name for target server	 
	int respCode = 0;
	ts.sin_family = AF_INET;
	ts.sin_port = htons(port);
	tmpSock = socket(AF_INET,SOCK_STREAM,0);
	if(prepareServer(server,&ts)<0)	 //get Server address from string
		return  respCode;
	if(connect(tmpSock,(struct sockaddr *)&ts,sizeof(struct sockaddr)) >= 0)
	{
		char * tmpbuf = rt_malloc(HTTP_TMPBUF_LEN+4);
		if(tmpbuf == RT_NULL){
		 	rt_kprintf("malloc failed in http get\n");
		}
		else{
			rt_memset(tmpbuf,0,HTTP_TMPBUF_LEN+4);
			rt_sprintf(tmpbuf,"GET %s HTTP/1.0\r\nHost:%s\r\n\r\n",url,inet_ntoa(*((struct in_addr*)&(netif_default->ip_addr))));
			if(send(tmpSock,tmpbuf,strlen(tmpbuf),0)==strlen(tmpbuf)){								   
				{int tmpret = recv(tmpSock,tmpbuf,HTTP_TMPBUF_LEN,0);
				if(tmpret > HTTP_RESP_LEN){
					int ver1=-1,ver2=-1;
					char tmpStr[24]={0};
				 	if(sscanf(tmpbuf,"HTTP/%d.%d %d %s\r\n",&ver1,&ver2,&respCode,tmpStr) &&
						ver1!= -1 && ver2!=-1 ){
#if DEBUG_LEVEL > 1
						rt_kprintf("%d.%d %d %s\n",ver1,ver2,respCode,tmpStr,retbuf);
#endif
					}
				}
				if(respCode == 200){
					parseHttpResult(tmpSock,tmpbuf,filename,tmpret,retbuf);
				}
				}
			}
			else
			{
			 	rt_kprintf("send get request failed\n");
			}
			rt_free(tmpbuf);
		}
		shutdown(tmpSock,0);
		closesocket(tmpSock);
	}
	else
	{
	 	LWIP_DEBUGF(SC_DEBUG, ("httpget con err start mem %d\n",list_mem()));
		rt_kprintf("GET sock %d server:%s Port:%d url:%s connect failed\n",tmpSock,server,port,url);
		closesocket(tmpSock);
		LWIP_DEBUGF(SC_DEBUG, ("httpget con err end mem %d\n",list_mem()));
	}
	LWIP_DEBUGF(SC_DEBUG, ("httpget end mem %d\n\n",list_mem()));
	return respCode;
}

int http_put(const char * server,unsigned short port,const char * url,const char * data,int datalen)
{
 	 int tmpSock = socket(AF_INET,SOCK_STREAM,0);
	 struct sockaddr_in ts = {0};	 //short name for target server
	 int respCode = 0;
	 ts.sin_family = AF_INET;
	 ts.sin_port = htons(port);
	 if(prepareServer(server,&ts)<0)
	 	return respCode;
	if(connect(tmpSock,(struct sockaddr *)&ts,sizeof(struct sockaddr)) >= 0)
	{
		do {
			char * tmpbuf = rt_malloc(HTTP_TMPBUF_LEN+4);
			if(tmpbuf == RT_NULL){
		 		rt_kprintf("malloc failed in http get\n");
				break;
			}
			rt_memset(tmpbuf,0,HTTP_TMPBUF_LEN+4);
			rt_sprintf(tmpbuf,"PUT %s HTTP/1.0\r\nContent-Length:%d\r\nHost:%s\r\n\r\n",url,datalen,server);
			if(send(tmpSock,tmpbuf,strlen(tmpbuf),0) != strlen(tmpbuf)){
				rt_kprintf("send http put failed\n");
				break;
			}
			if(send(tmpSock,data,datalen,0)==datalen){								   
				int tmpret = recv(tmpSock,tmpbuf,HTTP_TMPBUF_LEN,0); 
				if(tmpret > HTTP_RESP_LEN){
					int ver1=-1,ver2=-1;
					char tmpStr[24]={0};
				 	if(sscanf(tmpbuf,"HTTP/%d.%d %d %s\r\n",&ver1,&ver2,&respCode,tmpStr) &&
						ver1!= -1 && ver2!=-1 ){
#if DEBUGLEVEL>1
						rt_kprintf("%d.%d %d %s\n",ver1,ver2,respCode,tmpStr);
#endif
					}
				}
				if(respCode == 200){
					parseHttpResult(tmpSock,tmpbuf,RT_NULL,tmpret,RT_NULL);
				}
				else
				{
					rt_kprintf("Get problem No.-%d.\n",respCode);
				}
			}
			else
			{
			 	rt_kprintf("send get request failed\n");
			}
			rt_free(tmpbuf);
		}while(0);
		closesocket(tmpSock);
	}
	else
	{
	 	rt_kprintf("Put connect failed\n");
	}
	return respCode;
}

int http_post(const char * server,unsigned short port,const char * url,const char * data,int datalen)
{
 	 int tmpSock = socket(AF_INET,SOCK_STREAM,0);
	 struct sockaddr_in ts = {0};	 //short name for target server
	 int respCode = 0;
	 ts.sin_family = AF_INET;
	 ts.sin_port = htons(port);
	 if(prepareServer(server,&ts)<0)
	 	return respCode;
	rt_kprintf("post data %s\n",data);
	if(connect(tmpSock,(struct sockaddr *)&ts,sizeof(struct sockaddr)) >= 0)
	{
		do {
			char * tmpbuf = rt_malloc(HTTP_TMPBUF_LEN+4);
			if(tmpbuf == RT_NULL){
		 		rt_kprintf("malloc failed in http get\n");
				break;
			}
			rt_memset(tmpbuf,0,HTTP_TMPBUF_LEN+4);
			rt_sprintf(tmpbuf,"POST %s HTTP/1.0\r\nContent-Length:%d\r\nHost:%s\r\n\r\n",url,datalen,server);
			if(send(tmpSock,tmpbuf,strlen(tmpbuf),0) != strlen(tmpbuf)){
				rt_kprintf("send http put failed\n");
				break;
			}
			if(send(tmpSock,data,datalen,0)==datalen){								   
				int tmpret = recv(tmpSock,tmpbuf,HTTP_TMPBUF_LEN,0); 
				int respCode = 0;
				if(tmpret > HTTP_RESP_LEN){
					int ver1=-1,ver2=-1;
					char tmpStr[24]={0};
				 	if(sscanf(tmpbuf,"HTTP/%d.%d %d %s\r\n",&ver1,&ver2,&respCode,tmpStr) &&
						ver1!= -1 && ver2!=-1 ){
#if DEBUGLEVEL >1
						rt_kprintf("%d.%d %d %s\n",ver1,ver2,respCode,tmpStr);
#endif
					}
				}
				if(respCode == 200){
					parseHttpResult(tmpSock,tmpbuf,RT_NULL,tmpret,RT_NULL);
				}
			}
			else
			{
			 	rt_kprintf("send get request failed\n");
			}
			rt_free(tmpbuf);
		}while(0);
		closesocket(tmpSock);
	}
	else
	{
	 	rt_kprintf("Post connect %s port %u failed\n",server,port);
	}
	return respCode;
}


#ifdef RT_USING_FINSH	
#include <finsh.h>
#define PUTDATA "{\"subDevice\":\"67676565\"}"
#define TESTSERVER	"192.168.1.124"
/*long hc_test(void)
{
	//http_get("zlcb.8866.org",8080,"/controller?useAs=embeded","/controll.jsn");
	//http_post(TESTSERVER,8080,"/controller/b3b3b3b3?useAs=embeded",PUTDATA,strlen(PUTDATA));
	return 0;
}
FINSH_FUNCTION_EXPORT(hc_test, test http client lib parser);*/
FINSH_FUNCTION_EXPORT(http_get, http client lib get);
FINSH_FUNCTION_EXPORT(http_put, http client lib put);
FINSH_FUNCTION_EXPORT(http_post, http client lib post);
FINSH_FUNCTION_EXPORT(st, socket test);
#endif
