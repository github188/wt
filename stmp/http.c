#include "stmp_header.h"

/** 
 *@brief  获取邮件内容函数
 *@return 长度
 */
int get_http( char* address, int port, char* path ,char *buf, int buf_len )
{
	// 取得主机IP地址
	struct hostent *host = NULL;
	if((host = gethostbyname(address))==NULL){
		xyprintf(0, "SOCK_ERROR:%s %d -- Gethostname error, %s", __FILE__, __LINE__, strerror(errno));
		return -1;
	}
    
	int sockfd;
    struct sockaddr_in addr;
	
	if(( sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		xyprintf(0, "SOCK_ERROR:%s %d -- socket()", __FILE__, __LINE__);
		return -1;
	}

	addr.sin_family	= AF_INET;
	addr.sin_port	= htons( port );
	addr.sin_addr	= *((struct in_addr *)host->h_addr);
	
	if( connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1){
		xyprintf(0, "SOCK_ERROR:%d %s %d -- connect()", sockfd, __FILE__, __LINE__);
		return -1;
	}
	
	char        sndBuf[1024] = {0};

	// HTTP 消息构造开始，这是程序的关键之处
    sprintf(sndBuf, "GET %s HTTP/1.1\r\nAccept: */*\r\n"
			"Accept-Language: zh-cn\r\n"
			"User-Agent: Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)\r\n"
			"Host: %s\r\n"
			"Connection: Close\r\n\r\n", path, address);
      
    // 发送HTTP请求消息
    send(sockfd, (char*)sndBuf, strlen(sndBuf) + 1, 0);

	int num, pRcv = 0;

    // 接收HTTP响应消息
    while(1){
		num = recv(sockfd, buf + pRcv, buf_len - pRcv, 0);
        pRcv += num;

		if(buf_len == pRcv){
			xyprintf(0, "GET_HTTP:%s %d -- To get the page is too long!!", __FILE__, __LINE__);
		}
 
		if((0 == num) || (-1 == num)){
			break;
        }
    }
      
    // 打印响应消息
    //printf(" ### buf\n%s\n", buf);
 
	return pRcv;
}
