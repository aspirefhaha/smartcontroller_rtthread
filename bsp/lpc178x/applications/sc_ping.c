
/********************************************************
 *	IP报头格式数据结构定义在<netinet/ip.h>中	*
 *	ICMP数据结构定义在<netinet/ip_icmp.h>中		*
 *	套接字地址数据结构定义在<netinet/in.h>中	*
 ********************************************************/
#include <rtthread.h>

#include <lwip/inet.h>

#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/sockets.h> 
#include <lwip/netif.h>
#include <netif/ethernetif.h>
#include <lwip/icmp.h>
#include <lwip/ip.h>
#include <lwip/netdb.h>
#include <time.h>
#include <lpc177x_8x_rtc.h>

#ifdef RT_USING_FINSH
#define PINGDATALEN	64

void tv_sub(struct timeval *out,struct timeval *in);



/****检验和算法****/
unsigned short cal_chksum(unsigned short *addr,int len)
{
	int nleft = len;
	int sum = 0;
	unsigned short *w = addr;
	unsigned short check_sum = 0;

	while(nleft>1)		//ICMP包头以字（2字节）为单位累加
	{
		sum += *w++;
		nleft -= 2;
	}

	if(nleft == 1)		//ICMP为奇数字节时，转换最后一个字节，继续累加
	{
		*(unsigned char *)(&check_sum) = *(unsigned char *)w;
		sum += check_sum;
	}
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	check_sum = ~sum;	//取反得到校验和
	return check_sum;
}

time_t time(time_t * timer)
{
	RTC_TIME_Type rtc;


	RTC_GetFullTime( LPC_RTC, &rtc);
	if(timer!=NULL)
		*timer = ((uint32_t)(rtc.YEAR - 1980) << 25)
			| ((uint32_t)rtc.MONTH << 21)
			| ((uint32_t)rtc.DOM << 16)
			| ((uint32_t)rtc.HOUR << 11)
			| ((uint32_t)rtc.MIN<< 5)
			| ((uint32_t)rtc.SEC>> 1);
	return	  ((uint32_t)(rtc.YEAR - 1980) << 25)
			| ((uint32_t)rtc.MONTH << 21)
			| ((uint32_t)rtc.DOM << 16)
			| ((uint32_t)rtc.HOUR << 11)
			| ((uint32_t)rtc.MIN<< 5)
			| ((uint32_t)rtc.SEC>> 1);

} 

/*设置ICMP报头*/
int pack(unsigned short pid,unsigned char * buf,int len)
{
	struct icmp_echo_hdr *icmp;
	time_t * tval;
	RT_ASSERT(len>= PINGDATALEN);
	icmp = (struct icmp_echo_hdr*)buf;
	icmp->type = ICMP_ECHO;	//ICMP_ECHO类型的类型号为0
	icmp->code = 0;
	icmp->chksum = 0;
	icmp->seqno = pid;	//发送的数据报编号
	icmp->id = pid;

	tval = (time_t  *)(buf + sizeof(struct icmp_echo_hdr));
	time(tval);		//记录发送时间
	//校验算法
	icmp->chksum =  cal_chksum((unsigned short *)icmp,PINGDATALEN);	
	return PINGDATALEN;		//数据报大小为64字节
}

/****发送三个ICMP报文****/
void send_packet(int sockfd,struct sockaddr_in * pdest_addr,unsigned char * buf,int len)
{
	int packetsize;	
	static int pid = 1;
	packetsize = pack(pid ++,buf,len);	//设置ICMP报头
	//发送数据报
	if(sendto(sockfd,buf,packetsize,0,
		(struct sockaddr *)pdest_addr,sizeof(struct sockaddr_in)) < 0)
	{
		rt_kprintf("sendto error");
	}
}

