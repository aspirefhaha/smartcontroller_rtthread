

#define ZLCBSERVER	"zlcb.8866.org"

#define ZLCBPort	8080U

int http_post(const char * server,unsigned short port,const char * url,const char * data,int datalen);
int http_put(const char * server,unsigned short port,const char * url,const char * data,int datalen);
int http_get(const char * server,unsigned int port,const char * url,const char * filename,char ** retbuf);
