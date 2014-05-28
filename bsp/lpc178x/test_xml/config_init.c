#include <rtthread.h>
#include "mcbxml.h"
#include <dfs_fs.h>
#include <dfs_posix.h>
#include <stdint.h>	  
#include <lwip/sys.h>
#include <lwip/api.h>
#include "config_init.h"
#include "merror.h"
#include <finsh.h>


struct ip_addr g_ipaddr, g_netmask, g_gw;
int g_rsid,g_rfid;


m_node * g_pRootNode = RT_NULL;
m_node * g_pSelfNode = RT_NULL;

/*static int ishexdigit(char ch)
{
	if(ch <= '9' && ch >= '0')
		return 1;
	else if(ch <= 'F' && ch >= 'A')
		return 1;
	else if(ch <= 'f' && ch >= 'a')
		return 1;
	return 0;
} */

/*
static unsigned int atohex(const char *s)
{
	int rv = 0;
	if(*s == '\"')
		s++;
	while (*s && ishexdigit(*s) )
	{
		int v = *s - '0';
		if (v > 9)
			v -= 7;
		v &= 0xf; 
		rv = rv*16 + v;
		s++;
	}
	return rv;
}  */

static unsigned int atoint(const char *s)
{
	int rv = 0;
	if(*s == '\"')
		s++;
	while (*s && isdigit(*s) )
	{
		int v = *s - '0';
		v &= 0xf; /* in case it's lower case */
		rv = rv*10 + v;
		s++;
	}
	return rv;
}

//get the real root element (not declaration node <?xml .... ?>
static McbXMLElement * getRealRoot(McbXMLElement * pRoot,const char * needEleName)
{
	McbXMLElement * pSubEntry = RT_NULL;
	int i = 0;
	RT_ASSERT(pRoot);
	RT_ASSERT(pRoot->nSize == 1);
	pSubEntry = pRoot->pEntries[0].node.pElement;
		
	for(i = 0 ; i < pSubEntry->nSize; i++)
	{
	 	if(pSubEntry->pEntries[i].type == eNodeElement && 
			rt_strcmp(pSubEntry->pEntries[i].node.pElement->lpszName,needEleName) == 0)
			return pSubEntry->pEntries[i].node.pElement;
	}
	return RT_NULL;
}


//try to open and parse a xml file 
//parameter
//	filename : xml file name 
//	ppXMLRoot : pointer to save the Root Element Pointer
//return
//	ME_RESULT  0 for success else failed
static int parse_xml(const char * filename,McbXMLElement ** ppXMLRoot)
{
	McbXMLElement * pXMLRoot = RT_NULL;
	int ret = -1, fdxml, readlen = 0;
	ME_RESULT result = ME_UNKNOWN;
	McbXMLResults xmlError={eXMLErrorNone,0,0};
	char * pBuffer = RT_NULL;
	struct stat  tmpstat={0};

	ret = stat(filename,&tmpstat);

	if(ret < 0 || tmpstat.st_size == 0)
		return ME_FILENOTFOUND;

	pBuffer = rt_malloc(tmpstat.st_size + 1);

	if(pBuffer == RT_NULL)
		return ME_MEMORYLEAK;

	rt_memset(pBuffer,0,tmpstat.st_size + 1);

	fdxml = open(filename,O_RDONLY,0);

	if(fdxml < 0)
		return ME_FILEOPENFAILED;
	
	readlen = read(fdxml,pBuffer,tmpstat.st_size);
	
	if(readlen < tmpstat.st_size)
	{
		result = ME_FILEREADFAILED;
		goto errout;
	}
	close(fdxml);
	pXMLRoot = McbParseXML(pBuffer,&xmlError);
	if(pXMLRoot == NULL)
	{
		result = ME_PARSEXML;
		rt_kprintf("parse xml failed for errorcode %d ,line %d ,column %d\n",xmlError.error,xmlError.nLine,xmlError.nColumn);
		goto errout;
	}
	if(ppXMLRoot)
		*ppXMLRoot = pXMLRoot;
	result = ME_OK;
errout:
	rt_free(pBuffer);
	pBuffer = RT_NULL;
	return result;
}


