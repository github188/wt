/*****************************************************
 *
 * BOSS服务器 与 其他服务器的交互
 *
 *****************************************************/
#include "boss_header.h"

#define GUIDE_DEBUG			1

/** 
 *@brief  设备操作回复报文处理
 *@return succ 0,failed -1
 */
int router_process(bg_router_msg *msg, wt_sql_handle* handle)
{
	if(msg->order == BG_ROUTER_ADD){
		sprintf(handle->sql_str, "DELETE FROM mx_s_shebei_temp WHERE hard_seq = '%s'", msg->sn);
		if (wt_sql_exec(handle)){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			return WT_SQL_ERROR;
		}
	}
	else if(msg->order == BG_ROUTER_DEL){
		sprintf(handle->sql_str, "DELETE FROM mx_s_shebei_jiebangtemp WHERE hard_seq = '%s'", msg->sn);
		if (wt_sql_exec(handle)){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			return WT_SQL_ERROR;
		}
	}
	else {
		xyprintf(0, "GUIDE_ERROR:%d %s %d:Order is error!!!", __FILE__, __LINE__);
	}

	return 0;
}

/** 
 *@brief  数量修改函数
 *@return succ 0,failed -1
 */
int num_change(int userid)
{
	if(userid <= 0){
		xyprintf(0, "PLATFORM:Userid error -- %d", userid);
		return -1;
	}

	int sockfd = guide_id_call_sockfd(userid);
	if(sockfd == -1){
		xyprintf(0, "PLATFORM:%s %s -- Can't find this userid -- %d", __FILE__, __LINE__, userid);
		return -1;
	}
	if( send_get_msg(bg_get_sn, sockfd) ){
		guide_list_err(sockfd);
		return -1;	
	}
	return 0;
}

/** 
 *@brief  发送信息获取修改指令
 *@return succ 0,failed -1
 */
int send_router_msg(int userid, int order, char* type, char* sn)
{
	int sockfd = guide_id_call_sockfd(userid);
	if(sockfd == -1){
		xyprintf(0, "PLATFORM:%s %d -- Can't find this userid -- %d", __FILE__, __LINE__, userid);
		return -1;
	}
	
	char* buf = malloc(sizeof(bg_msg_head) + sizeof(bg_router_msg));
	memset(buf, 0, sizeof(bg_msg_head) + sizeof(bg_router_msg));
	bg_msg_head* head = (bg_msg_head*)buf;
	head->ver	= BG_HEAD_VER;
	head->head	= BG_HEAD_HEAD;
	head->order	= bg_router;
	head->len	= sizeof(bg_msg_head) + sizeof(bg_router_msg);

	bg_router_msg* msg = (bg_router_msg*)(buf + sizeof(bg_msg_head));
	msg->order = order;
	snprintf(msg->type, sizeof(msg->type), "%s", type);
	snprintf(msg->sn, sizeof(msg->sn), "%s", sn);

	
	if( wt_send_block(sockfd, buf, head->len ) ){
		xyprintf(0, "GUIDE_ERROR:%d %s %d:Send massage to guide is error, order is %d!", sockfd, __FILE__, __LINE__, order);
		if(buf){
			free(buf);
		}
		return -1;
	}

	xyprintf(0, "%d: --> --> send router msg (%s) --> -->", sockfd, msg->sn);
	if(buf){
		free(buf);
	}
	return 0;
}

/** 
 *@brief  发送信息获取修改指令
 *@return succ 0,failed -1
 */
int send_get_msg(int order, int sockfd)
{
	if( order == bg_get_shebei_type || order == bg_get_shanghu || order == bg_get_sn){
		
		bg_msg_head head;
		head.ver	= BG_HEAD_VER;
		head.head	= BG_HEAD_HEAD;
		head.order	= order;
		head.len	= sizeof(bg_msg_head);
	
		if( wt_send_block(sockfd, &head, head.len ) ){
			xyprintf(0, "GUIDE_ERROR:%d %s %d:Send massage to guide is error, order is %d!", sockfd, __FILE__, __LINE__, order);
			return -1;
		}

		if(order == bg_get_shebei_type){
			xyprintf(0, "GUIDE: --> --> send get shebei type msg --> -->");
		}
		else if (order == bg_get_shanghu){
			xyprintf(0, "GUIDE: --> --> send get shanghu msg --> -->");
		}
		else if (order == bg_get_sn){
			xyprintf(0, "GUIDE: --> --> send get sn msg --> -->");
		}

		return 0;
	}
	
	xyprintf(0, "GUIDE_ERROR:%s %d -- Order is error!", __FILE__, __LINE__);

	return -1;
}

/** 
 *@brief  发送设备类型修改指令
 *@return succ 0,failed -1,sql error WT_SQL_ERROR
 */
