/*****************************************************
 *
 * AC管理服务器 网络操作
 *
 *****************************************************/

#include "header.h"

/** 
 *@brief  接收管理信息包头 并装换为本地字节序
 *@param  sockfd	类型 int					要接收数据的socket套接字
 *@param  msg		类型 ac_head_st*			接收到的数据存放地址
 *@param  len		类型 int					接收数据长度
 *@return success 0 failed -1
 */
int recv_ac_head(int sockfd, ac_head_st *msg)
{
	if( wt_recv_block( sockfd, (char*)msg, sizeof(ac_head_st))){
		return -1;
	}

	big_little32(&msg->device_type);
	big_little32(&msg->deviceID);
	//big_little32(&msg->session);
	big_little32(&msg->datalen);
	return 0;
}

/** 
 *@brief  接收管理登陆信息 并装换为本地字节序
 *@param  sockfd	类型 int						要接收数据的socket套接字
 *@param  msg		类型 ac_login_st*			接收到的数据存放地址
 *@return success 0 failed -1
 */
int recv_ac_login(int sockfd, ac_login_st *msg)
{
	if( wt_recv_block( sockfd, (char*)msg, sizeof(ac_login_st))){
		return -1;
	}

	big_little32(&msg->userID);
	big_little32(&msg->deviceID);
	big_little32(&msg->randID);
	big_little32(&msg->needPWD);
	big_little32(&msg->hearttime);
	big_little32(&msg->baseinfo.type);
	big_little32(&msg->baseinfo.offlinenotauth);
	return 0;
}


/** 
 *@brief  发送管理登陆回复报文到路由器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int				目标路由器的socket套接字
 *@param  msg_head	类型 ac_head_st*	报文头
 *@param  state		类型 ac_login_st*	报文体
 *@return success 0 failed -1
 */
int send_ac_login(int sockfd, ac_head_st *msg_head, ac_login_st *login)
{
	int len = msg_head->datalen;

	big_little32(&msg_head->device_type);
	big_little32(&msg_head->deviceID);
	//big_little32(&msg_head->session);
	big_little32(&msg_head->datalen);

	big_little32(&login->userID);
	big_little32(&login->deviceID);
	big_little32(&login->randID);
	big_little32(&login->needPWD);
	big_little32(&login->hearttime);
	big_little32(&login->baseinfo.type);
	big_little32(&login->baseinfo.offlinenotauth);

	char *msg = malloc(len);
	memcpy(msg, msg_head, sizeof(ac_head_st) );
	memcpy(msg + sizeof(ac_head_st), login, sizeof(ac_login_st));

	int res = wt_send_block( sockfd, msg, len );
	free(msg);
	return res;
}


void inline big_little_stat_ap(ac_state_st *msg)
{
	int i, j;
	for(i = 0; i < msg->count; i++){
		big_little32(&msg->ap[i].device_id);
		big_little32(&msg->ap[i].flag);

		big_little16(&msg->ap[i].base_info.vlan_id);
		//big_little32(&msg->ap[i].base_info.ipaddr);
		//big_little32(&msg->ap[i].base_info.netmask);
		//big_little32(&msg->ap[i].base_info.gateway);
		
		//big_little32(&msg->ap[i].base_info.mem_total);
		//big_little32(&msg->ap[i].base_info.mem_free);
		//big_little32(&msg->ap[i].base_info.ct_max);
		//big_little32(&msg->ap[i].base_info.ct_num);
		
		big_little16(&msg->ap[i].base_info.web_port);

		big_little32(&msg->ap[i].dhcp_info.lease_time);
		//big_little32(&msg->ap[i].dhcp_info.start_ipaddr);
		//big_little32(&msg->ap[i].dhcp_info.end_ipaddr);
		//big_little32(&msg->ap[i].dhcp_info.gw);
		//big_little32(&msg->ap[i].dhcp_info.mask);
		//big_little32(&msg->ap[i].dhcp_info.dns[0]);
		//big_little32(&msg->ap[i].dhcp_info.dns[1]);
		
		for(j = 0; j < 5; j++){
			if(msg->ap[i].wifi_base_info.list[j].security_mode == 4 || 
				msg->ap[i].wifi_base_info.list[j].security_mode == 5 || 
				msg->ap[i].wifi_base_info.list[j].security_mode == 6 || 
				msg->ap[i].wifi_base_info.list[j].security_mode == 7 || 
				msg->ap[i].wifi_base_info.list[j].security_mode == 8 || 
				msg->ap[i].wifi_base_info.list[j].security_mode == 9 ){
			
				big_little32(&(msg->ap[i].wifi_base_info.list[j].key.wpa.rekeyinterval));
				big_little32(&(msg->ap[i].wifi_base_info.list[j].key.wpa.radius_server));
				big_little32(&(msg->ap[i].wifi_base_info.list[j].key.wpa.session_timeout));
				big_little16(&(msg->ap[i].wifi_base_info.list[j].key.wpa.radius_port));
				big_little16(&(msg->ap[i].wifi_base_info.list[j].key.wpa.PMKCachePeriod));
				big_little16(&(msg->ap[i].wifi_base_info.list[j].key.wpa.PreAuthentication));
			}
		}
	
		big_little32(&msg->ap[i].user.user_num);
		big_little32(&msg->ap[i].user.create_time);
	}
}

