/*****************************************************
 *
 * 工具函数
 *
 *****************************************************/

#include "header.h"

/** 
 *@brief  16字节数据的网络序和本地序互转
 *@param  num		类型 __u16*    要转换数据的地址
 *@return 无
 */
void inline big_little16(__u16 *num)
{
	*num = ((*num & 0xff00) >> 8) | ((*num & 0x00ff) << 8); 
}

/** 
 *@brief  32字节数据的网络序和本地序互转
 *@param  num		类型 __u32*    要转换数据的地址
 *@return 无
 */
void inline big_little32(__u32 *num)
{
	*num =	((*num & 0xff000000) >> 24) |
			((*num & 0x00ff0000) >> 8)  |
			((*num & 0x0000ff00) << 8)  |
			((*num & 0x000000ff) << 24); 
}

/** 
 *@brief  数字ip地址转字符串
 *@param  cip		类型 char*    转换后ip地址字符串存放位置
 *@param  sip		类型 __u32    要转换的ip地址
 *@return 无
 */
void ip_to_str(char* cip, __u32 sip)
{
	__u8 * ip_dot;
	__u32 ip_dot1,ip_dot2,ip_dot3,ip_dot4;
	
	ip_dot=(char*)&sip;
	ip_dot1 = ip_dot[0];
	ip_dot2 = ip_dot[1];
	ip_dot3 = ip_dot[2];
	ip_dot4 = ip_dot[3];
	
	//ip_dot1=ip_dot[3];
	//ip_dot2=ip_dot[2];
	//ip_dot3=ip_dot[1];
	//ip_dot4=ip_dot[0];
	
	
	sprintf(cip,"%u.%u.%u.%u",ip_dot1,ip_dot2,ip_dot3,ip_dot4);
}

/** 
 *@brief  字符类型转换
 *@param  from_charset		类型 char*		源字符串的格式类型
 *@param  to_charset		类型 char*		目标字符串的格式类型
 *@param  inbuf				类型 char*		源字符串地址
 *@param  inlen				类型 size_t		源字符串长度？？？？
 *@param  outbuf			类型 char*		目标字符串地址
 *@param  outlen			类型 size_t		目标字符串长度？？？？
 *@return failed -1 success 0
 */
int code_convert(char *from_charset, char *to_charset, char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
	iconv_t cd;
	int rc;
	char **pin = &inbuf;
	char **pout = &outbuf;
	
	cd = iconv_open(to_charset, from_charset);
	if (cd == 0){
		xyprintf(errno, "OTHER_ERROR:%s %s %d -- iconv_open() failed!", __func__, __FILE__, __LINE__);
		return -1;
	}
	
	memset(outbuf, 0, outlen);

	if (iconv(cd, pin, &inlen, pout, &outlen) == -1){
		xyprintf(errno, "OTHER_ERROR:%s %s %d -- iconv() failed!", __func__, __FILE__, __LINE__);
		return -1;
	}

	iconv_close(cd);
	return 0;
}







/** 
 *@brief  打印报文头信息
 *@param  msg_head		类型 msg_head_st*	要打印的报文头
 *@return 无
 */
void inline xyprintf_msg_head(msg_head_st *msg_head)
{
	xyprintf(0, "msg_head->ver = 0x%x\n\
			->mode = 0x%x\n\
			->order = 0x%x\n\
			->len = 0x%x\n\
			->sec_flag = 0x%x\n\
			->crc = 0x%x\n\
			->router_id = 0x%x\n\
			->hard_seq = %s",
		(int)msg_head->ver,
		(int)msg_head->mode,
		msg_head->order,
		msg_head->len,
		msg_head->sec_flag,
		msg_head->crc,
		msg_head->router_id,
		msg_head->hard_seq);
}

/** 
 *@brief  打印认证报文信息
 *@param  msg		类型 cer_msg_head_st*	认证报文
 *@return 无
 */
void inline xyprintf_cer_msg_head(cer_msg_st *msg)
{
	xyprintf(0, "msg->order = 0x%x\n\
			->srv_type = 0x%x\n\
			->pad = 0x%x\n\
			->route_seq = 0x%x\n\
			->srv_seq = 0x%x\n\
			->time = 0x%x\n\
			->hard_id = 0x%x\n\
			->hard_seq = %s\n\
			->router_addr = %s\n\
			->router_cont = %s\n\
			->router_tel = %s\n\
			->router_dinate = %s\n\
			->router_agents = %s\n\
			->dev_name = %s\n\
			->auth_srv = %s\n\
			->sj_srv = %s\n\
			->mng_srv = %s\n\
			->def_redirect = %s\n\
			->cs_type = 0x%x\n\
			->url_len = 0x%x\n\
			->url_data = %s",
			msg->order,
			(int)msg->srv_type,
			(int)msg->pad,
			msg->router_seq,
			msg->srv_seq,
			msg->time,
			msg->hard_id,
			msg->hard_seq,
			msg->router_addr,
			msg->router_cont,
			msg->router_tel, 
			msg->router_dinate,
			msg->router_agents,
			msg->dev_name,
			msg->auth_srv,
			msg->sj_srv,
			msg->mng_srv,
			msg->def_redirect,
			msg->cs_type,
			msg->url_len,
			msg->url_data);
}

/**
 *@bried  打印集中管理平台地址下发报文
 *@param  msg		类型 addr_set_st*	设备与管理平台之间的状态包
 *@return 无
 */
void xyprintf_addr_set(addr_set_st* msg)
{
	xyprintf(0, "addr_set_st->addr = %s\n\
			.port = %u",
			msg->addr,
			msg->port);
}

















void xyprintf_bg_msg_head(bg_msg_head *msg)
{
	xyprintf(0, "bg_msg_head->ver = 0x%x\n\
			->head = 0x%x\n\
			->order = 0x%x\n\
			->len = %d\n\
			->version = %s",
			msg->ver,
			msg->head,
			msg->order,
			msg->len,
			msg->version);
}

void xyprintf_bg_num_msg(bg_num_msg *msg)
{
	xyprintf(0, "bg_num_msg->php_sockfd = %d\n\
			->guide_flag = %d\n\
			->heart_interval = %d\n\
			->num_flag = 0x%x\n\
			->total_num = %u\n\
			->rt_num = %u",
			msg->php_sockfd,
			msg->guide_flag,
			msg->heart_interval,
			msg->num_flag,
			msg->total_num,
			msg->rt_num);
	int i = 0;
	for(; i < msg->rt_num; i++){
		xyprintf(0, "%d:name = %s -- %u",
				i, msg->rts[i].name, msg->rts[i].num);
	}
}

void xyprintf_bg_router_msg(bg_router_msg *msg)
{
	xyprintf(0, "bg_router_msg->order = %d\n\
			->type = %s\n\
			->sn = %s",
			msg->order,
			msg->type,
			msg->sn);
}
