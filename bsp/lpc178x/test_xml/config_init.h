#ifndef CONFIG_INIT_H
#define CONFIG_INIT_H
#include "merror.h"

ME_RESULT config_init(void);
ME_RESULT topoligy_init(void);

extern struct ip_addr g_ipaddr, g_netmask, g_gw;
extern int g_rsid,g_rfid;

typedef struct _m_node {
	struct _m_node * up;
	struct _m_node * down_list;
	struct _m_node * next_sibling ;
	int ip;
	int upchannel;
} m_node,*m_node_t;

extern m_node * g_pRootNode;
extern m_node * g_pSelfNode;	  

m_node_t next_node(m_node_t pcur);

#endif