/** 
 *@brief  接收管理平台的状态信息 并装换为本地字节序
 *@param  sockfd	类型 int						要接收数据的socket套接字
 *@param  msg		类型 ac_state_st*			接收到的数据存放地址
 *@param  len		类型 int						接收数据长度
 *@return success 0 failed -1
 */
int recv_ac_state(int sockfd, ac_state_st *msg, int len)
{
	if( wt_recv_block( sockfd, (char*)msg, len)){
		return -1;
	}
	
	big_little32(&msg->deviceID);
	big_little32(&msg->state);
	big_little32(&msg->count);

	if(msg->count * sizeof(struct ac_ap_conf_st) + sizeof(ac_state_st) != len){
		xyprintf(0, "DATA_ERROR: %s %d -- len is error!", __FILE__, __LINE__);
		return -1;
	}
	
	big_little_stat_ap(msg);

	return 0;
}

/** 
 *@brief  发送管理状态报文到路由器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int				目标路由器的socket套接字
 *@param  msg_head	类型 ac_head_st*	报文头
 *@param  msg		类型 ac_state_st*	报文体
 *@return success 0 failed -1
 */
int send_ac_state(int sockfd, ac_head_st *msg_head, ac_state_st *msg)
{
	int len = msg_head->datalen;

	big_little32(&msg_head->device_type);
	big_little32(&msg_head->deviceID);
	//big_little32(&msg_head->session);
	big_little32(&msg_head->datalen);
	
	big_little_stat_ap(msg);
	
	big_little32(&msg->deviceID);
	big_little32(&msg->state);
	big_little32(&msg->count);

	char *buf = malloc(len);
	memset(buf, 0, len);
	memcpy(buf, msg_head, sizeof(ac_head_st) );
	memcpy(buf + sizeof(ac_head_st), msg, len - sizeof(ac_head_st) );

	int res = wt_send_block( sockfd, buf, len );
	free(buf);
	return res;
}

/** 
 *@brief  接收管理平台的控制请求信息 并转换为本地字节序
 *@param  sockfd	类型 int						要接收数据的socket套接字
 *@param  msg		类型 ac_proxy_st*			接收到的数据存放地址
 *@return success 0 failed -1
 */
int recv_ac_proxy(int sockfd, ac_proxy_st* msg )
{
	if( wt_recv_block( sockfd, (char*)msg, sizeof(ac_proxy_st)) ){
		return -1;
	}

	big_little32(&msg->socket_php);
	big_little32(&msg->ac_id);
	big_little32(&msg->ap_id);
	big_little32(&msg->enable);
	big_little32(&msg->port);
	
	return 0;
}

/** 
 *@brief  发送管理控制请求报文到路由器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int				目标路由器的socket套接字
 *@param  msg_head	类型 ac_head_st*	报文头
 *@param  msg		类型 ac_proxy_st*	报文体
 *@return success 0 failed -1
 */
