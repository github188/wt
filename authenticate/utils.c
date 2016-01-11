/*****************************************************
 *
 * 工具函数
 *
 *****************************************************/

#include "auth_header.h"

/** 
 *@brief  16进制字符转换数字
 *@param  num		类型 char    要转换16进制字符
 *@return 无
 */
static int char2x(const char ch){
	if(ch >= '0' && ch <= '9'){
		return ch - '0';
	}
	switch(ch){
		case 'a':
		case 'A':
			return 10;
		case 'b':
		case 'B':
			return 11;
		case 'c':
		case 'C':
			return 12;
		case 'd':
		case 'D':
			return 13;
		case 'e':
		case 'E':
			return 14;
		case 'f':
		case 'F':
			return 15;
		defalut:
			return 0;
	}
}

/** 
 *@brief  mac地址字符串转换成16进制数据
 *@param  mac		类型 __u8*    转换后的16进制数据存放地址
 *@param  sql_mac	类型 char*    mac地址
 *@param  size		类型 int      mac地址长度
 *@return 无
 */
void string2x(__u8* mac, const char* sql_mac, int size){
	int i;
	for(i = 0; i < size; i++){
		mac[i] = char2x(sql_mac[2 * i]) * 16 + char2x(sql_mac[2 * i+1]);
	}
}

/** 
 *@brief  获取系统运行状态
 *@param  stat		类型 ag_msg_stat*	获取到的数据存放的地址
 *@return 无
 */
void get_stat(ag_msg_stat* stat)
{
    FILE *fd;
	char buff[256];
	char name[16];
	char unit[8];
	int i;
	unsigned int mem[ 5 ];
    
	fd = fopen ("/proc/meminfo", "r");
	for(i = 0; i < 5; i++){
		fgets (buff, sizeof(buff), fd);
		sscanf (buff, "%s %u %s", name, &(mem[i]), unit); 
	}

// /proc/meminfo文件行
#define TOTAL		0
#define MEMFREE		1
#define BUFFERS		2
#define CACHED		3
#define SWAPCACHED	4

	stat->total_mem		= mem[TOTAL] / 1024;
	stat->used_mem		= (mem[TOTAL] - mem[MEMFREE]) / 1024;
	stat->free_mem		= mem[MEMFREE] / 1024;
	stat->buffers_mem	= mem[BUFFERS] / 1024;
	stat->cached_mem	= mem[CACHED] / 1024;
	stat->swap			= mem[SWAPCACHED] / 1024;
   
	stat->cpu_usage		= 0;

	fclose(fd);     //关闭文件fd
}

/** 
 *@brief  打印用户报文信息
 *@param  msg			类型 user_msg_st*	用户报文
 *@return 无
 */
void inline xyprintf_user_msg_head(user_msg_st *msg)
{
	xyprintf(0, "user_msg_head->cmd = 0x%x\n\
			->result = 0x%x\n\
			->auth_len = 0x%x\n\
			->user_ip = 0x%x\n\
			->mac[0] = 0x%02x\n\
			->mac[1] = 0x%02x\n\
			->mac[2] = 0x%02x\n\
			->mac[3] = 0x%02x\n\
			->mac[4] = 0x%02x\n\
			->mac[5] = 0x%02x\n\
			->mac[6] = 0x%02x\n\
			->mac[7] = 0x%02x\n\
			->speed[0] = 0x%x\n\
			->speed[1] = 0x%x\n\
			->user_id = %s\n\
			->auth_time = 0x%x\n\
			->flow[0] = 0x%x\n\
			->flow[1] = 0x%x\n\
			->auth_addr = %s",
			(int)msg->cmd,
			(int)msg->result,
			msg->auth_len,
			msg->user_ip,
			msg->mac[0],
			msg->mac[1],
			msg->mac[2],
			msg->mac[3],
			msg->mac[4],
			msg->mac[5],
			msg->mac[6],
			msg->mac[7],
			msg->speed[0],
			msg->speed[1],
			msg->user_id,
			msg->auth_time,
			msg->flow[0],
			msg->flow[1],
			msg->auth_addr);
}

