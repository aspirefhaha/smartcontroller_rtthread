
/********************************************************
 *	IP��ͷ��ʽ���ݽṹ������<netinet/ip.h>��	*
 *	ICMP���ݽṹ������<netinet/ip_icmp.h>��		*
 *	�׽��ֵ�ַ���ݽṹ������<netinet/in.h>��	*
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



/****������㷨****/
unsigned short cal_chksum(unsigned short *addr,int len)
{
	int nleft = len;
	int sum = 0;
	unsigned short *w = addr;
	unsigned short check_sum = 0;

	while(nleft>1)		//ICMP��ͷ���֣�2�ֽڣ�Ϊ��λ�ۼ�
	{
		sum += *w++;
		nleft -= 2;
	}

	if(nleft == 1)		//ICMPΪ�����ֽ�ʱ��ת�����һ���ֽڣ������ۼ�
	{
		*(unsigned char *)(&check_sum) = *(unsigned char *)w;
		sum += check_sum;
	}
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	check_sum = ~sum;	//ȡ���õ�У���
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

/*����ICMP��ͷ*/
int pack(unsigned short pid,unsigned char * buf,int len)
{
	struct icmp_echo_hdr *icmp;
	time_t * tval;
	RT_ASSERT(len>= PINGDATALEN);
	icmp = (struct icmp_echo_hdr*)buf;
	icmp->type = ICMP_ECHO;	//ICMP_ECHO���͵����ͺ�Ϊ0
	icmp->code = 0;
	icmp->chksum = 0;
	icmp->seqno = pid;	//���͵����ݱ����
	icmp->id = pid;

	tval = (time_t  *)(buf + sizeof(struct icmp_echo_hdr));
	time(tval);		//��¼����ʱ��
	//У���㷨
	icmp->chksum =  cal_chksum((unsigned short *)icmp,PINGDATALEN);	
	return PINGDATALEN;		//���ݱ���СΪ64�ֽ�
}

/****��������ICMP����****/
void send_packet(int sockfd,struct sockaddr_in * pdest_addr,unsigned char * buf,int len)
{
	int packetsize;	
	static int pid = 1;
	packetsize = pack(pid ++,buf,len);	//����ICMP��ͷ
	//�������ݱ�
	if(sendto(sockfd,buf,packetsize,0,
		(struct sockaddr *)pdest_addr,sizeof(struct sockaddr_in)) < 0)
	{
		rt_kprintf("sendto error");
	}
}

/******��ȥICMP��ͷ******/
int unpack(unsigned char *buf,int len,unsigned short pid,struct sockaddr_in * from)
{
	int iphdrlen;		//ipͷ����
	struct ip_hdr *ip;
	struct icmp_echo_hdr *icmp;
	time_t *tvsend;
	time_t tvrecv,rtt;

	time(&tvrecv);
	ip = (struct ip_hdr *)buf;
	iphdrlen = ip->_len << 2;	//��IP����ͷ���ȣ���IP��ͷ���ȳ�4
	icmp = (struct icmp_echo_hdr *)(buf + iphdrlen);	//Խ��IPͷ��ָ��ICMP��ͷ
	len -= iphdrlen;	//ICMP��ͷ�����ݱ����ܳ���
	if(len < 8)		//С��ICMP��ͷ�ĳ����򲻺���
	{
		rt_kprintf("ICMP packet\'s length is less than 8\n");
		return -1;
	}
	//ȷ�������յ���������ICMP�Ļ�Ӧ
	if((icmp->type == ICMP_ER) && (icmp->id == pid))
	{
		tvsend = (time_t *)(buf + sizeof(struct icmp_echo_hdr));
		rtt = tvrecv - *tvsend; 
		//all_time += rtt;	//��ʱ��
		//��ʾ��ص���Ϣ
		rt_kprintf("%d bytes from %s: icmp_seq=%u ttl=%d time=%ds\n",
				len,inet_ntoa(from->sin_addr),
				icmp->seqno,ip->_proto,rtt);
	}
	else 
		return -1;
	return 0;
}

/****��������ICMP����****/
int recv_packet(int sockfd,struct sockaddr_in * pdest_addr,unsigned char * buf,int len)
{
	u32_t fromlen;
	//static int pid = 1;
	struct sockaddr from;
	fromlen = sizeof(from);
	//�������ݱ�
	if((recvfrom(sockfd,buf,len,MSG_DONTWAIT,
		(struct sockaddr *)&from,&fromlen)) < 0)
	{
		rt_kprintf("recvfrom error");
		return -1;
	}
	//gettimeofday(&tvrecv,NULL);		//��¼����ʱ��
	//unpack(buf,len,pid++,(struct sockaddr_in *)&from);		//��ȥICMP��ͷ
	return 0;
}

/*������*/
int ping(const char* peeraddr)
{
	struct hostent *host;
	int sockfd;
	struct sockaddr_in dest_addr;
	int ret = 0;
//	int waittime = MAX_WAIT_TIME;
//	int size = 50 * 1024;
	
	unsigned char buf[PINGDATALEN]={0};

	//����ʹ��ICMP��ԭʼ�׽��֣�ֻ��root��������
	if((sockfd = socket(AF_INET,SOCK_RAW,IP_PROTO_ICMP)) < 0)
	{
		rt_kprintf("socket error");
		return 1;
	}

	rt_memset(&dest_addr,0,sizeof(dest_addr));	//��ʼ��
	dest_addr.sin_family = AF_INET;		//�׽�������AF_INET(�����׽���)

	//�ж��������Ƿ���IP��ַ
	if(inet_addr(peeraddr) == INADDR_NONE)
	{
		if((host = gethostbyname(peeraddr)) == NULL)	//��������
		{
			rt_kprintf("gethostbyname error");
			return -1;
		}
		memcpy((char *)&dest_addr.sin_addr,host->h_addr,host->h_length);
	}
	else{ //��IP ��ַ
		dest_addr.sin_addr.s_addr = inet_addr(peeraddr);
	}
	//rt_kprintf("PING %s(%s):%d bytes of data.\n",peeraddr,
	//		inet_ntoa(dest_addr.sin_addr),PINGDATALEN);

	
	send_packet(sockfd,&dest_addr,buf,PINGDATALEN);		//����ICMP����
	rt_thread_delay(RT_TICK_PER_SECOND);		//�ȴ�һ��
	ret = recv_packet(sockfd,&dest_addr,buf,PINGDATALEN);		//����ICMP����
	

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