ME_RESULT config_init(void)
{
	ME_RESULT result = ME_UNKNOWN;
	McbXMLElement * pXMLRoot = RT_NULL , * pRealRoot = RT_NULL;		
	McbXMLNode * pChild = RT_NULL;
	int nIndex = 0;//,point_position; 	  
	//char *pfile_name=RT_NULL;
	//rt_ubase_t str_len;
	
    IP4_ADDR(&g_ipaddr, RT_LWIP_IPADDR0, RT_LWIP_IPADDR1, RT_LWIP_IPADDR2, RT_LWIP_IPADDR3);
    IP4_ADDR(&g_gw, RT_LWIP_GWADDR0, RT_LWIP_GWADDR1, RT_LWIP_GWADDR2, RT_LWIP_GWADDR3);
    IP4_ADDR(&g_netmask, RT_LWIP_MSKADDR0, RT_LWIP_MSKADDR1, RT_LWIP_MSKADDR2, RT_LWIP_MSKADDR3);
	g_rsid = 0xfe;
	g_rfid = 0xfe;
#ifdef RT_USING_DFS
	rt_kprintf("parse Configuration: config.xml\n");
	result = parse_xml("/config.xml",&pXMLRoot);
	if(ME_FAILED(result))
		rt_kprintf("Parse XML File: %s Failed for errcode: 0x%x\n","/config.xml",result);
	else
	{
		pRealRoot = getRealRoot(pXMLRoot,"config");
		if(pRealRoot)
		{
			if(rt_strcmp(pRealRoot->lpszName,"config")!=0)
			{
				rt_kprintf("Get Bad Root Element %s,but need \"config\"	\n",pRealRoot->lpszName);
				return ME_BADELEMENT;
			}
			nIndex = 0; 	
			while((pChild = McbEnumNodes(pRealRoot,&nIndex))!=0)
			{
				McbXMLAttribute * pAttr = NULL;
				char valuestr[64]={0};
				if(pChild->type != eNodeAttribute)
					continue;
				pAttr = pChild->node.pAttrib;
				memset(valuestr,0,sizeof(valuestr));
				if(rt_strcmp(pAttr->lpszName,"mac")==0)
				{
					rt_kprintf("mac:%s\n",pAttr->lpszValue);
				}
				else if(rt_strcmp(pAttr->lpszName,"gateway")==0)
				{
					rt_kprintf("gateway:%s\n",pAttr->lpszValue);
					strcpy(valuestr,pAttr->lpszValue+1);
					if(strlen(valuestr)>1)
						valuestr[strlen(valuestr)-1]=0;
					g_gw.addr = ipaddr_addr(valuestr);
				}
				else if(rt_strcmp(pAttr->lpszName,"netmask")==0)
				{
					rt_kprintf("netmask:%s\n",pAttr->lpszValue);
					strcpy(valuestr,pAttr->lpszValue+1);
					if(strlen(valuestr)>1)
						valuestr[strlen(valuestr)-1]=0;
					g_netmask.addr = ipaddr_addr(valuestr);
				}
				else if(rt_strcmp(pAttr->lpszName,"ip")==0)
				{
					rt_kprintf("ip:%s\n",pAttr->lpszValue);
					strcpy(valuestr,pAttr->lpszValue+1);
					if(strlen(valuestr)>1)
						valuestr[strlen(valuestr)-1]=0;
					g_ipaddr.addr = ipaddr_addr(valuestr);
					g_rsid = (g_ipaddr.addr & 0xff000000) >> 24;
					g_rfid = g_rsid;
				}
				else if(rt_strcmp(pAttr->lpszName,"type")==0)
				{
					rt_kprintf("type:%s\n",pAttr->lpszValue);
				}
				else if(rt_strcmp(pAttr->lpszName,"rfid")==0)
				{										 
					rt_kprintf("rfid:%s\n",pAttr->lpszValue);
					strcpy(valuestr,pAttr->lpszValue+1);
					if(strlen(valuestr)>1)
						valuestr[strlen(valuestr)-1]=0;
					g_rfid = atoi(valuestr);
				}
				else if(rt_strcmp(pAttr->lpszName,"rsid")==0)
				{
					rt_kprintf("rsid:%s\n",pAttr->lpszValue); 
					strcpy(valuestr,pAttr->lpszValue+1);
					if(strlen(valuestr)>1)
						valuestr[strlen(valuestr)-1]=0;
					g_rsid = atoi(valuestr);
				} 
			}
			McbDeleteRoot(pXMLRoot);
		}
	}
#else


#endif		 	
	return ME_OK;
}

