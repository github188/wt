#include "stmp_header.h"


/** 
 * @func  sendemail
 * @brief send email in blocking-mode
 * @param smtpServer 
 * @param body 
 */
void send_email(char *smtpServer, char *body, char *from_addr, char* from_passwd, char* to_qq)
{
	int sockfd = 0;
	struct sockaddr_in their_addr = {0};
	char buf[1500] = {0};
	char rbuf[1500] = {0};
	char login[128] = {0};
	char pass[128] = {0};
	struct hostent *host = NULL;

	// 取得主机IP地址
	if((host = gethostbyname(smtpServer))==NULL){
		fprintf(stderr,"Gethostname error, %s\n", strerror(errno));
		exit(1);
	}

	memset(&their_addr, 0, sizeof(their_addr));
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(25);
	their_addr.sin_addr = *((struct in_addr *)host->h_addr);

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		xyprintf(0, "STMP_ERROR:%s %d -- Open sockfd(TCP ) error!", __FILE__, __LINE__);
		return ;
	}

	if(connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) < 0){
		close(sockfd);
		xyprintf(0, "STMP_ERROR:%s %d -- Connect sockfd(TCP ) error!", __FILE__, __LINE__);
		return ;
	}

	memset(rbuf, 0, 1500);
	if(recv(sockfd, rbuf, 1500, 0) == 0){
		close(sockfd);
		xyprintf(0, "STMP_ERROR:%s %d -- ");
		return ;
	}

	memset(buf, 0, 1500);

	/* EHLO */
	sprintf(buf, "EHLO abcdefg-PC\r\n");
	send(sockfd, buf, strlen(buf), 0);
	xyprintf(0, " --> %s", buf);
	
	memset(rbuf, 0, 1500);
	recv(sockfd, rbuf, 1500, 0);
	xyprintf(0, " <-- %s", rbuf);
	
	memset(buf, 0, 1500);
	
	/*AUTH LOGIN  */
	sprintf(buf, "AUTH LOGIN\r\n");
	send(sockfd, buf, strlen(buf), 0);
	xyprintf(0, " --> %s", buf);

	memset(rbuf, 0, 1500);
	recv(sockfd, rbuf, 1500, 0);
	xyprintf(0, " <-- %s", rbuf);

	memset(buf, 0, 1500);
	
	/* USER */

	sprintf(buf, "%s", from_addr);
	memset(login, 0, 128);
	base64_encode(buf, strlen(buf), login, 128);				   /* base64 */
	xyprintf(0, " --> %s", buf);
	sprintf(buf, "%s\r\n", login);
	send(sockfd, buf, strlen(buf), 0);
	xyprintf(0, " --> %s", buf);

	memset(rbuf, 0, 1500);
	recv(sockfd, rbuf, 1500, 0);
	xyprintf(0, " <-- %s", rbuf);

	/* PASSWORD */
	memset(buf, 0, 1500);
	sprintf(buf, "%s", from_passwd);
	memset(pass, 0, 128);

	base64_encode(buf, strlen(buf), pass, 128);
	memset(buf, 0, 1500);
	xyprintf(0, " --> %s", buf);
	sprintf(buf, "%s\r\n", pass);
	send(sockfd, buf, strlen(buf), 0);
	xyprintf(0, " --> %s", buf);

	memset(rbuf, 0, 1500);
	recv(sockfd, rbuf, 1500, 0);
	xyprintf(0, " <-- %s", rbuf);

	memset(buf, 0, 1500);
	
	/* MAIL FROM */
	sprintf(buf, "MAIL FROM: <%s>\r\n", from_addr);
	send(sockfd, buf, strlen(buf), 0);
	xyprintf(0, " --> %s", buf);

	memset(rbuf, 0, 1500);
	recv(sockfd, rbuf, 1500, 0);
	xyprintf(0, " <-- %s", rbuf);

	/* rcpt to 第一个收件人 */
	sprintf(buf, "RCPT TO:<%s@qq.com>\r\n", to_qq);
	send(sockfd, buf, strlen(buf), 0);
	xyprintf(0, " --> %s", buf);

	memset(rbuf, 0, 1500);
	recv(sockfd, rbuf, 1500, 0);
	xyprintf(0, " <-- %s", rbuf);

	/* DATA email connext ready  */
	sprintf(buf, "DATA\r\n");
	send(sockfd, buf, strlen(buf), 0);
	xyprintf(0, " --> %s", buf);

	memset(rbuf, 0, 1500);
	recv(sockfd, rbuf, 1500, 0);
	xyprintf(0, " <-- %s", rbuf);

	/* send email connext \r\n.\r\n end*/
	send(sockfd, body, strlen(body), 0);
	//xyprintf(0, " --> %s", body);

	memset(rbuf, 0, 1500);
	recv(sockfd, rbuf, 1500, 0);
	xyprintf(0, " <-- %s", rbuf);

	/* QUIT */
	sprintf(buf, "QUIT\r\n");
	send(sockfd, buf, strlen(buf), 0);
	xyprintf(0, " --> %s", buf);
	
	memset(rbuf, 0, 1500);
	recv(sockfd, rbuf, 1500, 0);
	xyprintf(0, " <-- %s", rbuf);

	close(sockfd);

	return ;

}