int send_ac_proxy(int sockfd, ac_head_st *msg_head, ac_proxy_st *msg)
{
	int len = msg_head->datalen;

	big_little32(&msg_head->device_type);
	big_little32(&msg_head->deviceID);
	//big_little32(&msg_head->session);
	big_little32(&msg_head->datalen);
	
	big_little32(&msg->socket_php);
	big_little32(&msg->ac_id);
	big_little32(&msg->ap_id);
	big_little32(&msg->enable);
	big_little32(&msg->port);

	char *buf = malloc(len);
	memcpy(buf, msg_head, sizeof(ac_head_st) );
	memcpy(buf + sizeof(ac_head_st), msg, sizeof(ac_proxy_st) );

	int res = wt_send_block( sockfd, buf, len );
	free(buf);
	return res;
}



/**************************** AC管理平台 *******************************/


/** 
 *@brief  打印设备与管理平台通信的报文头
 *@param  msg		类型 ac_head_st*	设备到管理平台的报文头
 *@return 无
 */
void xyprintf_ac_head(ac_head_st* msg)
{
	xyprintf(0, "ac_head_st->ver = %u\n\
			.cmdID = %u\n\
			.mode = %u\n\
			.device_type = %u\n\
			.deviceID = %u\n\
			.session = %u\n\
			.datalen = %u",
			//\n\.tags = %s",
			msg->ver,
			msg->cmdID,
			msg->mode,
			msg->device_type,
			msg->deviceID,
			msg->session,
			msg->datalen/*,
			msg->tags*/);
}

/** 
 *@brief  打印设备与管理平台通信的报文头
 *@param  msg		类型 ac_login_st*	设备到管理平台的报文头
 *@return 无
 */
void xyprintf_ac_login(ac_login_st* msg)
{
	xyprintf(0, "ac_login_st->userID = %u\n\
			.deviceID = %u\n\
			.randID = %u\n\
			.needPWD = %u\n\
			.devicePWD = %s\n\
			.deviceName = %s\n\
			.sn = %s\n\
			.hearttime = %u\n\
			.state = %u\n\
			.msg = %s\n\
			.baseinfo.addr = %s\n\
			.baseinfo.cont = %s\n\
			.baseinfo.tel = %s\n\
			.baseinfo.dinate = %s\n\
			.baseinfo.type = %u\n\
			.baseinfo.offlinenotauth = %u",
			msg->userID,
			msg->deviceID,
			msg->randID,
			msg->needPWD,
			msg->devicePWD,
			msg->deviceName,
			msg->sn,
			msg->hearttime,
			msg->state,
			msg->msg,
			msg->baseinfo.addr,
			msg->baseinfo.cont,
			msg->baseinfo.tel,
			msg->baseinfo.dinate,
			msg->baseinfo.type,
			msg->baseinfo.offlinenotauth);
}

/**
 *@bried  打印设备与管理平台之间的状态包
 *@param  msg		类型 ac_state_st*	设备与管理平台之间的状态包
 *@return 无
 */