int send_rt_num_msg(char* version, int isok_flag, int new_user_flag, int php_sockfd, unsigned int userid, int sockfd, unsigned char num_flag, unsigned int total_num, wt_sql_handle* handle)
{
	// 如果不可用 直接发送不可用的数据
	if(isok_flag){
		char *buf = malloc(sizeof(bg_msg_head) + sizeof(bg_num_msg));
		memset(buf, 0, sizeof(bg_msg_head) + sizeof(bg_num_msg) );

		bg_msg_head *head	= (bg_msg_head*)buf;
		head->ver			= BG_HEAD_VER;
		head->head			= BG_HEAD_HEAD;
		head->order			= bg_canuse_num;
		head->len			= sizeof(bg_msg_head) + sizeof(bg_num_msg);
		
		bg_num_msg *msg = (bg_num_msg*)(buf + sizeof(bg_msg_head));
		msg->php_sockfd		= php_sockfd;
		msg->guide_flag		= -1;
		msg->heart_interval = sgv_tupdate;
		msg->num_flag		= 0;
		msg->total_num		= 0;
		msg->rt_num			= 0;

		//xyprintf_bg_num_msg(msg);
		
		if( SEND_BOSSS_MSG_OF_MD5(version, sockfd, head, head->len) ){
			xyprintf(0, "SEND_ERROR:%d %s %d : Send num massage error!", sockfd, __FILE__, __LINE__);
			guide_list_err(sockfd);
			
			if(buf){
				free(buf);
			}

			return -1;
		}
		
		xyprintf(0, "%d --> --> Can use num (Can't use) --> -->", sockfd);
			
		if(buf){
			free(buf);
		}

		return 0;
	}

	// 查询类型数据
	unsigned int	rnum;
	char			name[32];
	SQLBindCol( handle->sqlstr_handle, 1, SQL_C_ULONG,	&rnum,			20,  &handle->sql_err);//将变量和查询结果绑定
	SQLBindCol( handle->sqlstr_handle, 2, SQL_C_CHAR,	name,			32,  &handle->sql_err);//将变量和查询结果绑定
	
	sprintf(handle->sql_str, "SELECT rnum, typecode FROM mx_s_user_shebei_type WHERE userid = %u", userid);
	if (wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return WT_SQL_ERROR;
	}

	char *buf = malloc(sizeof(bg_msg_head) + sizeof(bg_num_msg));
	bg_num_msg *msg = (bg_num_msg*)(buf + sizeof(bg_msg_head));
	int count = 0;

	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	while(handle->sql_ret != SQL_NO_DATA){
		count++;
		buf = realloc(buf, sizeof(bg_msg_head) + sizeof(bg_num_msg) + count * sizeof(struct bg_msg_rt_num));
		msg = (bg_num_msg*)(buf + sizeof(bg_msg_head));
		
		msg->rts[count - 1].num = rnum;
		memcpy(msg->rts[count - 1].name, name, sizeof(name) );
		
		handle->sql_ret=SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
	}

	// 填充报文头
	bg_msg_head *head	= (bg_msg_head*)buf;
	head->ver			= BG_HEAD_VER;
	head->head			= BG_HEAD_HEAD;
	head->order			= bg_canuse_num;
	head->len			= sizeof(bg_msg_head) + sizeof(bg_num_msg) + count * sizeof(struct bg_msg_rt_num);

	// 填充报文体
	msg->php_sockfd		= php_sockfd;
	msg->guide_flag		= new_user_flag;
	msg->heart_interval = sgv_tupdate;
	msg->num_flag		= num_flag;
	msg->total_num		= total_num;
	msg->rt_num			= count;

	//xyprintf_bg_num_msg(msg);

	// 发送
	if( SEND_BOSSS_MSG_OF_MD5(version, sockfd, head, head->len) ){
		xyprintf(0, "SEND_ERROR:%d %s %d : Send num massage error!", sockfd, __FILE__, __LINE__);
		guide_list_err(sockfd);
		
		if(buf){
			free(buf);
		}
		
		return -1;
	}

	xyprintf(0, "%d --> --> Can use num (Can use) --> -->", sockfd);
	
	if(buf){
		free(buf);
	}

	return 0;
}

/** 
 *@brief  日报表数据处理函数
 *@return nothing
 */