/** 
 *@brief  打印心跳报文信息
 *@param  msg		类型 heart_msg_st*	心跳报文
 *@return 无
 */
void inline xyprintf_heart_msg_head(heart_msg_st *msg)
{
	xyprintf(0, "heart_msg_head->router_id = 0x%x\n\
			->user_num = 0x%x",
			msg->router_id,
			msg->user_num);
}

/** 
 *@brief  打印白名单dns报文信息
 *@param  msg		类型 dns_white_list_st*	白名单DNS报文
 *@return 无
 */
void inline xyprintf_dns_msg_head(white_dns_msg_st *msg)
{
	xyprintf(0, "dns_white_list->url_len = 0x%x\n\
			->url_data = %s",
			msg->url_len,
			msg->url_data);
}

/** 
 *@brief  打印QQ数据报文信息
 *@param  msg		类型 third_qq_msg_st*	qq数据报文
 *@return 无
 */
void inline xyprintf_third_qq_head(third_qq_msg_st *msg)
{
	xyprintf(0, "third_qq_st->qq_num = %u\n\
			->qq_type = %u",
			msg->qq_num,
			msg->qq_type);
}

/** 
 *@brief  打印富媒体报文信息
 *@param  msg		类型 simple_gg_msg_st*	富媒体报文
 *@return 无
 */
void inline xyprintf_simple_gg_head(simple_gg_msg_st *msg)
{
	char *buf = (char*)msg + sizeof(simple_gg_msg_st);
	xyprintf(0, "simple_gg_st->en = %u\n\
			->js_len = %u\n\
			->dns_cnt = %u\n\
			->dns_cnt = %u\n\
			->data = %s",
			msg->en, msg->js_len, msg->dns_cnt, msg->data_len, buf);
}

/** 
 *@brief  打印操作结果报文信息
 *@param  msg		类型 result_msg_st*	操作结果报文
 *@return 无
 */
void inline xyprintf_result(result_msg_st *msg)
{
	if(msg->len){
		xyprintf(0, "result_msg_st->result = %u\n\
				->len = %u\n\
				->data = %s",
				msg->result, msg->len, msg->data);
	}
	else {
		xyprintf(0, "result_msg_st->result = %u\n\
				->len = %u",
				msg->result, msg->len);
	}
}

/** 
 *@brief  打印自动升级报文信息
 *@param  msg		类型 auto_upgrade_st*	自动升级报文
 *@return 无
 */
void inline xyprintf_auto_upgrade(auto_upgrade_msg_st *msg)
{
	xyprintf(0, "auto_upgrade_st->url_len = %u\n\
			->url_data = %s",
			msg->url_len,
			msg->url_data);
}

/** 
 *@brief  打印自动升级操作结果回复报文信息
 *@param  msg		类型 auto_upgrade_result_st*	自动升级操作结果回复报文
 *@return 无
 */
void inline xyprintf_auto_upgrade_result(auto_upgrade_result_st* msg)
{
	xyprintf(0, "auto_upgrade_result_st->result = %u\n\
			->len = %u\n\
			->data = %s",
			msg->result,
			msg->len,
			msg->data);
}

/** 
 *@brief  打印无线配置报文信息
 *@param  msg		类型 wifi_base_conf_msg_st*	无线配置报文
 *@return 无
 */