/******剥去ICMP报头******/
int unpack(unsigned char *buf,int len,unsigned short pid,struct sockaddr_in * from)
{
	int iphdrlen;		//ip头长度
	struct ip_hdr *ip;
	struct icmp_echo_hdr *icmp;
	time_t *tvsend;
	time_t tvrecv,rtt;

	time(&tvrecv);
	ip = (struct ip_hdr *)buf;
	iphdrlen = ip->_len << 2;	//求IP报文头长度，即IP报头长度乘4
	icmp = (struct icmp_echo_hdr *)(buf + iphdrlen);	//越过IP头，指向ICMP报头
	len -= iphdrlen;	//ICMP报头及数据报的总长度
	if(len < 8)		//小于ICMP报头的长度则不合理
	{
		rt_kprintf("ICMP packet\'s length is less than 8\n");
		return -1;
	}
	//确保所接收的是所发的ICMP的回应
	if((icmp->type == ICMP_ER) && (icmp->id == pid))
	{
		tvsend = (time_t *)(buf + sizeof(struct icmp_echo_hdr));
		rtt = tvrecv - *tvsend; 
		//all_time += rtt;	//总时间
		//显示相关的信息
		rt_kprintf("%d bytes from %s: icmp_seq=%u ttl=%d time=%ds\n",
				len,inet_ntoa(from->sin_addr),
				icmp->seqno,ip->_proto,rtt);
	}
	else 
		return -1;
	return 0;
}

/****接受所有ICMP报文****/
int recv_packet(int sockfd,struct sockaddr_in * pdest_addr,unsigned char * buf,int len)
{
	u32_t fromlen;
	//static int pid = 1;
	struct sockaddr from;
	fromlen = sizeof(from);
	//接收数据报
	if((recvfrom(sockfd,buf,len,MSG_DONTWAIT,
		(struct sockaddr *)&from,&fromlen)) < 0)
	{
		rt_kprintf("recvfrom error");
		return -1;
	}
	//gettimeofday(&tvrecv,NULL);		//记录接收时间
	//unpack(buf,len,pid++,(struct sockaddr_in *)&from);		//剥去ICMP报头
	return 0;
}

/*主函数*/
int ping(const char* peeraddr)
{
	struct hostent *host;
	int sockfd;
	struct sockaddr_in dest_addr;
	int ret = 0;
//	int waittime = MAX_WAIT_TIME;
//	int size = 50 * 1024;
	
	unsigned char buf[PINGDATALEN]={0};

	//生成使用ICMP的原始套接字，只有root才能生成
	if((sockfd = socket(AF_INET,SOCK_RAW,IP_PROTO_ICMP)) < 0)
	{
		rt_kprintf("socket error");
		return 1;
	}

	rt_memset(&dest_addr,0,sizeof(dest_addr));	//初始化
	dest_addr.sin_family = AF_INET;		//套接字域是AF_INET(网络套接字)

	//判断主机名是否是IP地址
	if(inet_addr(peeraddr) == INADDR_NONE)
	{
		if((host = gethostbyname(peeraddr)) == NULL)	//是主机名
		{
			rt_kprintf("gethostbyname error");
			return -1;
		}
		memcpy((char *)&dest_addr.sin_addr,host->h_addr,host->h_length);
	}
	else{ //是IP 地址
		dest_addr.sin_addr.s_addr = inet_addr(peeraddr);
	}
	//rt_kprintf("PING %s(%s):%d bytes of data.\n",peeraddr,
	//		inet_ntoa(dest_addr.sin_addr),PINGDATALEN);

	
	send_packet(sockfd,&dest_addr,buf,PINGDATALEN);		//发送ICMP报文
	rt_thread_delay(RT_TICK_PER_SECOND);		//等待一秒
	ret = recv_packet(sockfd,&dest_addr,buf,PINGDATALEN);		//接收ICMP报文
	

	lwip_close(sockfd);
	if(ret==0){
		printf("dest %s is reachable\n",peeraddr);
	}
	else
		printf("dest %s is unreachable\n",peeraddr);
	return ret;
}
#include <finsh.h>
FINSH_FUNCTION_EXPORT(ping, ping a host )

#endif