int daily_report_process(int sockfd, bg_daily_msg *msg, wt_sql_handle* handle)
{
	unsigned int userid = guide_sockfd_call_id(sockfd);
	if(userid == 0){
		return -1;
	}

	unsigned int rnum;
	SQLBindCol( handle->sqlstr_handle, 1, SQL_C_ULONG,	&rnum,				64,  &handle->sql_err);
	sprintf(handle->sql_str, "SELECT canusenum FROM mx_s_user WHERE id = %u", userid);
	if (wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return WT_SQL_ERROR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	int iswarning = 0;
	if(msg->router_used_num >= rnum){
		iswarning = 1;
	}

	sprintf(handle->sql_str, "INSERT INTO mx_s_user_shebei_log(logdate, userid, rnum, rusenum,\
			ronlinenum, apnum, aponlinenum, iswarning, shanghunum ) \
			VALUES('%s', %u, %u, %u, %u, %u, %u, %u, %u)",
			msg->yesterday, userid, rnum, msg->router_used_num,
			msg->router_yo_num, msg->ap_total_num, msg->ap_yo_num, iswarning, msg->shanghu_num);
	if (wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return WT_SQL_ERROR;
	}
	
	return 0;
}

/** 
 *@brief  商户数据处理函数
 *@return nothing
 */
int shanghu_process(int sockfd, bg_shanghu_msg *msg, wt_sql_handle* handle)
{
	unsigned int userid = guide_sockfd_call_id(sockfd);
	if(userid == 0){
		return -1;
	}

	unsigned int id;
	SQLBindCol( handle->sqlstr_handle, 1, SQL_C_ULONG,	&id,				64,  &handle->sql_err);

	int i = 0;
	for(; i < msg->shanghu_num; i++){
		sprintf(handle->sql_str, "SELECT id FROM mx_s_shanghu WHERE userid = %u AND usershanghuid = %u",
				userid, msg->shanghus[i].shanghu_id);
		if (wt_sql_exec(handle)){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			return WT_SQL_ERROR;
		}
		
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);
		SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

		if(handle->sql_ret != SQL_NO_DATA){
			sprintf(handle->sql_str, "UPDATE mx_s_shanghu SET srealname = '%s', isreopen = %u,\
					retype = '%s', tel = '%s', mobile = '%s', address = '%s', \
					fuzeren = '%s', devicenum = %u, usenum = %u, unusenum = %u,\
					apnum = %u, macnum = %u, isok = %u, zuobiao = '%s' \
					WHERE id = %u",
					msg->shanghus[i].name, msg->shanghus[i].isreopen,
					msg->shanghus[i].retype, msg->shanghus[i].tel, msg->shanghus[i].mobile, msg->shanghus[i].address,
					msg->shanghus[i].fuzeren, msg->shanghus[i].router_num, msg->shanghus[i].use_num, msg->shanghus[i].unuse_num,
					msg->shanghus[i].ap_num, msg->shanghus[i].mac_num, msg->shanghus[i].isok, msg->shanghus[i].zuobiao,
					id);
			if (wt_sql_exec(handle)){
				xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
				return WT_SQL_ERROR;
			}
		}
		else {
			sprintf(handle->sql_str, "INSERT INTO mx_s_shanghu(userid, usershanghuid, srealname, isreopen, \
					retype, tel, mobile, address, fuzeren, devicenum, usenum, unusenum, apnum, macnum, isok, zuobiao ) \
					VALUES(%u, %u, '%s', %u, '%s', '%s', '%s', '%s', '%s', %u, %u, %u, %u, %u, %u, '%s')",
					userid, msg->shanghus[i].shanghu_id, msg->shanghus[i].name, msg->shanghus[i].isreopen,
					msg->shanghus[i].retype, msg->shanghus[i].tel, msg->shanghus[i].mobile, msg->shanghus[i].address,
					msg->shanghus[i].fuzeren, msg->shanghus[i].router_num, msg->shanghus[i].use_num, msg->shanghus[i].unuse_num,
					msg->shanghus[i].ap_num, msg->shanghus[i].mac_num, msg->shanghus[i].isok, msg->shanghus[i].zuobiao);
			if (wt_sql_exec(handle)){
				xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
				return WT_SQL_ERROR;
			}
		}
	}
	
	if(msg->shanghu_num != SHANGHU_ONCE_NUM){
		pthread_mutex_lock(&gv_guide_list_lock);//⊙﹏⊙
		gv_time_ed_flag = 1;
		pthread_mutex_unlock(&gv_guide_list_lock);//⊙﹏⊙
	}

	return 0;
}

/** 
 *@brief  设备类型数据处理函数
 *@return nothing
 */
int rts_process(int sockfd, bg_rts_msg *msg, wt_sql_handle* handle)
{
	unsigned int userid = guide_sockfd_call_id(sockfd);
	if(userid == 0){
		return -1;
	}

	unsigned int id;
	SQLBindCol( handle->sqlstr_handle, 1, SQL_C_ULONG,	&id,				64,  &handle->sql_err);

	int i = 0;
	for(; i < msg->rt_num; i++){
		sprintf(handle->sql_str, "SELECT id FROM mx_s_user_shebei_type WHERE userid = %u AND usershebeitypeid = %u",
				userid, msg->rts[i].id);
		if (wt_sql_exec(handle)){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			return WT_SQL_ERROR;
		}
		
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);
		SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
		
		if(handle->sql_ret != SQL_NO_DATA){
			sprintf(handle->sql_str, "UPDATE mx_s_user_shebei_type SET typecode = '%s', drivecode = '%s',\
					driveurl = '%s', rarurl = '%s', ishavewifi = %u, ishaveacserver = %u, \
					nowrnum = %u, modiid = 6612, modidate = GETDATE() WHERE id = %u",
					msg->rts[i].name, msg->rts[i].driver_code,
					msg->rts[i].upurl, msg->rts[i].rarurl, msg->rts[i].ishavewifi, msg->rts[i].ishaveacserver,
					msg->rts[i].curr_num, id);
			if (wt_sql_exec(handle)){
				xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
				return WT_SQL_ERROR;
			}
		}
		else {
			sprintf(handle->sql_str, "INSERT INTO mx_s_user_shebei_type(userid, usershebeitypeid, typecode, drivecode, \
					driveurl, rarurl, ishavewifi, ishaveacserver, \
					rnum, nowrnum, createdate ) \
					VALUES( %u, %u, '%s', '%s', \
					'%s', '%s', %u, %u, \
					0, '%u', GETDATE())",
					userid, msg->rts[i].id, msg->rts[i].name, msg->rts[i].driver_code,
					msg->rts[i].upurl, msg->rts[i].rarurl, msg->rts[i].ishavewifi, msg->rts[i].ishaveacserver,
					msg->rts[i].curr_num);
			if (wt_sql_exec(handle)){
				xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
				return WT_SQL_ERROR;
			}
		}
	}
	
	pthread_mutex_lock(&gv_guide_list_lock);//⊙﹏⊙
	gv_time_ed_flag = 1;
	pthread_mutex_unlock(&gv_guide_list_lock);//⊙﹏⊙
	
	return 0;
}