void xyprintf_wt_wifi_base_conf_so(wifi_base_conf_msg_st* msg)
{
	xyprintf(0, "wt_wifi_base_conf_so->wifi_enable = %u\n\
			->wifi_mode = %u\n\
			->wifi_channel = %u\n\
			->wifi_ssid_num = %u\n\
			->wifi_isolated = %u\n\
			->wifi_rssi_down = %d\n\
			->wifi_txpower = %u\n\
			->wifi_pad = %u\n\
			->run_mode = %u\n\
			->HT_BW = %u\n\
			->HT_EXTCHA = %u\n\
			->HT_BSSCoexistence = %u",
			msg->wifi_enable, msg->wifi_mode,
			msg->wifi_channel, msg->wifi_ssid_num,
			msg->wifi_isolated, msg->wifi_rssi_down,
			msg->wifi_txpower, msg->wifi_pad,
			msg->run_mode, msg->HT_BW,
			msg->HT_EXTCHA, msg->HT_BSSCoexistence);
	int i;
	for(i = 0; i < 5; i++){
		xyprintf(0, "wt_wifi_base_conf_so->list[%d]\n\
				.ssid = %s\n\
				.hide = %u\n\
				.isolated = %u\n\
				.security_mode = %u\n\
				.ssid_charset = %u\n\
				.balanced = %u\n\
				.pad = %u %u %u",
				i,
				msg->list[i].ssid,
				msg->list[i].hide,
				msg->list[i].isolated,
				msg->list[i].security_mode,
				msg->list[i].ssid_charset,
				msg->list[i].balanced,
				msg->list[i].pad[0],
				msg->list[i].pad[1],
				msg->list[i].pad[2]);
		if(msg->list[i].security_mode == 3){
			xyprintf(0, "wt_wifi_base_conf_so->list[%d].key.wep\n\
					.defaultkey = %u\n\
					.encrypt_type = %u\n\
					.pad = %u %u\n\
					.key1type = %u\n\
					.key1 = %s\n\
					.key2type = %u\n\
					.key2 = %s\n\
					.key3type = %u\n\
					.key3 = %s\n\
					.key4type = %u\n\
					.key4 = %s",
					i,
					msg->list[i].key.wep.defaultkey,
					msg->list[i].key.wep.encrypt_type,
					msg->list[i].key.wep.pad[0],
					msg->list[i].key.wep.pad[1],
					msg->list[i].key.wep.key1type,
					msg->list[i].key.wep.key1,
					msg->list[i].key.wep.key2type,
					msg->list[i].key.wep.key2,
					msg->list[i].key.wep.key3type,
					msg->list[i].key.wep.key3,
					msg->list[i].key.wep.key4type,
					msg->list[i].key.wep.key4);
		}
		else if(msg->list[i].security_mode == 4 || 
				msg->list[i].security_mode == 5 || 
				msg->list[i].security_mode == 6 || 
				msg->list[i].security_mode == 7 || 
				msg->list[i].security_mode == 8 || 
				msg->list[i].security_mode == 9 ){
			xyprintf(0, "wt_wifi_base_conf_so->list[%d].key.wpa\n\
					.key = %s\n\
					.rekeyinterval = %u\n\
					.radius_server = %u\n\
					.session_timeout = %u\n\
					.radius_port = %u\n\
					.PMKCachePeriod = %u\n\
					.PreAuthentication = %u\n\
					.encrypt_type = %u\n\
					.pad = %u",
					i,
					msg->list[i].key.wpa.key,
					msg->list[i].key.wpa.rekeyinterval,
					msg->list[i].key.wpa.radius_server,
					msg->list[i].key.wpa.session_timeout,
					msg->list[i].key.wpa.radius_port,
					msg->list[i].key.wpa.PMKCachePeriod,
					msg->list[i].key.wpa.PreAuthentication,
					msg->list[i].key.wpa.encrypt_type,
					msg->list[i].key.wpa.pad);
		}
	}//end for
}

/** 
 *@brief  打印速度控制报文信息
 *@param  msg		类型 third_user_qos_msg_st*	无线配置报文
 *@return 无
 */
void xyprintf_qos_msg_st(third_user_qos_msg_st* msg)
{
	xyprintf(0, "third_user_qos_msg_st->enable = %u\n\
			.qos_up = %u\n\
			.qos_down = %u\n\
			.exp_ip_num = %u",
			msg->enable,
			msg->qos_up,
			msg->qos_down,
			msg->exp_ip_num);
}