/** 
 * @func  sendemail
 * @brief send email in blocking-mode
 * @param smtpServer 
 * @param body 
 */
void send_email_local(char *body, char *from_addr, char* to_qq)
{
	int sockfd = 0;
	char buf[1500] = {0};
	char rbuf[1500] = {0};

	struct sockaddr_in their_addr = {0};
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(25);
	their_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		xyprintf(0, "STMP_ERROR:%s %d -- Open sockfd(TCP ) error!", __FILE__, __LINE__);
		return ;
	}

	if(connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) < 0){
		close(sockfd);
		xyprintf(0, "STMP_ERROR:%s %d -- Connect sockfd(TCP ) error!", __FILE__, __LINE__);
		return ;
	}

	memset(rbuf, 0, 1500);
	if(recv(sockfd, rbuf, 1500, 0) == 0){
		close(sockfd);
		xyprintf(0, "STMP_ERROR:%s %d -- ");
		return ;
	}

	memset(buf, 0, 1500);
	/* MAIL FROM */
	sprintf(buf, "MAIL FROM: <%s>\r\n", from_addr);
	send(sockfd, buf, strlen(buf), 0);
	xyprintf(0, " --> %s", buf);

	memset(rbuf, 0, 1500);
	recv(sockfd, rbuf, 1500, 0);
	xyprintf(0, " <-- %s", rbuf);

	/* rcpt to 第一个收件人 */
	sprintf(buf, "RCPT TO:<%s@qq.com>\r\n", to_qq);
	send(sockfd, buf, strlen(buf), 0);
	xyprintf(0, " --> %s", buf);

	memset(rbuf, 0, 1500);
	recv(sockfd, rbuf, 1500, 0);
	xyprintf(0, " <-- %s", rbuf);

	/* DATA email connext ready  */
	sprintf(buf, "DATA\r\n");
	send(sockfd, buf, strlen(buf), 0);
	xyprintf(0, " --> %s", buf);

	memset(rbuf, 0, 1500);
	recv(sockfd, rbuf, 1500, 0);
	xyprintf(0, " <-- %s", rbuf);

	/* send email connext \r\n.\r\n end*/
	send(sockfd, body, strlen(body), 0);
	//xyprintf(0, " --> %s", body);

	memset(rbuf, 0, 1500);
	recv(sockfd, rbuf, 1500, 0);
	xyprintf(0, " <-- %s", rbuf);

	/* QUIT */
	sprintf(buf, "QUIT\r\n");
	send(sockfd, buf, strlen(buf), 0);
	xyprintf(0, " --> %s", buf);
	
	memset(rbuf, 0, 1500);
	recv(sockfd, rbuf, 1500, 0);
	xyprintf(0, " <-- %s", rbuf);

	close(sockfd);

	return ;

}