/** 
 *@brief  心跳数据处理函数
 *@return nothing
 */
int heart_process(int sockfd, bg_heart_msg *msg, wt_sql_handle* handle)
{
	unsigned int userid = guide_sockfd_call_id(sockfd);
	if(userid == -1){
		return -1;
	}

	if(userid == 0){
		return 0;
	}

	sprintf(handle->sql_str, "UPDATE mx_s_user SET devicenum = %u, usenum = %u, isonlinenum = %u, \
			apnum = %u, isonlineap = %u, isonline = 1, lastdate = GETDATE() WHERE id = %u",
			msg->router_total_num, msg->router_bind_num, msg->router_online_num,
			msg->ap_total_num, msg->ap_online_num, userid);
	if (wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return WT_SQL_ERROR;
	}

	return 0;
}

/** 
 *@brief  基础信息报文处理
 *@return succ 0,failed -1,sql error WT_SQL_ERROR
 */
int basis_sn_process(char* version, unsigned int *userid, bg_conn_msg* msg, guide_node *node, wt_sql_handle* handle)
{
	int error_flag = 0; //是否出错标志
	int new_user_flag = 0;

	// 判断sn号是否正确或存在
	unsigned int id = 0, isuse = 0, isok = 0, isjingjian = 0;
	SQLBindCol( handle->sqlstr_handle, 1, SQL_C_ULONG, &id,			20, &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 2, SQL_C_ULONG, &isuse,		20, &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 3, SQL_C_ULONG, &isok,		20, &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 4, SQL_C_ULONG, &isjingjian,	20, &handle->sql_err);
	sprintf(handle->sql_str, "SELECT id, isuse, isok, isjingjian FROM mx_s_sncode WHERE sncode = '%s'", msg->sn);
	if(wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
		return WT_SQL_ERROR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);

	// 没有sn号
	if(handle->sql_ret == SQL_NO_DATA){
		xyprintf(0, "%d -- Not has this sncode in mx_s_sncode!", node->sockfd);
		error_flag = -1;
	} else {
		// sn号不可用
		if( !isok ){
			xyprintf(0, "GUIDE -- %s %d -- This sncode's ok stat is error, sockfd is %d!", __FILE__, __LINE__, node->sockfd);
			id = 0;
			error_flag = -1;
		}
	
		// 版本判断
		if( !isjingjian ){
			if( !msg->is_streamline ){
				xyprintf(0, "GUIDE -- %s %d -- The sn is streamline, but user's program is not, sockfd is %d!", __FILE__, __LINE__, node->sockfd);
				id = 0;
				error_flag = -1;
			}
		}
	}

	unsigned int usetype = 0, canusenum = 0;

	// 在有sn号和sn号可用的情况下
	if( !error_flag ){
		
		// 判断sn号是否使用
		if(!isuse){
			sprintf(handle->sql_str, "UPDATE mx_s_sncode SET isuse = 1 WHERE id = %u", id);
			if(wt_sql_exec(handle)){
				xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
				return WT_SQL_ERROR;
			}
			xyprintf(0, "%d -- Now sncode of %s is used!", node->sockfd, msg->sn);
		}

		// mx_s_user 表查询 判断
		id = 0;
		unsigned int isonline;
		char ydservip[32];
		SQLBindCol( handle->sqlstr_handle, 1, SQL_C_ULONG, &id,			20, &handle->sql_err);
		SQLBindCol( handle->sqlstr_handle, 2, SQL_C_ULONG, &usetype,	20, &handle->sql_err);
		SQLBindCol( handle->sqlstr_handle, 3, SQL_C_ULONG, &canusenum,	20, &handle->sql_err);
		SQLBindCol( handle->sqlstr_handle, 4, SQL_C_ULONG, &isonline,	20, &handle->sql_err);
		SQLBindCol( handle->sqlstr_handle, 5, SQL_C_ULONG, &isok,		20, &handle->sql_err);
		SQLBindCol( handle->sqlstr_handle, 6, SQL_C_CHAR,  ydservip,	20, &handle->sql_err);
	
		sprintf(handle->sql_str, "SELECT id, usetype, canusenum, isonline, isok, ydservip FROM mx_s_user WHERE sncode = '%s'", msg->sn);
		if(wt_sql_exec(handle)){
			xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
			return WT_SQL_ERROR;
		}
	
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);
		SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);

		if(handle->sql_ret == SQL_NO_DATA){
			// 如果没有数据 那就插入数据
			sprintf(handle->sql_str, "INSERT INTO mx_s_user(sncode, drealname, companyname, companyaddress, \
				fzrname, fzrtel, qq, usetype, canusenum, isok, \
				isonline, ydservip, lastdate, createdate, isjingjian) \
				VALUES('%s', '%s', '%s', '%s', \
				'%s', '%s', '%s', 1, %u, 1, \
				1, '%s', GETDATE(), GETDATE() , %d)",
				msg->sn, msg->company_name, msg->company_name, msg->company_addr, 
				msg->fzr_name, msg->fzr_tel, msg->qq, sgv_trnum,
				inet_ntoa(node->sin_addr) , isjingjian);
				
			if(wt_sql_exec(handle)){
				xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
				return WT_SQL_ERROR;
			}

			// 获取id号
			sprintf(handle->sql_str, "SELECT SCOPE_IDENTITY()");
			if( wt_sql_exec(handle) ){
				xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
				return WT_SQL_ERROR;
			}
			handle->sql_ret = SQLFetch(handle->sqlstr_handle);
			SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);

			usetype = 1;
			canusenum = sgv_trnum;
			new_user_flag = 1;
		} else {
			// 如果有数据 开始判断
			if( !isok ){	// isok状态
				xyprintf(0, "GUIDE_ERROR:%s %d -- This user's ok stat is error, sockfd is %d!", __FILE__, __LINE__, node->sockfd);
				error_flag = -1;
			}

			// 在线并且当前ip和原来ip不同
			if( isonline && strcmp(ydservip, inet_ntoa(node->sin_addr) ) ){
				xyprintf(0, "GUIDE_ERROR:%s %d -- This user's is online and ip is different( %s -- %s ), sockfd is %d!",
						__FILE__, __LINE__, ydservip, inet_ntoa(node->sin_addr),  node->sockfd);
				error_flag = -1;
			}

			// 如果可用 执行操作
			if(!error_flag){
				sprintf(handle->sql_str, "UPDATE mx_s_user SET companyname = '%s', companyaddress = '%s', \
						fzrname = '%s', fzrtel = '%s', qq = '%s', \
						isonline = 1, ydservip = '%s', lastdate = GETDATE(), \
						modiid = 0, modidate = GETDATE() \
						WHERE id = %u",
					msg->company_name, msg->company_addr, 
					msg->fzr_name, msg->fzr_tel, msg->qq,
					inet_ntoa(node->sin_addr),
					id );
				if(wt_sql_exec(handle)){
					xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
					return WT_SQL_ERROR;
				}
			}
		}
	}// end if(!error_flag)

	*userid = id;

	// 发送设备类型信息
	return send_rt_num_msg(version, error_flag, new_user_flag, msg->php_sockfd, id, node->sockfd, usetype, canusenum, handle);
}