/** 
 *@brief  打印简单缓存报文信息
 *@param  msg		类型 simple_cache_msg_st*	无线配置报文
 *@return 无
 */
void xyprintf_simple_cache_msg_st(simple_cache_msg_st* msg)
{
	xyprintf(0, "simple_cache_msg_st->url_len = %u\n\
			.url_data = %s",
			msg->url_len,
			msg->url_data);
}

/** 
 *@brief  打印下发缓存内容
 *@param  msg		类型 usb_simple_cache_st*	无线配置报文
 *@return 无
 */
void xyprintf_usb_simple_cache_st(usb_simple_cache_st* msg)
{
	if(msg->len){
		xyprintf(0, "usb_simple_cache_st->opt = %u\n\
				.name = %s\n\
				.len = %u\n\
				.url = %s",
				msg->opt,
				msg->name,
				msg->len,
				msg->url);
	}
	else {
		xyprintf(0, "usb_simple_cache_st->opt = %u\n\
				.name = %s\n\
				.len = %u",
				msg->opt,
				msg->name,
				msg->len);
	}
}

/** 
 *@brief  打印行为管理 关键词过滤 报文
 *@param  msg		类型 web_keyword_st*	无线配置报文
 *@return 无
 */
void xyprintf_web_keyword_st(web_keyword_st* msg)
{
	xyprintf(0, "web_keyword_st->opt = %s\n\
			.def = %s\n\
			.log = %s\n\
			.block = %s\n\
			.en = %s\n\
			.name = %s\n\
			.keyword_gb2312 = %s\n\
			.keyword_utf8 = %s",
			msg->opt,
			msg->def,
			msg->log,
			msg->block,
			msg->en,
			msg->name,
			msg->keyword_gb2312,
			msg->keyword_utf8);
/*	int i;
	xyprintf(0, "gb2312");
	for(i = 0; i < 64; i++){
		if(i % 8 == 0 && i){
			printf("\n");
		}
		printf("%2X ", msg->keyword_gb2312[i]);
	}
	printf("\n");
	xyprintf(0, "utf-8");
	for(i = 0; i < 64; i++){
		if(i % 8 == 0 && i){
			printf("\n");
		}
		printf("%2X ", msg->keyword_utf8[i]);
	}
	printf("\n");
*/
}

/** 
 *@brief  阻止用户微信域名重定向功能接口报文打印
 *@param  msg		类型 domain_redirect_st*
 *@return 无
 */
void xyprintf_domain_redirect(domain_redirect_st* msg)
{
	xyprintf(0, "domain_redirect_st->domain_len = %u\n\
			.domain = %s",
			msg->domain_len,
			msg->domain);
}

/** 
 *@brief  打印行为管理 URL重定向 设置报文
 *@param  msg		类型 url_redirect_st*	无线配置报文
 *@return 无
 */
void xyprintf_url_redirect_st(url_redirect_st* msg)
{
	xyprintf(0, "url_redirect_st->en = %s\n\
			.name = %s\n\
			.host = %s\n\
			.host_flag = %s\n\
			.url = %s\n\
			.url_flag = %s\n\
			.parm = %s\n\
			.parm_flag = %s\n\
			.urlrd = %s\n\
			.ld_en = %s\n\
			.ips = %s\n\
			.log = %s\n\
			.time = %s\n\
			.opt = %s",
			msg->en,
			msg->name,
			msg->host,
			msg->host_flag,
			msg->url,
			msg->url_flag,
			msg->parm,
			msg->parm_flag,
			msg->urlrd,
			msg->ld_en,
			msg->ips,
			msg->log,
			msg->time,
			msg->opt);
}

/** 
 *@brief  打印行为管理 流媒体识别 设置报文
 *@param  msg		类型 mx_app_filter_st*	无线配置报文
 *@return 无
 */