int isRoot(m_node * pNode)
{
 	return pNode == g_pRootNode;
}

ME_RESULT parse_node(m_node ** ppNode,McbXMLElement * pElem,m_node * pUpNode)
{			 
	m_node * pNewNode = RT_NULL,*pLastChild = RT_NULL;
	int childindex = 0;
	McbXMLElement * pChild = RT_NULL;
	McbXMLAttribute * pAttr = RT_NULL;
	RT_ASSERT(ppNode);
	if(pElem == RT_NULL){
	 	*ppNode = RT_NULL;
		return ME_BADARG;
	}
 	if(*ppNode == RT_NULL){
	 	pNewNode = rt_malloc(sizeof(m_node));
		RT_ASSERT(pNewNode );
		*ppNode = pNewNode;
		memset(pNewNode,0,sizeof(m_node));
		pNewNode->up = pUpNode;		
	} 
	
	while(pChild = McbEnumElements(pElem,&childindex)) {
		m_node * pCurChild  = RT_NULL;
	 	parse_node(&pCurChild,pChild,pNewNode);
		if(pNewNode->down_list == RT_NULL){
		 	pNewNode->down_list = pCurChild;
			pLastChild = pCurChild;
		}
		else{
		 	pLastChild->next_sibling = pCurChild;
			pLastChild = pCurChild;
		} 
	}
	childindex = 0 ;
	while(pAttr = McbEnumAttributes(pElem,&childindex)){
		if(rt_strcasecmp(pAttr->lpszName,"ip")==0){
			pNewNode->ip = atoint(pAttr->lpszValue);
			if(pNewNode->ip == g_rfid){
			 	g_pSelfNode = pNewNode;
			}
		}
		if(rt_strcasecmp(pAttr->lpszName,"upchannel")==0){
			pNewNode->upchannel = atoint(pAttr->lpszValue);
		}
	}
	return ME_OK;
}

m_node_t up_sibling(m_node_t pcur)
{
	if(pcur->next_sibling)
		return pcur->next_sibling;
	if(pcur->up){
	 	return up_sibling(pcur->up);
	} 	
	return RT_NULL;
}

m_node_t next_node(m_node_t pcur)
{
	if(pcur==RT_NULL)
		return RT_NULL;
 	if(pcur->down_list){
	 	return pcur->down_list;
	}
	if(pcur->next_sibling){
	 	return pcur->next_sibling;
	}
	if(pcur->up){
	 	return up_sibling(pcur->up);
	}
	return RT_NULL;
}

ME_RESULT topoligy_init(void)
{
	ME_RESULT result = ME_UNKNOWN;
	McbXMLElement * pXMLRoot = RT_NULL , * pRealRoot = RT_NULL;		
	McbXMLNode * pChild = RT_NULL;
	int nIndex = 0;
	
#ifdef RT_USING_DFS
	rt_kprintf("parse Configuration: topology.xml\n");
	result = parse_xml("/topology.xml",&pXMLRoot);
	if(ME_FAILED(result)){
		rt_kprintf("Parse XML File: %s Failed for errcode: 0x%x\n","/topology.xml",result);
	}
	else
	{
		pRealRoot = getRealRoot(pXMLRoot,"topology");
		if(pRealRoot)
		{
			if(rt_strcmp(pRealRoot->lpszName,"topology")!=0)
			{
				rt_kprintf("Get Bad Root Element %s,but need \"topology\"	\n",pRealRoot->lpszName);
				return ME_BADELEMENT;
			}
			nIndex = 0; 	
			while((pChild = McbEnumNodes(pRealRoot,&nIndex))!=0)
			{
				McbXMLElement * pElem = NULL;
				if(pChild->type != eNodeElement)
					continue;
				pElem = pChild->node.pElement;
				parse_node(&g_pRootNode,pElem,g_pRootNode);
				
			}
			McbDeleteRoot(pXMLRoot);
		}
	}
#else


#endif		 	
	return ME_OK;
}