/** 
 *@brief  接收认证服务器发来的状态报告
 *@param  sockfd	类型 int	epoll到有数据可接收的socket套接字
 *@return nothing
 */
void* guide_packet_process_fun( void* fd, wt_sql_handle* handle )
{
	int sockfd = (int)(long)fd;

	int ret = 0;

	bg_msg_head head;
	//接收认证服务器发来的状态报文
	if( wt_recv_block(sockfd, &head, sizeof(bg_msg_head) ) ){
		xyprintf(0, "GUIDE_ERROR:%d %s %d -- Recv guide's massage head error!", sockfd, __FILE__, __LINE__);
		goto ERR;
	}

	//判断认证服务器发来的状态报文头信息
	if(head.ver != BG_HEAD_VER || head.head != BG_HEAD_HEAD || head.len < sizeof(bg_msg_head) ){
		xyprintf(0, "GUIDE_ERROR:%d %s %d -- Report massage data error!", sockfd, __FILE__, __LINE__);
		goto ERR;
	}

	int body_len = head.len - sizeof(bg_msg_head);

	char *buf = NULL;
	if(body_len > 0){
		buf = malloc( body_len );
		memset(buf, 0, body_len);

		if( wt_recv_block(sockfd, buf, body_len ) ){
			xyprintf(0, "GUIDE_ERROR:%d %s %d -- Recv guide's massage body error!", sockfd, __FILE__, __LINE__);
			goto SOCK_ERR;
		}
	}

	if(head.order == bg_heart){
		// 数据长度效验
		if(body_len != sizeof(bg_heart_msg) ){
			xyprintf(0, "GUIDE_ERROR:%d %s %d : error!", sockfd, __FILE__, __LINE__);
			goto SOCK_ERR;
		}

		// 返回心跳
		head.len = sizeof(head);
		if( wt_send_block(sockfd, &head, sizeof(head) ) ){
			xyprintf(0, "GUIDE_ERROR:%d %s %d : Send rts chanage massage error!", sockfd, __FILE__, __LINE__);
			goto SOCK_ERR;
		}
		xyprintf(0, "%d <-- <-- Guide's heart, body_len is %d <-- <--", sockfd, body_len);
	
		// 处理
		ret = heart_process(sockfd, (bg_heart_msg *)buf, handle);
		if(ret == WT_SQL_ERROR){
			xyprintf(0, "GUIDE_ERROR:%d %s %d : error!", sockfd, __FILE__, __LINE__);
			goto SQL_ERR;
		}
		else if(ret){
			xyprintf(0, "GUIDE_ERROR:%d %s %d : error!", sockfd, __FILE__, __LINE__);
			goto SOCK_ERR;
		}
	}
	else if(head.order == bg_shebei_type){
		xyprintf(0, "%d <-- <-- Guide's shebei type %ds, body_len is %d <-- <--", sockfd, ((bg_rts_msg *)buf)->rt_num, body_len);
		
		ret = rts_process(sockfd, (bg_rts_msg *)buf, handle);
		if(ret == WT_SQL_ERROR){
			xyprintf(0, "GUIDE_ERROR:%d %s %d : error!", sockfd, __FILE__, __LINE__);
			goto SQL_ERR;
		}
		else if(ret){
			xyprintf(0, "GUIDE_ERROR:%d %s %d : error!", sockfd, __FILE__, __LINE__);
			goto SOCK_ERR;
		}
	}
	else if(head.order == bg_shanghu){
		xyprintf(0, "%d <-- <-- Guide's shanghu %ds, body_len is %d <-- <--", sockfd, ((bg_shanghu_msg *)buf)->shanghu_num, body_len);
		
		ret = shanghu_process(sockfd, (bg_shanghu_msg *)buf, handle);
		if(ret == WT_SQL_ERROR){
			xyprintf(0, "GUIDE_ERROR:%d %s %d : error!", sockfd, __FILE__, __LINE__);
			goto SQL_ERR;
		}
		else if(ret){
			xyprintf(0, "GUIDE_ERROR:%d %s %d : error!", sockfd, __FILE__, __LINE__);
			goto SOCK_ERR;
		}
	}
	else if(head.order == bg_daily_report){
		xyprintf(0, "%d <-- <-- Guide's daily report, body_len is %d <-- <--", sockfd, body_len);
		
		ret = daily_report_process(sockfd, (bg_daily_msg *)buf, handle);
		if(ret == WT_SQL_ERROR){
			xyprintf(0, "GUIDE_ERROR:%d %s %d : error!", sockfd, __FILE__, __LINE__);
			goto SQL_ERR;
		}
		else if(ret){
			xyprintf(0, "GUIDE_ERROR:%d %s %d : error!", sockfd, __FILE__, __LINE__);
			goto SOCK_ERR;
		}
	}
	else if(head.order == bg_guide){
		xyprintf(0, "%d <-- <-- Guide's guide, body_len is %d <-- <--", sockfd, body_len);
		guide_node node;
		ret = guide_sockfd_call_all(sockfd, &node);
		if(ret){
			goto SOCK_ERR;
		}

		char msg[body_len];
		memset(&msg, 0, body_len);
		int len = 0;

		// 解码
		if( MD5_MSG_RESOLOVE(head.version, buf, body_len, &msg, &len) ){
			xyprintf(0, "GUIDE_ERROR:%s %d -- Recv report massage head error, sockfd is %d!", __FILE__, __LINE__, node.sockfd);
			goto SOCK_ERR;
		}

		if(len != sizeof(bg_conn_msg)){
			xyprintf(0, "GUIDE_ERROR:%s %d -- Recv report massage head error, sockfd is %d!", __FILE__, __LINE__, node.sockfd);
			goto SOCK_ERR;
		}

		unsigned int userid;
	
		ret = basis_sn_process(head.version, &userid, (bg_conn_msg*)msg, &node, handle);
		if(ret == WT_SQL_ERROR){
			xyprintf(0, "GUIDE_ERROR:%d %s %d : error!", sockfd, __FILE__, __LINE__);
			goto SQL_ERR;
		} else if(ret){
			xyprintf(0, "GUIDE_ERROR:%d %s %d : error!", sockfd, __FILE__, __LINE__);
			goto SOCK_ERR;
		}

		// 如果id发生变化 重连
		if(userid != node.userid){
			guide_sockfd_change_id(sockfd, userid);
			xyprintf(0, "%d -- Id is change(%u --> %u)", sockfd, node.userid, userid);
		}
	}
	else if(head.order == bg_router){
		// 数据长度效验
		if(body_len != sizeof(bg_router_msg) ){
			xyprintf(0, "GUIDE_ERROR:%d %s %d : error!", sockfd, __FILE__, __LINE__);
			goto SOCK_ERR;
		}

		// 处理
		ret = router_process((bg_router_msg *)buf, handle);
		if(ret == WT_SQL_ERROR){
			xyprintf(0, "GUIDE_ERROR:%d %s %d : error!", sockfd, __FILE__, __LINE__);
			goto SQL_ERR;
		}
		else if(ret){
			xyprintf(0, "GUIDE_ERROR:%d %s %d : error!", sockfd, __FILE__, __LINE__);
			goto SOCK_ERR;
		}
	}
	else{
		xyprintf(0, "%d <-- <-- Guide's order error <-- <--", sockfd);
		goto SOCK_ERR;
	}
	
	struct list_head* pos;
	//遍历查找对应的认证服务器 然后更新状态信息
	pthread_mutex_lock(&gv_guide_list_lock);//⊙﹏⊙
	for( pos = gv_guide_list_head.next; pos != &gv_guide_list_head; pos = pos->next ){
		if( sockfd == ((guide_node*)pos)->sockfd){		//更新数据
			time( &( ((guide_node*)pos)->last_time) );
			((guide_node*)pos)->stat	= SOCK_STAT_ADD;	
			break;
		}
	}
	pthread_mutex_unlock(&gv_guide_list_lock);//⊙﹏⊙

	if(buf){
		free(buf);
	}
	return (void*) 0;
SQL_ERR:
	return (void*)WT_SQL_ERROR;
SOCK_ERR:
	if(buf){
		free(buf);
	}
ERR:
	guide_list_err(sockfd);
	return (void*) 0;
}