void xyprintf_ac_state(ac_state_st* msg)
{
	xyprintf(0, "ac_state_st->deviceID = %u\n\
			.state = %u\n\
			.msg = %s\n\
			.count = %u",
			msg->deviceID,
			msg->state,
			msg->msg,
			msg->count);
	int i, j;
	char temp[6][32] = { 0 };
	for(i = 0; i < msg->count; i++){
		xyprintf(0, "ac_ap_conf_st[%d]->device_id = %u\n\
				.flag = %d",
				i,
				msg->ap[i].device_id,
				msg->ap[i].flag);
		
		ip_to_str(temp[0], msg->ap[i].base_info.ipaddr);
		ip_to_str(temp[1], msg->ap[i].base_info.netmask);
		ip_to_str(temp[2], msg->ap[i].base_info.gateway);
		
		xyprintf(0, "[%d].ac_ap_base_conf_st->ap_name = %s\n\
				.ap_info = %s\n\
				.vlan_name = %s\n\
				.vlan_id = %u\n\
				.ap_bssid = %02x%02x%02x%02x%02x%02x\n\
				.max_num = %u\n\
				.ap_power = %u\n\
				.ap_mode = %u\n\
				.ipaddr = %s\n\
				.netmask = %s\n\
				.gateway = %s\n\
				.ap_version = %s\n\
				.atime = %s\n\
				.ftime = %s\n\
				.mem_total = %u\n\
				.mem_free = %u\n\
				.ct_max = %u\n\
				.ct_num = %u\n\
				.web_port = %u\n\
				.http_username = %s\n\
				.http_passwd = %s",
				i,
				msg->ap[i].base_info.ap_name,
				msg->ap[i].base_info.ap_info,
				msg->ap[i].base_info.vlan_name,
				msg->ap[i].base_info.vlan_id,
				msg->ap[i].base_info.ap_bssid[0],
				msg->ap[i].base_info.ap_bssid[1],
				msg->ap[i].base_info.ap_bssid[2],
				msg->ap[i].base_info.ap_bssid[3],
				msg->ap[i].base_info.ap_bssid[4],
				msg->ap[i].base_info.ap_bssid[5],
				msg->ap[i].base_info.max_num,
				msg->ap[i].base_info.ap_power,
				msg->ap[i].base_info.ap_mode,
				temp[0],
				temp[1],
				temp[2],
				msg->ap[i].base_info.ap_version,
				msg->ap[i].base_info.atime,
				msg->ap[i].base_info.ftime,
				msg->ap[i].base_info.mem_total,
				msg->ap[i].base_info.mem_free,
				msg->ap[i].base_info.ct_max,
				msg->ap[i].base_info.ct_num,
				msg->ap[i].base_info.web_port,
				msg->ap[i].base_info.http_username,
				msg->ap[i].base_info.http_passwd);

		ip_to_str(temp[0], msg->ap[i].dhcp_info.start_ipaddr);
		ip_to_str(temp[1], msg->ap[i].dhcp_info.end_ipaddr);
		ip_to_str(temp[2], msg->ap[i].dhcp_info.gw);
		ip_to_str(temp[3], msg->ap[i].dhcp_info.mask);
		ip_to_str(temp[4], msg->ap[i].dhcp_info.dns[0]);
		ip_to_str(temp[5], msg->ap[i].dhcp_info.dns[1]);
		
		xyprintf(0, "[%d].ac_ap_dhcp_conf_st->enable = %u\n\
				.lease_time = %u\n\
				.start_ipaddr = %s\n\
				.end_ipaddr = %s\n\
				.gw = %s\n\
				.mask = %s\n\
				.dns[0] = %s\n\
				.dns[1] = %s",
				i,
				msg->ap[i].dhcp_info.enable,
				msg->ap[i].dhcp_info.lease_time,
				temp[0], temp[1], temp[2],
				temp[3], temp[4], temp[5] );

		xyprintf(0, "[%d].ac_ap_wifi_base_conf_st->enable = %u\n\
				.mode = %u\n\
				.channel = %u\n\
				.ssid_num = %u\n\
				.isolated = %u\n\
				.rssi_down = %d\n\
				.user_auth = %u",
				i,
				msg->ap[i].wifi_base_info.enable,
				msg->ap[i].wifi_base_info.mode,
				msg->ap[i].wifi_base_info.channel,
				msg->ap[i].wifi_base_info.ssid_num,
				msg->ap[i].wifi_base_info.isolated,
				msg->ap[i].wifi_base_info.rssi_down,
				msg->ap[i].wifi_base_info.user_auth);
		for(j = 0; j < 5; j++){
			xyprintf(0, "[%d].ac_ap_wifi_base_conf_st->list[%d]->ssid = %s\n\
					.hide = %u\n\
					.isolated = %u\n\
					.security_mode = %u\n\
					.ssid_charset = %u\n\
					.balanced = %u",
					i, j,
					msg->ap[i].wifi_base_info.list[j].ssid,
					msg->ap[i].wifi_base_info.list[j].hide,
					msg->ap[i].wifi_base_info.list[j].isolated,
					msg->ap[i].wifi_base_info.list[j].security_mode,
					msg->ap[i].wifi_base_info.list[j].ssid_charset,
					msg->ap[i].wifi_base_info.list[j].balanced);
			if(msg->ap[i].wifi_base_info.list[j].security_mode == 3){
				xyprintf(0, "[%d].ac_ap_wifi_base_conf_st->list[%d].key.wep\n\
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
						i, j,
						msg->ap[i].wifi_base_info.list[j].key.wep.defaultkey,
						msg->ap[i].wifi_base_info.list[j].key.wep.encrypt_type,
						msg->ap[i].wifi_base_info.list[j].key.wep.pad[0],
						msg->ap[i].wifi_base_info.list[j].key.wep.pad[1],
						msg->ap[i].wifi_base_info.list[j].key.wep.key1type,
						msg->ap[i].wifi_base_info.list[j].key.wep.key1,
						msg->ap[i].wifi_base_info.list[j].key.wep.key2type,
						msg->ap[i].wifi_base_info.list[j].key.wep.key2,
						msg->ap[i].wifi_base_info.list[j].key.wep.key3type,
						msg->ap[i].wifi_base_info.list[j].key.wep.key3,
						msg->ap[i].wifi_base_info.list[j].key.wep.key4type,
						msg->ap[i].wifi_base_info.list[j].key.wep.key4);
			}
			else if(msg->ap[i].wifi_base_info.list[j].security_mode == 4 || 
					msg->ap[i].wifi_base_info.list[j].security_mode == 5 || 
					msg->ap[i].wifi_base_info.list[j].security_mode == 6 || 
					msg->ap[i].wifi_base_info.list[j].security_mode == 7 || 
					msg->ap[i].wifi_base_info.list[j].security_mode == 8 || 
					msg->ap[i].wifi_base_info.list[j].security_mode == 9 ){
				xyprintf(0, "[%d].ac_ap_wifi_base_conf_st->list[%d].key.wpa\n\
						.key = %s\n\
						.rekeyinterval = %u\n\
						.radius_server = %u\n\
						.session_timeout = %u\n\
						.radius_port = %u\n\
						.PMKCachePeriod = %u\n\
						.PreAuthentication = %u\n\
						.encrypt_type = %u\n\
						.pad = %u",
						i, j,
						msg->ap[i].wifi_base_info.list[j].key.wpa.key,
						msg->ap[i].wifi_base_info.list[j].key.wpa.rekeyinterval,
						msg->ap[i].wifi_base_info.list[j].key.wpa.radius_server,
						msg->ap[i].wifi_base_info.list[j].key.wpa.session_timeout,
						msg->ap[i].wifi_base_info.list[j].key.wpa.radius_port,
						msg->ap[i].wifi_base_info.list[j].key.wpa.PMKCachePeriod,
						msg->ap[i].wifi_base_info.list[j].key.wpa.PreAuthentication,
						msg->ap[i].wifi_base_info.list[j].key.wpa.encrypt_type,
						msg->ap[i].wifi_base_info.list[j].key.wpa.pad);
			}
		}
		
		xyprintf(0, "[%d].ac_ap_wifi_wds_conf_st->wds_mode = %u\n\
				.phy_mode = %u\n\
				.list[0].mac = %02x%02x%02x%02x%02x%02x\n\
				.list[0].encrypt_type = %u\n\
				.list[0].key = %s\n\
				.list[1].mac = %02x%02x%02x%02x%02x%02x\n\
				.list[1].encrypt_type = %u\n\
				.list[1].key = %s\n\
				.list[2].mac = %02x%02x%02x%02x%02x%02x\n\
				.list[2].encrypt_type = %u\n\
				.list[2].key = %s\n\
				.list[3].mac = %02x%02x%02x%02x%02x%02x\n\
				.list[3].encrypt_type = %u\n\
				.list[3].key = %s",
				i,
				msg->ap[i].wifi_wds_info.wds_mode,
				msg->ap[i].wifi_wds_info.phy_mode,
				msg->ap[i].wifi_wds_info.list[0].mac[0],
				msg->ap[i].wifi_wds_info.list[0].mac[1],
				msg->ap[i].wifi_wds_info.list[0].mac[2],
				msg->ap[i].wifi_wds_info.list[0].mac[3],
				msg->ap[i].wifi_wds_info.list[0].mac[4],
				msg->ap[i].wifi_wds_info.list[0].mac[5],
				msg->ap[i].wifi_wds_info.list[0].encrypt_type,
				msg->ap[i].wifi_wds_info.list[0].key,
				msg->ap[i].wifi_wds_info.list[1].mac[0],
				msg->ap[i].wifi_wds_info.list[1].mac[1],
				msg->ap[i].wifi_wds_info.list[1].mac[2],
				msg->ap[i].wifi_wds_info.list[1].mac[3],
				msg->ap[i].wifi_wds_info.list[1].mac[4],
				msg->ap[i].wifi_wds_info.list[1].mac[5],
				msg->ap[i].wifi_wds_info.list[1].encrypt_type,
				msg->ap[i].wifi_wds_info.list[1].key,
				msg->ap[i].wifi_wds_info.list[2].mac[0],
				msg->ap[i].wifi_wds_info.list[2].mac[1],
				msg->ap[i].wifi_wds_info.list[2].mac[2],
				msg->ap[i].wifi_wds_info.list[2].mac[3],
				msg->ap[i].wifi_wds_info.list[2].mac[4],
				msg->ap[i].wifi_wds_info.list[2].mac[5],
				msg->ap[i].wifi_wds_info.list[2].encrypt_type,
				msg->ap[i].wifi_wds_info.list[2].key,
				msg->ap[i].wifi_wds_info.list[3].mac[0],
				msg->ap[i].wifi_wds_info.list[3].mac[1],
				msg->ap[i].wifi_wds_info.list[3].mac[2],
				msg->ap[i].wifi_wds_info.list[3].mac[3],
				msg->ap[i].wifi_wds_info.list[3].mac[4],
				msg->ap[i].wifi_wds_info.list[3].mac[5],
				msg->ap[i].wifi_wds_info.list[3].encrypt_type,
				msg->ap[i].wifi_wds_info.list[3].key);
		xyprintf(0, "[%d].ac_ap_user_st->user_num = %u\n\
				.user_ssid_num[0] = %u\n\
				.user_ssid_num[1] = %u\n\
				.user_ssid_num[2] = %u\n\
				.user_ssid_num[3] = %u\n\
				.user_ssid_num[4] = %u\n\
				.user_ssid_num[5] = %u\n\
				.user_ssid_num[6] = %u\n\
				.user_ssid_num[7] = %u\n\
				.create_time = %u",
				i,
				msg->ap[i].user.user_num,
				msg->ap[i].user.user_ssid_num[0],
				msg->ap[i].user.user_ssid_num[1],
				msg->ap[i].user.user_ssid_num[2],
				msg->ap[i].user.user_ssid_num[3],
				msg->ap[i].user.user_ssid_num[4],
				msg->ap[i].user.user_ssid_num[5],
				msg->ap[i].user.user_ssid_num[6],
				msg->ap[i].user.user_ssid_num[7],
				msg->ap[i].user.create_time);
	}
}

/**
 *@bried  打印设备与管理平台之间的控制请求报文
 *@param  msg		类型 ac_proxy_st*	设备与管理平台之间的状态包
 *@return 无
 */
void xyprintf_ac_proxy(ac_proxy_st* msg)
{
	unsigned char gb2312[64] = { 0 };
	unsigned char utf8[128] = { 0 };
	memcpy(gb2312, msg->msg, 64);
	
	if( code_convert("GB2312", "UTF-8", gb2312, strlen(gb2312), utf8, 128) ){
		xyprintf(0, "DATA_ERROR:%s %d -- code_convert() has error!", __FILE__, __LINE__);
	}
	
	xyprintf(0, "ac_proxy_st->socket_php = %u\n\
			.ac_id = %u\n\
			.ap_id = %u\n\
			.enable = %u\n\
			.port = %u\n\
			.ctrlURL = %s\n\
			.msg = %s",
			msg->socket_php,
			msg->ac_id,
			msg->ap_id,
			msg->enable,
			msg->port,
			msg->ctrlURL,
			utf8);
}