long pconf(void)
{
	rt_kprintf("ip:%s\n",ipaddr_ntoa(&g_ipaddr));
	rt_kprintf("netmask:%s\n",ipaddr_ntoa(&g_netmask));
	rt_kprintf("gateway:%s\n",ipaddr_ntoa(&g_gw));
	rt_kprintf("rfid:0x%x\n",g_rfid);
	rt_kprintf("rsid:0x%x\n",g_rsid);

	return 0;
}
void printlevel(int level)
{
 	int i= 0 ;
	for(;i<level;i++){
	 	rt_kprintf("  ");
	}
}
void printnode(m_node * pNode,int level)
{
	printlevel(level);
	rt_kprintf("node ip %d upchannel %d\n",pNode->ip,pNode->upchannel);
	if(pNode->up){
		printlevel(level);
		rt_kprintf("upnode ip %d\n",pNode->up->ip);
	}
	if(pNode->down_list){
	 	printlevel(level);
		rt_kprintf("down node list:\n");
		printnode(pNode->down_list,level+1);
	}
	if(pNode->next_sibling){
	 	printlevel(level);
		rt_kprintf("next sibling:\n");
		printnode(pNode->next_sibling,level+1);
	}
 	return;
}

int find_up_node(m_node_t pstartnode,int tarid)
{
	//int isFind = 0;
	m_node_t pfind = pstartnode->up;
	while(pfind!= RT_NULL){
		if(pfind->ip == tarid){
		 	return 1;
		}
	 	pfind = pfind->up;
	}
	return 0;
}

m_node_t find_down_node(m_node_t pstartnode,int tarid)
{
	m_node_t pfind = pstartnode->down_list;
	while(pfind!=RT_NULL){
		m_node_t pcur = pfind;
	 	if(pfind->ip == tarid)
			return pfind;
		pcur = find_down_node(pcur,tarid);
		if(pcur){
		 	return pfind;
		}
		pfind = pfind->next_sibling;
	}
	return pfind;
}	

long find_route(char * ptarip)
{
 	int tarip = ipaddr_addr(ptarip);
	int tarid = (tarip >> 24) & 0xff;
	m_node_t pfind = RT_NULL;
	//int isFind = 0;
	rt_kprintf("tarip 0x%x,tarid %d\n",tarip,tarid);
	if(g_pSelfNode==RT_NULL){
	 	rt_kprintf("self node unset !!!!!!\n");
		return -1;
	}
	if((tarip & 0x00ffffff) != (g_ipaddr.addr & 0x00ffffff)){
	 	rt_kprintf("bad target not in top map\n");
		return -1;
	}
	if(g_pSelfNode->ip == tarid){
	 	rt_kprintf("is my self ip\n");
		return 0;
	}
	if(g_pSelfNode == g_pRootNode){
	 	rt_kprintf("god ,I am PC!!!\n");
		return -1;
	}
	
	if(find_up_node(g_pSelfNode,tarid)){
	 	rt_kprintf("find upchannel %d next route id %d\n",g_pSelfNode->upchannel,g_pSelfNode->up->ip);
		return 0;
	}
	pfind = find_down_node(g_pSelfNode,tarid);
	if(pfind){
		rt_kprintf("find downchannel %d next route id %d\n",pfind->upchannel,pfind->ip);
	}
	else
		rt_kprintf("no route to %d\n",tarid);
	return 0;
}

long tranodes(void)
{
	if(g_pRootNode == RT_NULL)
		return -1;
	printnode(g_pRootNode,0);
	if(g_pSelfNode != RT_NULL){
		rt_kprintf("self ip %d,up node ip %d ,self upchannel %d\n",g_pSelfNode->ip,g_pSelfNode->up?g_pSelfNode->up->ip:0,g_pSelfNode->upchannel);
	}
	else{
	 	rt_kprintf("self node unmatch!!!!!\n");
	}
	
 	return 0;
}

#ifdef RT_USING_FINSH
FINSH_FUNCTION_EXPORT(pconf,print global configurateion);
FINSH_FUNCTION_EXPORT(tranodes,travels all mine nodes);
FINSH_FUNCTION_EXPORT(find_route,find target channel);
#endif