/** 
 *@brief  epoll线程 接收认证服务器状态报告
 *@param  fd		类型 void*		线程启动参数,未使用
 *@return nothing
 */
void* guide_epoll_thread(void *fd)
{
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ Epoll thread is running!!!");
	int ret, i;
	struct list_head *pos;	//设备list遍历变量
	
	int epfd = epoll_create( MAX_EPOLL_NUM );		//创建epoll
	if(epfd == -1){
		xyprintf(errno, "EPOLL_ERROR: %s %d -- epoll_create()", __FILE__, __LINE__);
		goto CREATE_ERR;
	}
	
	struct epoll_event *events;
	events = (struct epoll_event*)malloc (16 * sizeof(struct epoll_event) );	//一次最多获得16条结果
	if(events == NULL){
		xyprintf(0, "EPOLL_ERROR: %s %d malloc error!", __FILE__, __LINE__);
		goto CREATE_ERR;
	}

	while(1){
		pthread_mutex_lock(&gv_guide_list_lock);//⊙﹏⊙
		for( pos = gv_guide_list_head.next; pos != &gv_guide_list_head; pos = pos->next ){//遍历设备list 添加或删除设备到epoll队列中

			//将两个请求间隔 没有收到回复报文的认证服务器 置错
			if( time(0) - ((guide_node*)pos)->last_time > sgv_tupdate * 2){
				((guide_node*)pos)->stat = SOCK_STAT_DEL;
				xyprintf(0, "e~ A socket is dead -- %d", ((guide_node*)pos)->sockfd);
			}

			if( ((guide_node*)pos)->stat == SOCK_STAT_ADD ){		//需要添加到epoll列表的认证服务器
				struct epoll_event event;
				event.data.fd = ((guide_node*)pos)->sockfd;
				event.events = EPOLLIN | EPOLLRDHUP;
				ret = epoll_ctl(epfd, EPOLL_CTL_ADD, ((guide_node*)pos)->sockfd, &event);
				if(ret == -1){
					xyprintf(errno, "EPOLL_ERROR: %s %d -- epoll_ctl()", __FILE__, __LINE__);
				}
				((guide_node*)pos)->stat = SOCK_STAT_ADDED;
			}
			else if( ((guide_node*)pos)->stat == SOCK_STAT_DEL ) {	//需要从epoll列表中删除 并关闭连接的认证服务器
				struct epoll_event event;
				event.data.fd = ((guide_node*)pos)->sockfd;
				event.events = EPOLLIN | EPOLLRDHUP;
				epoll_ctl(epfd, EPOLL_CTL_DEL, ((guide_node*)pos)->sockfd, &event);	//从监听列表中删除
				
				wt_close_sock( &(((guide_node*)pos)->sockfd) );								//关闭socket
				pthread_mutex_destroy( &(((guide_node*)pos)->lock) );						//销毁锁
				
				xyprintf(0, "GUIDE:✟ ✟ Delete guide of ip is %s from the list, now gv_guide_count is %d", inet_ntoa(((guide_node*)pos)->sin_addr), gv_guide_count - 1);
				
				//从链表中删除
				struct list_head *temp = pos;
				pos = temp->prev;
				list_del( temp );
				if(temp){
					free( temp );
				}
				gv_guide_count--;
			}
		}
		pthread_mutex_unlock(&gv_guide_list_lock);//⊙﹏⊙

		//epoll_wait
		ret = epoll_wait(epfd, events, 1024, 100);
		for(i = 0; i < ret; i++){
			if( (events[i].events & EPOLLRDHUP ) || (events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || 
					( !(events[i].events & EPOLLIN) ) ){ //判断是否出错
				guide_list_err(events[i].data.fd);	//将连接置错
			}
			else {
				if(events[i].data.fd > 0){
					struct epoll_event event;
					event.data.fd = events[i].data.fd;
					event.events = EPOLLIN | EPOLLRDHUP;
					ret = epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, &event);    //从监听列表中删除
					if(ret == -1){
						xyprintf(errno, "EPOLL_ERROR: %s %d -- epoll_ctl()", __FILE__, __LINE__);
					}
					
					pool_add_worker( guide_packet_process_fun, (void*)(long) (events[i].data.fd) ); //添加到接收并处理任务队列
				} else {
					xyprintf(0,"EPOLL_ERROR:%s %d -- fd is %d", __FILE__, __LINE__, events[i].data.fd);
				}
			}

		}// end for

	}// end while(1)

CTL_ERR:
	close(epfd);
	if(events){
		free(events);
	}
CREATE_ERR:
	xyprintf(0, "GUIDE_ERROR:✟ ✟ ✟ ✟  -- %s %d:Epoll pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

/** 
 *@brief  引导服务器连接处理线程函数
 *@param  fd		类型 void*		线程启动参数,传递认证服务器信息guide_node
 *@return nothing
 */
void* guide_conn_process(void *fd)
{
	pthread_detach(pthread_self());

	// 转变格式 将创建线程时传递的数据 即带有sockfd 和 ip的空间
	guide_node *node = fd;

	//接收连接报文头 并判断
	bg_msg_head head;
	
	if( wt_recv_block(node->sockfd, &head, sizeof(bg_msg_head) ) ){
		xyprintf(0, "GUIDE_ERROR:%s %d -- Recv report massage head error, sockfd is %d!", __FILE__, __LINE__, node->sockfd);
		goto ERR;
	}

	//xyprintf_bg_msg_head(&head);
	
	if(head.ver != BG_HEAD_VER || head.head != BG_HEAD_HEAD || head.order != bg_guide ){
		xyprintf(0, "GUIDE_ERROR:%s %d -- Report massage data error, sockfd is %d!", __FILE__, __LINE__, node->sockfd);
		goto ERR;
	}
	
	// 接收连接报文体
	int len = head.len - sizeof(bg_msg_head);
	char *md5_msg = malloc(len);
	memset(md5_msg, 0, len);
	char *msg = malloc(len);
	memset(msg, 0, len);

	if( wt_recv_block(node->sockfd, md5_msg, len ) ){
		xyprintf(0, "GUIDE_ERROR:%s %d -- Recv report massage head error, sockfd is %d!", __FILE__, __LINE__, node->sockfd);
		goto MALLOC_ERR;
	}

	// 解码
	if( MD5_MSG_RESOLOVE(head.version, md5_msg, len, msg, &len) ){
		xyprintf(0, "GUIDE_ERROR:%s %d -- Recv report massage head error, sockfd is %d!", __FILE__, __LINE__, node->sockfd);
		goto MALLOC_ERR;
	}

	if( len != sizeof(bg_conn_msg)){
		xyprintf(0, "GUIDE_ERROR:%s %d -- Report massage data error, sockfd is %d!", __FILE__, __LINE__, node->sockfd);
		goto MALLOC_ERR;
	}
	
	// 打印报文接收信息 
	xyprintf(0, "%d <-- <-- Connection msg(%s) <-- <--", node->sockfd, ((bg_conn_msg*)msg)->sn);

	//数据库操作所需参数
	wt_sql_handle	*handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	
	//初始化数据库连接
	if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){
		xyprintf(0, "SQL_INIT_ERROR:%s %d -- Datebase connect error!", __FILE__, __LINE__);
		if(handle){
			free(handle);
		}
		goto MALLOC_ERR;
	}

	// 处理连接请求
	unsigned int id;
	int ret = basis_sn_process(head.version, &id, (bg_conn_msg*)msg, node, handle);
	if(ret == -1){
		goto SQL_ERR;
	}

	//填充状态 并将连接添加到链表内
	node->userid	= id;
	time(&(node->last_time));
	node->stat		= SOCK_STAT_ADD;
	pthread_mutex_init( &node->lock, 0 );
	node->get_shanghu_flag	= 0;
	node->get_shebei_type_flag = 0;

	pthread_mutex_lock(&gv_guide_list_lock);
	list_add(&(node->node), &gv_guide_list_head);
	gv_guide_count++;
	xyprintf(0, "%d -- Connection success, ip is %s, id is %d, gv_guide_count = %d", node->sockfd, inet_ntoa(node->sin_addr), id, gv_guide_count);
	pthread_mutex_unlock(&gv_guide_list_lock);

	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
	if(msg){
		free(msg);
	}
	if(md5_msg){
		free(md5_msg);
	}
	pthread_exit(NULL);

SQL_ERR:
	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
MALLOC_ERR:
	if(msg){
		free(msg);
	}
	if(md5_msg){
		free(md5_msg);
	}
ERR:
	sleep(sgv_tupdate / 2);
	close(node->sockfd);
	xyprintf(0, "GUIDE_ERROR:✟ ✟ ✟ ✟  -- %s %d -- Report_fun pthread is unnatural deaths, sockfd is %d!!", __FILE__, __LINE__, node->sockfd);
	if(node){
		free(node);
	}
	return (void*) 0;
}

/** 
 *@brief  认证服务器连接监听线程函数
 *@param  fd		类型 void*		线程启动参数,未使用
 *@return nothing
 */
void* guide_conn_thread(void *fd){
	pthread_detach(pthread_self());
	
	pthread_t pt;
	
	xyprintf(0, "** O(∩ _∩ )O ~~ Guide connection thread is running!!!");
	while(1){
		int sockfd;
		if( wt_sock_init( &sockfd, BG_PORT, MAX_EPOLL_NUM ) ){
			xyprintf(errno, "GUIDE_ERROR:0 %s %d -- wt_sock_init()", __FILE__, __LINE__);
			sleep(5);
			continue;
		}
		struct sockaddr_in client_address;//存放客户端地址信息
		int client_len = sizeof(client_address);
		while(1){
			int client_sockfd;
			client_sockfd = accept(sockfd, (struct sockaddr *)&client_address, (socklen_t *)&client_len);  
			if(client_sockfd == -1){ 
				xyprintf(errno, "GUIDE_ERROR:%d %s %d -- accept()", sockfd, __FILE__, __LINE__);
				break;
			}

			// 打印连接信息
			xyprintf(0, "** O(∩ _∩ )O ~~ Guide server of ip is %s connection, sockfd is %d", inet_ntoa( client_address.sin_addr) , client_sockfd);
			
			// 申请存放guide信息的空间 并填充sockfd 和 ip地址
			guide_node *node = malloc(sizeof(guide_node));
			if(node == NULL){
				xyprintf(0, "GUIDE_ERROR: %s %d malloc error!", __FILE__, __LINE__);
				break;
			}
			node->sockfd = client_sockfd;
			memcpy(&(node->sin_addr), &(client_address.sin_addr), sizeof(client_address.sin_addr) );
		
			// 创建线程处理guide的连接
			if( pthread_create(&pt, NULL, guide_conn_process, (void*)node ) != 0){ 
				xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
			}
		}// end while(1)
		close(sockfd);
	}
	// 远方
	xyprintf(0, "GUIDE:✟ ✟ ✟ ✟  -- %s %d:Report_start pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}