void xyprintf_mx_app_filter_st(mx_app_filter_st* msg)
{
	xyprintf(0, "mx_app_filter_st->en = %s\n\
			.name = %s\n\
			.act = %s\n\
			.user_id = %s\n\
			.shibie_name = %s\n\
			.log = %s\n\
			.time = %s\n\
			.opt = %s",
			msg->en,
			msg->name,
			msg->act,
			msg->user_id,
			msg->shibie_name,
			msg->log,
			msg->time,
			msg->opt);
}

/**
 *@bried  打印微信分享设置报文
 *@param  msg		类型 weixin_share_set_st*	设备与管理平台之间的状态包
 *@return 无
 */
void xyprintf_weixin_share_set(weixin_share_set_st* msg)
{
	xyprintf(0, "weixin_share_set_st->enable = %u\n\
			.html_len = %u\n\
			.redirt_url = %s\n\
			.html_data = %s",
			msg->enable,
			msg->html_len,
			msg->redirt_url,
			msg->html_data);
}

/**
 *@bried  打印USB路径文件列表获取报文
 *@param  msg		类型 usb_file_get_st*	设备与管理平台之间的状态包
 *@return 无
 */
void xyprintf_usb_file_get(usb_file_get_st* msg)
{
	xyprintf(0, "usb_file_get_st->dir = %s",
			msg->dir);
}

/**
 *@bried  打印获取实时流量回复报文
 *@param  msg		类型 third_speed_st*	设备返回的流量包
 *@return 无
 */
void xyprintf_speed_st(third_speed_st* msg)
{
	xyprintf(0, "third_speed_st->speed_up = %u\n\
			.speed_down = %u",
			msg->speed_up,
			msg->speed_down);
}

/**
 *@bried  打印获取实时流量回复报文
 *@param  msg		类型 third_speed_st*	设备返回的流量包
 *@return 无
 */
void xyprintf_http_pw_st(httpd_pwd_st* msg)
{
	xyprintf(0, "httpd_pwd_st->user_name = %s\n\
			.passwd = %s",
			msg->user_name,
			msg->passwd);
}

/**
 *@bried  打印简单广告2设置报文
 *@param  msg		类型 simple_gg2_head_st*	设备与管理平台之间的状态包
 *@return 无
 */
void xyprintf_simple_gg2_head_st(simple_gg2_head_st* msg)
{
	xyprintf(0, "simple_gg2_head_st->en = %u\n\
			->cnt = %u\n\
			->data_len = %u",
			msg->en,
			msg->cnt,
			msg->data_len);
	
	int i;
	simple_gg2_st *data = (simple_gg2_st*)(msg->data);
	
	for(i = 0; i < msg->cnt; i++){
		xyprintf(0, "simple_gg2_st->replace_pos_flag = %d\n\
			->replace_pos = %s\n\
			->dns_len= %u\n\
			->js_len = %u\n\
			->date_len = %u",
			data->replace_pos_flag,
			data->replace_pos,
			data->dns_len,
			data->js_len,
			data->data_len);
		
		if( data->data_len > 0 ){
			char* buf = malloc( data->data_len + 1 );
			memset(buf, 0, data->data_len + 1);
			memcpy(buf, data->data , data->data_len);
			xyprintf(0, "->data = %s", buf);
			if(buf){
				free(buf);
			}
		}
		data = (simple_gg2_st*)( ((void*)data) + sizeof(simple_gg2_st) + data->data_len );
	}
}

/**
 *@bried  打印探针设置报文
 *@param  msg		类型 simple_gg2_head_st*	设备与管理平台之间的状态包
 *@return 无
 */
void xyprintf_monitor_set_st(monitor_set_st* msg)
{
	xyprintf(0, "monitor_set_st->state = %u\n\
			->port = %u\n\
			->time = %u\n\
			->timeout = %u\n\
			->address = %s",
			msg->state,
			msg->port,
			msg->time,
			msg->timeout,
			msg->address);
}

