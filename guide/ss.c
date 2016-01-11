/*****************************************************
 *
 * 与boss的交互
 *
 *****************************************************/

#include "guide_header.h"

#define BOSS_DEBUG		0

static int		BOSS_SOCKFD;						// 与boss连接的BOSS_SOCKFD

/**
 *@brief  设备操作
 *@return succ 0, failed -1
 */
int RECV_ROUTER(int len)
{
	// 数据长度校验
	if(len != sizeof(bg_router_msg) ){
		xyprintf(0, "ERROR:%s %d -- result 's msg data error!", __FILE__, __LINE__);
		return -1;
	}
	
	// 申请报文空间
	bg_router_msg *msg = malloc(len);
	memset(msg, 0, len);
	
	// 检测BOSS_SOCKFD是否可用
	if(BOSS_SOCKFD <= 0){
		xyprintf(0, "ERROR:%s %d -- sockfd is %d", __FILE__, __LINE__, BOSS_SOCKFD);
		if(msg){
			free(msg);
		}
		return -1;
	}

	// 接收
	if( wt_recv_block( BOSS_SOCKFD, msg, len ) ){
		xyprintf(errno, "ERROR:%s %d -- Recv message from error!", __FILE__, __LINE__);
		goto MSG_ERR;
	}
#if BOSS_DEBUG
	xyprintf(0, "BOSS: <-- <-- recv router(%d,%s,%s) <-- <--", msg->order, msg->type, msg->sn);
#endif

	//xyprintf_bg_router_msg(msg);

	// 初始化 数据库连接
	wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
		xyprintf(0, "SQL_INIT_ERROR -- %s %d -- Datebase connect error!", __FILE__, __LINE__);
		free(handle);
		goto MSG_ERR;
	}
	
	// 判断是需要入库还是删除
	if( msg->order == BG_ROUTER_ADD ){
		unsigned int id = 0;
		SQLBindCol( handle->sqlstr_handle, 1, SQL_C_ULONG,	&id,			64,  &handle->sql_err);
	
		//查询库中是否已经有这台设备
		sprintf(handle->sql_str, "SELECT TOP 1 id FROM mx_shebei where hard_seq = '%s'", msg->sn);
		if ( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
			goto SQLED_ERR;
		}
		
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);
		SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
		// 没有的话 查询设备型号 然后入库 没有的话 不做任何操作
		if(handle->sql_ret == SQL_NO_DATA){
			// 查询设备型号
			sprintf(handle->sql_str, "SELECT TOP 1 id FROM mx_shebei_type WHERE typecode = '%s'", msg->type);
			if ( wt_sql_exec(handle) ){
				xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
				goto SQLED_ERR;
			}
			handle->sql_ret = SQLFetch(handle->sqlstr_handle);
			SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
			
			// 添加设备
#ifdef ADVERTISING
			sprintf(handle->sql_str, "INSERT INTO mx_shebei(pici, xinghaoid, xinghao, iscandoing, \
				rukudate, ischuku, createdate, hard_seq, rid) \
				VALUES(CONVERT(varchar(100), GETDATE(), 23), %u, '%s', 1, \
				GETDATE(), 0, GETDATE(), '%s', 0)", id, msg->type, msg->sn);
#else
			sprintf(handle->sql_str, "INSERT INTO mx_shebei(pici, xinghaoid, xinghao, wanopen, iscandoing, \
				rukudate, ischuku, createdate, hard_seq, rid, heartbeatnum, hard_ver) \
				VALUES(CONVERT(varchar(100), GETDATE(), 23), %u, '%s', 1, 1, \
				GETDATE(), 0, GETDATE(), '%s', 0, 0, 0)", id, msg->type, msg->sn);
#endif
			if( wt_sql_exec(handle) ){
				xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
				goto SQLED_ERR;
			}
			xyprintf(0, "Router -- %s:%s Warehousing!!!", msg->type, msg->sn);
		}
		else {
			xyprintf(0, "Router -- %s:%s Warehoused!!!", msg->type, msg->sn);
		}

	}
	else if(msg->order == BG_ROUTER_DEL){
		unsigned int temp = 0, dailiid = 0;
		SQLBindCol( handle->sqlstr_handle, 1, SQL_C_ULONG,	&temp,			64,  &handle->sql_err);
		SQLBindCol( handle->sqlstr_handle, 2, SQL_C_ULONG,	&dailiid,		64,  &handle->sql_err);
	
		// 查询设备 设备是否绑定商户
		sprintf(handle->sql_str, "SELECT TOP 1 shanghuid, dailiid FROM mx_shebei where hard_seq = '%s'", msg->sn);
		if ( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
			goto SQLED_ERR;
		}
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);
		SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

		// 删除设备
		sprintf(handle->sql_str, "DELETE FROM mx_shebei WHERE hard_seq = '%s'", msg->sn);
		if ( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
			goto SQLED_ERR;
		}
		
		// 如果绑定了商户 修改商户的设备数量信息
		if(temp){
			unsigned int total_num = 0, ok_num = 0, shanghuid = temp;
			
			// 查询商户设备总数量
			sprintf(handle->sql_str, "SELECT count(id) FROM mx_view_useshebei WHERE shanghuid=%u", shanghuid);
			if ( wt_sql_exec(handle) ){
				xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
				goto SQLED_ERR;
			}
			handle->sql_ret = SQLFetch(handle->sqlstr_handle);
			total_num = temp;
			SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

			// 查询商户ok设备数量
			sprintf(handle->sql_str, "SELECT count(id) FROM mx_view_shenbeiok WHERE shanghuid=%u", shanghuid);
			if ( wt_sql_exec(handle) ){
				xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
				goto SQLED_ERR;
			}
			handle->sql_ret = SQLFetch(handle->sqlstr_handle);
			ok_num = temp;
			SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

			// 修改商户设备数量信息
			sprintf(handle->sql_str, "UPDATE mx_shanghu SET devicenum=%u, usenum=%u, unusenum=%u WHERE id=%u",
					total_num, ok_num, total_num - ok_num, shanghuid);
			if ( wt_sql_exec(handle) ){
				xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
				goto SQLED_ERR;
			}

			// 查询代理设备总数量
			sprintf(handle->sql_str, "SELECT count(id) FROM mx_view_useshebei WHERE dailiid=%u", dailiid);
			if ( wt_sql_exec(handle) ){
				xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
				goto SQLED_ERR;
			}
			handle->sql_ret = SQLFetch(handle->sqlstr_handle);
			total_num = temp;
			SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

			// 查询代理ok设备数量
			sprintf(handle->sql_str, "SELECT count(id) FROM mx_view_shenbeiok WHERE dailiid=%u", dailiid);
			if ( wt_sql_exec(handle) ){
				xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
				goto SQLED_ERR;
			}
			handle->sql_ret = SQLFetch(handle->sqlstr_handle);
			ok_num = temp;
			SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

			// 修改代理设备数量信息
			sprintf(handle->sql_str, "UPDATE mx_daili SET devicenum=%u, usenum=%u, unusenum=%u WHERE id=%u",
					total_num, ok_num, total_num - ok_num, dailiid);
			if ( wt_sql_exec(handle) ){
				xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
				goto SQLED_ERR;
			}
		}
		xyprintf(0, "Router -- %s Unwarehousing!!!", msg->sn);
	}
	else {
#if BOSS_DEBUG
		xyprintf(0, "BOSS:%s %d -- Order is error!!!", __FILE__, __LINE__);
#endif
		goto SQLED_ERR;
	}// end if

	// 报文空间
	char *buf = malloc(sizeof(bg_msg_head) + sizeof(bg_router_msg));
	memset(buf, 0, sizeof(bg_msg_head) + sizeof(bg_router_msg));

	// 报文头
	bg_msg_head *head = (bg_msg_head*)buf;
	head->ver	= BG_HEAD_VER;
	head->head	= BG_HEAD_HEAD;
	head->order	= bg_router;
	head->len	= sizeof(bg_msg_head) + sizeof(bg_router_msg);
	// 报文
	memcpy(buf + sizeof(bg_msg_head), msg, sizeof(bg_router_msg));

	// 检测BOSS_SOCKFD是否可用
	if(BOSS_SOCKFD <= 0){
		xyprintf(0, "ERROR:%s %d -- sockfd is %d", __FILE__, __LINE__, BOSS_SOCKFD);
		goto SEND_ERR;
	}

	// 发送
	if( wt_send_block( BOSS_SOCKFD, head, head->len ) ){
		xyprintf(0, "ERROR:%s %d -- Send msg error!", __FILE__, __LINE__);
		goto SEND_ERR;
	}

#if BOSS_DEBUG
	xyprintf(0, "BOSS:--> --> send daily --> -->");
#endif

	// 资源释放
	if(buf){
		free(buf);
	}
	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
	if(msg){
		free(msg);
	}

	return 0;

SEND_ERR:
	if(buf){
		free(buf);
	}
SQLED_ERR:
	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
MSG_ERR:
	if(msg){
		free(msg);
	}
	return -1;
}

/**
 *@brief  发送日报表
 *@return succ 0, failed -1
 */
int SEND_DAILY_REPORT()
{
	// 初始化 数据库连接
	wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	
	if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
		xyprintf(0, "SQL_INIT_ERROR:%s %d -- Datebase connect error!", __FILE__, __LINE__);
		free(handle);
		return -1;
	}

	// 报文空间
	char *buf = malloc(sizeof(bg_msg_head) + sizeof(bg_daily_msg));
	memset(buf, 0, sizeof(bg_msg_head) + sizeof(bg_daily_msg));
	bg_daily_msg *msg = (bg_daily_msg*)(buf + sizeof(bg_msg_head));
	
	// 组成昨天日期的字符串
	time_t now;
	time(&now);
	time_t yesterday_now = now - 24 * 60 * 60;
	struct tm *yesterday_tm;
	yesterday_tm = localtime(&yesterday_now);
	snprintf(msg->yesterday, sizeof(msg->yesterday), "%4d-%02d-%02d", yesterday_tm->tm_year + 1900, yesterday_tm->tm_mon + 1, yesterday_tm->tm_mday);
	xyprintf(0, "yesterday is %s", msg->yesterday);

	unsigned int count_num;
	SQLBindCol( handle->sqlstr_handle, 1, SQL_C_ULONG,	&count_num,			64,  &handle->sql_err);

	// 总设备数量
	sprintf(handle->sql_str, "SELECT COUNT(id) FROM mx_shebei");
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	msg->router_used_num	= count_num;
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
	
	// 已绑定的设备
	sprintf(handle->sql_str, "SELECT COUNT(id) FROM mx_shebei WHERE lastdotime > '%s 00:00:00'", msg->yesterday);
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	msg->router_yo_num		= count_num;
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
	
	// AP总数
	sprintf(handle->sql_str, "SELECT COUNT(id) FROM mx_ac_ap");
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	msg->ap_total_num		= count_num;
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
	
	// 昨日在线AP总数
	sprintf(handle->sql_str, "SELECT COUNT(id) FROM mx_ac_ap WHERE lastdotime > '%s 00:00:00'", msg->yesterday);
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	msg->ap_yo_num		= count_num;
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
	
	// 商户总数
	sprintf(handle->sql_str, "SELECT COUNT(id) FROM mx_shanghu");
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	msg->shanghu_num		= count_num;
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	// 报文头
	bg_msg_head *head = (bg_msg_head*)buf;
	head->ver	= BG_HEAD_VER;
	head->head	= BG_HEAD_HEAD;
	head->order	= bg_daily_report;
	head->len	= sizeof(bg_msg_head) + sizeof(bg_daily_msg);
	snprintf(head->version, sizeof(head->version), "%s", PROFRAM_VERSION);

	// 检测BOSS_SOCKFD是否可用
	if(BOSS_SOCKFD <= 0){
		xyprintf(0, "ERROR:%s %d -- sockfd is %d", __FILE__, __LINE__, BOSS_SOCKFD);
		goto SQLED_ERR;
	}

	// 发送
	if( wt_send_block( BOSS_SOCKFD, head, head->len ) ){
		xyprintf(0, "ERROR:%s %d -- Send msg error!", __FILE__, __LINE__);
		goto SQLED_ERR;
	}

#if BOSS_DEBUG
	xyprintf(0, "BOSS:--> --> send daily --> -->");
#endif

	// 资源释放
	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
	if(buf){
		free(buf);
	}
	return 0;
	
SQLED_ERR:
	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
	if(buf){
		free(buf);
	}
	return -1;
}

/**
 *@brief  发送商户信息的一部分
 *@return succ 0,failed -1
 */
int SEND_SHANGHU_PART(char* buf, int num)
{
	// 初始化 数据库连接
	wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	
	if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
		xyprintf(0, "SQL_INIT_ERROR:%s %d -- Datebase connect error!", __FILE__, __LINE__);
		free(handle);
		return -1;
	}

	// 取出报文体指针位置
	bg_shanghu_msg *msg = (bg_shanghu_msg*)(buf + sizeof(bg_msg_head));
	msg->shanghu_num = num;

	// 计算ap数量和mac数量
	unsigned int count_id;
	SQLBindCol( handle->sqlstr_handle, 1, SQL_C_ULONG,	&count_id,			64,  &handle->sql_err);
	int i = 0;
	for( ; i < num; i++){
		sprintf(handle->sql_str, "SELECT COUNT(id) FROM mx_ac_ap WHERE shanghuid = %u", msg->shanghus[i].shanghu_id);
		if ( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
			goto SQLED_ERR;
		}
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);
		msg->shanghus[i].ap_num = count_id;
		SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);

#ifdef ADVERTISING
		sprintf(handle->sql_str, "SELECT COUNT(id) FROM mx_shanghu_maclist WHERE shanghuid = %u", msg->shanghus[i].shanghu_id);
#else
		sprintf(handle->sql_str, "SELECT COUNT(id) FROM mx_shang_maclist WHERE shanghuid = %u", msg->shanghus[i].shanghu_id);
#endif
		if ( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
			goto SQLED_ERR;
		}
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);
		msg->shanghus[i].mac_num = count_id;
		SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);
	}
	
	// 报文头
	bg_msg_head *head = (bg_msg_head*)buf;
	head->ver	= BG_HEAD_VER;
	head->head	= BG_HEAD_HEAD;
	head->order	= bg_shanghu;
	head->len	= sizeof(bg_msg_head) + sizeof(bg_shanghu_msg) + num * sizeof(struct bg_shanghu_node);
	snprintf(head->version, sizeof(head->version), "%s", PROFRAM_VERSION);

	// 检测BOSS_SOCKFD是否可用
	if(BOSS_SOCKFD <= 0){
		xyprintf(0, "ERROR:%s %d -- sockfd is %d", __FILE__, __LINE__, BOSS_SOCKFD);
		goto SQLED_ERR;
	}

	// 发送
	if( wt_send_block( BOSS_SOCKFD, head, head->len) ){
		xyprintf(0, "ERROR:%s %d -- Send part msg error(%d - %u)!",
				__FILE__, __LINE__, num, head->len);
		goto SQLED_ERR;
	}

#if BOSS_DEBUG
	xyprintf(0, "BOSS: --> --> send shanghu %ds, len %u --> -->", num, head->len);
#endif
	
	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
	return 0;
SQLED_ERR:
	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
	return -1;
}

/**
 *@brief  发送商户信息
 *@return succ 0,failed -1
 */
int SEND_SHANGHU()
{
	// 初始化 数据库连接
	wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	
	if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
		xyprintf(0, "SQL_INIT_ERROR:%s %d -- Datebase connect error!", __FILE__, __LINE__);
		free(handle);
		return -1;
	}

	// 申请报文空间 按照一次发送的数量
	int count = 0;
	char* buf = malloc( sizeof(bg_msg_head) + sizeof(bg_shanghu_msg) + SHANGHU_ONCE_NUM * sizeof(struct bg_shanghu_node) );
	memset(buf, 0, sizeof(bg_msg_head) + sizeof(bg_shanghu_msg) + SHANGHU_ONCE_NUM * sizeof(struct bg_shanghu_node) );
	bg_shanghu_msg *msg = (bg_shanghu_msg*)( buf + sizeof(bg_msg_head));

	// 需要查询的数据 定义和绑定
	unsigned int	shanghu_id;
	char			name[128];
	unsigned int	isreopen;
	unsigned int	isok;
	unsigned int	router_num;
	unsigned int	use_num;
	unsigned int	unuse_num;
	unsigned int	ap_num;
	unsigned int	mac_num;
	char			retype[32];
	char			tel[32];
	char			mobile[16];
	char			address[256];
	char			fuzeren[32];
	char			zuobiao[32];
	char			modidate[32];

	SQLBindCol( handle->sqlstr_handle, 1, SQL_C_ULONG,	&shanghu_id,			64,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 2, SQL_C_CHAR,	name,					128, &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 3, SQL_C_ULONG,	&isok,					64,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 4, SQL_C_ULONG,	&router_num,			64,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 5, SQL_C_ULONG,	&use_num,				64,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 6, SQL_C_ULONG,	&unuse_num,				64,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 7, SQL_C_CHAR,	retype,					32,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 8, SQL_C_CHAR,	tel,					32,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 9, SQL_C_CHAR,	mobile,					16,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 10,SQL_C_CHAR,	address,				256, &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 11,SQL_C_CHAR,	fuzeren,				32,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 12,SQL_C_CHAR,	zuobiao,				32,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 13,SQL_C_ULONG,	&isreopen,				64,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 14,SQL_C_CHAR,	modidate,				32,  &handle->sql_err);
	
	// 查询
#ifdef ADVERTISING
	sprintf(handle->sql_str, "SELECT id, srealname, isok, devicenum, usenum, unusenum, \
			retype, tel, mobile, address, fuzeren FROM mx_shanghu");
#else
	sprintf(handle->sql_str, "SELECT id, srealname, isok, devicenum, usenum, unusenum, \
			retype, tel, mobile, address, fuzeren, zuobiao, isreopen FROM mx_shanghu");
#endif
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	
	while(handle->sql_ret != SQL_NO_DATA){
		count++;
	
		msg->shanghus[count - 1].shanghu_id	= shanghu_id;
		msg->shanghus[count - 1].isreopen	= isreopen;
		msg->shanghus[count - 1].isok		= isok;
		msg->shanghus[count - 1].router_num	= router_num;
		msg->shanghus[count - 1].use_num	= use_num;
		msg->shanghus[count - 1].unuse_num	= unuse_num;
		memcpy(msg->shanghus[count - 1].name,		name,		sizeof(name));
		memcpy(msg->shanghus[count - 1].retype,		retype,		sizeof(retype));
		memcpy(msg->shanghus[count - 1].tel,		tel,		sizeof(tel) );
		memcpy(msg->shanghus[count - 1].mobile,		mobile,		11);
		memcpy(msg->shanghus[count - 1].address,	address,	sizeof(address));
		memcpy(msg->shanghus[count - 1].fuzeren,	fuzeren,	sizeof(fuzeren));
		memcpy(msg->shanghus[count - 1].zuobiao,	zuobiao,	sizeof(zuobiao));
		memcpy(msg->shanghus[count - 1].modidate,	modidate,	sizeof(modidate));

		handle->sql_ret = SQLFetch(handle->sqlstr_handle);

		// 当数据达到SHANGHU_ONCE_NUM 调用发送函数发送发送
		if(count == SHANGHU_ONCE_NUM){
			
			if( SEND_SHANGHU_PART(buf, count) ){
				goto SQLED_ERR;
			}

			count = 0;
			memset(buf, 0, sizeof(bg_msg_head) + sizeof(bg_shanghu_msg) + SHANGHU_ONCE_NUM * sizeof(struct bg_shanghu_node) );
		}
	}

	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	// 继续发送 是0 也发送
	if( SEND_SHANGHU_PART(buf, count) ){
		goto SQLED_ERR;
	}

	// 资源清除
	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
	if(buf){
		free(buf);
	}
	return 0;
	
SQLED_ERR:
	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
	if(buf){
		free(buf);
	}
	return -1;
}

/**
 *@brief  发送设备类型
 *@return succ 0,failed -1
 */
int SEND_RTS()
{
	// 初始化 数据库连接
	wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	
	if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
		xyprintf(0, "SQL_INIT_ERROR:%s %d -- Datebase connect error!", __FILE__, __LINE__);
		free(handle);
		return -1;
	}

	// 申请报文空间
	char* buf = malloc( sizeof(bg_msg_head) + sizeof(bg_rts_msg) );
	bg_rts_msg *msg = (bg_rts_msg*)( buf + sizeof(bg_msg_head));

	int count = 0;

	// 查询数据
	unsigned int	id;
	unsigned int	curr_num;
	unsigned int	ishavewifi;
	unsigned int	ishaveacserver;
	char			name[16];
	char			driver_code[32];
	char			upurl[128];
	char			rarurl[128];
	
	SQLBindCol( handle->sqlstr_handle, 1, SQL_C_ULONG,	&id,				64,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 2, SQL_C_ULONG,	&ishavewifi,		64,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 3, SQL_C_ULONG,	&ishaveacserver,	64,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 4, SQL_C_CHAR,	name,				16,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 5, SQL_C_CHAR,	driver_code,		32,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 6, SQL_C_CHAR,	upurl,				128,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 7, SQL_C_CHAR,	rarurl,				128,  &handle->sql_err);
	
	sprintf(handle->sql_str, "SELECT id, ishavewifi, ishaveacserver, \
			typecode, drivecode, driveurl, rarurl FROM mx_shebei_type");
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	
	while(handle->sql_ret != SQL_NO_DATA){
		count++;	
		
		// 根据报文数量 调整申请的报文空间大小
		buf = realloc(buf, sizeof(bg_msg_head) + sizeof(bg_rts_msg) + count * sizeof(struct bg_rt_node) );
		msg = (bg_rts_msg*)( buf + sizeof(bg_msg_head));
		
		msg->rts[count - 1].id				= id;
		msg->rts[count - 1].ishavewifi		= ishavewifi;
		msg->rts[count - 1].ishaveacserver	= ishaveacserver;
		memcpy(msg->rts[count - 1].name,		name,			sizeof(name));
		memcpy(msg->rts[count - 1].driver_code, driver_code,	sizeof(driver_code) );
		memcpy(msg->rts[count - 1].upurl,		upurl,			sizeof(upurl));
		memcpy(msg->rts[count - 1].rarurl,		rarurl,			sizeof(rarurl));

		handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	}

	msg->rt_num	= count;
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	// 计算各类型的设备数量
	SQLBindCol( handle->sqlstr_handle, 1, SQL_C_ULONG,	&curr_num,			64,  &handle->sql_err);
	
	int i = 0;
	for( ; i < count; i++){
		sprintf(handle->sql_str, "SELECT COUNT(id) FROM mx_shebei WHERE xinghaoid = %u AND shanghuid > 0", msg->rts[i].id);
		if ( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
			goto SQLED_ERR;
		}
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);
		msg->rts[i].curr_num = curr_num;
		SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);
	}

	// 报文头
	bg_msg_head *head = (bg_msg_head*)buf;
	head->ver	= BG_HEAD_VER;
	head->head	= BG_HEAD_HEAD;
	head->order	= bg_shebei_type;
	head->len	= sizeof(bg_msg_head) + sizeof(bg_rts_msg) + count * sizeof(struct bg_rt_node);
	snprintf(head->version, sizeof(head->version), "%s", PROFRAM_VERSION);

	// 检测BOSS_SOCKFD是否可用
	if(BOSS_SOCKFD <= 0){
		xyprintf(0, "ERROR:%s %d -- sockfd is %d", __FILE__, __LINE__, BOSS_SOCKFD);
		goto SQLED_ERR;
	}

	// 发送
	if( wt_send_block( BOSS_SOCKFD, head, head->len) ){
		xyprintf(0, "ERROR:%s %d -- Send rts error!", __FILE__, __LINE__);
		goto SQLED_ERR;
	}

#if BOSS_DEBUG
	xyprintf(0, "BOSS: --> --> send rts(%d) --> -->", count);
#endif
	
	//资源释放
	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
	if(buf){
		free(buf);
	}
	return 0;
	
SQLED_ERR:
	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
	if(buf){
		free(buf);
	}
	return -1;
}

/**
 *@brief  发送心跳
 *@return succ 0, failed -1
 */
int SEND_HEART()
{
	// 初始化 数据库连接
	wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	
	if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
		xyprintf(0, "SQL_INIT_ERROR:%s %d -- Datebase connect error!", __FILE__, __LINE__);
		free(handle);
		return -1;
	}

	// 报文空间
	char *buf = malloc(sizeof(bg_msg_head) + sizeof(bg_heart_msg));
	bg_heart_msg *msg = (bg_heart_msg*)(buf + sizeof(bg_msg_head));
	
	unsigned int count_num;
	SQLBindCol( handle->sqlstr_handle, 1, SQL_C_ULONG,	&count_num,			64,  &handle->sql_err);

	// 总设备数量
	sprintf(handle->sql_str, "SELECT COUNT(id) FROM mx_shebei");
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	msg->router_total_num	= count_num;
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	// 已绑定的设备
	sprintf(handle->sql_str, "SELECT COUNT(id) FROM mx_shebei WHERE shanghuid > 0");
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	msg->router_bind_num		= count_num;
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	// 在线设备
	sprintf(handle->sql_str, "SELECT COUNT(id) FROM mx_shebei WHERE isonlien = 1");
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	msg->router_online_num	= count_num;
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	// AP总数
	sprintf(handle->sql_str, "SELECT COUNT(id) FROM mx_ac_ap");
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	msg->ap_total_num		= count_num;
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	// 在线AP总数
	sprintf(handle->sql_str, "SELECT COUNT(id) FROM mx_ac_ap WHERE isonline = 1");
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	msg->ap_online_num		= count_num;
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	// 报文头
	bg_msg_head *head = (bg_msg_head*)buf;
	head->ver	= BG_HEAD_VER;
	head->head	= BG_HEAD_HEAD;
	head->order	= bg_heart;
	head->len	= sizeof(bg_msg_head) + sizeof(bg_heart_msg);
	snprintf(head->version, sizeof(head->version), "%s", PROFRAM_VERSION);

	// 检测BOSS_SOCKFD是否可用
	if(BOSS_SOCKFD <= 0){
		xyprintf(0, "ERROR:%s %d -- sockfd is %d", __FILE__, __LINE__, BOSS_SOCKFD);
		goto SQLED_ERR;
	}

	// 发送
	if( wt_send_block( BOSS_SOCKFD, head, head->len ) ){
		xyprintf(0, "ERROR:%s %d -- Send heart error!", __FILE__, __LINE__);
		goto SQLED_ERR;
	}

#if BOSS_DEBUG
	xyprintf(0, "BOSS: --> --> send heart --> -->");
#endif

	// 资源释放
	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
	if(buf){
		free(buf);
	}
	return 0;
	
SQLED_ERR:
	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
	if(buf){
		free(buf);
	}
	return -1;
}

/**
 *@brief  接收可使用设备数量 在连接后 boss会返回这个报文
 *@return succ 0, failed -1
 */
int RECV_CANUSE_NUM(int len)
{
	// 数据长度校验
	if(len < sizeof(bg_num_msg) ){
		xyprintf(0, "ERROR:%s %d -- result 's msg data error!", __FILE__, __LINE__);
		return -1;
	}
	
	// 申请报文空间
	bg_num_msg *msg = malloc(len);
	memset(msg, 0, len);
	bg_num_msg *md5_msg = malloc(len);
	memset(md5_msg, 0, len);
	
	// 检测BOSS_SOCKFD是否可用
	if(BOSS_SOCKFD <= 0){
		xyprintf(0, "ERROR:%s %d -- sockfd is %d", __FILE__, __LINE__, BOSS_SOCKFD);
		if(md5_msg){
			free(md5_msg);
		}
		if(msg){
			free(msg);
		}
		return -1;
	}

	// 接收
	if( wt_recv_block( BOSS_SOCKFD, md5_msg, len ) ){
		xyprintf(errno, "ERROR:%s %d -- Recv message from error!", __FILE__, __LINE__);
		goto MALLOCED_ERR;
	}

	// 解码
	if( MD5_MSG_RESOLOVE(PROFRAM_VERSION, md5_msg, len, msg, &len) ){
		xyprintf(errno, "ERROR:%s %d -- Recv message error!", __FILE__, __LINE__);
		goto MALLOCED_ERR;
	}

	// 报文长度校验
	if( len != sizeof(bg_num_msg) + msg->rt_num * sizeof(struct bg_msg_rt_num) ){
		xyprintf(0, "ERROR:%s %d -- Result 's msg data error!", __FILE__, __LINE__);
		goto MALLOCED_ERR;
	}
	
#if BOSS_DEBUG
	xyprintf(0, "BOSS: <-- <-- recv canuse num <-- <--");
#endif

	//xyprintf_bg_num_msg(msg);

	// 是否需要发送返回值给php server
	if(msg->php_sockfd){
		char *res;
		if(msg->guide_flag == -1){
			res = "0";
			xyprintf(0, ".-. . ... ..-. .- .. .-..");
		}
		else {
			res = "1";
			xyprintf(0, ".-. . ... ... ..- -.-. -.-.");
		}

		if( send(msg->php_sockfd, res, strlen(res), 0) <= 0){
			xyprintf(0, "PLATFORM_ERROR:%d %s %d -- Res platform's massage error!", msg->php_sockfd, __FILE__, __LINE__);
			close(msg->php_sockfd);
			goto MALLOCED_ERR;
		}
		close(msg->php_sockfd);			// 发送后关闭socket
	}

	// 初始化 数据库连接
	wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
		xyprintf(0, "SQL_INIT_ERROR -- %s %d -- Datebase connect error!", __FILE__, __LINE__);
		free(handle);
		goto MALLOCED_ERR;
	}
	
	// 如果发送的sn号不可用
	if( msg->guide_flag < 0){
		// 修改sys_config表
		sprintf(handle->sql_str, "UPDATE mx_sys_config SET sncode = '', usetype = 0, canusenum = 0");
		if ( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
			goto SQLED_ERR;
		}

		// 修改设备类型表
		sprintf(handle->sql_str, "UPDATE mx_shebei_type SET rnum = 0");
		if ( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
			goto SQLED_ERR;
		}
		
		// 修改全局引导控制变量
		pthread_mutex_lock(&gv_boss_flag_lock);
		gv_boss_flag		= 0;
		gv_heart_interval	= msg->heart_interval;
		pthread_mutex_unlock(&gv_boss_flag_lock);
	} else {
	// 发送的sn号可用
		
		// 新用户 修改创建时间
		if( msg->guide_flag == 1 ){
			sprintf(handle->sql_str, "UPDATE mx_sys_config SET createdate = GETDATE()");
			if ( wt_sql_exec(handle) ){
				xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
				goto SQLED_ERR;
			}
		}
	
		// 设备数量控制方式 ~ 总数量
		if( msg->num_flag == BG_NUM_FLAG_TOTAL ){
			
			// 修改 sys_config 表	
			sprintf(handle->sql_str, "UPDATE mx_sys_config SET usetype = %u, canusenum = %u", msg->num_flag, msg->total_num);
			if ( wt_sql_exec(handle) ){
				xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
				goto SQLED_ERR;
			}

			// 修改全局引导控制变量
			pthread_mutex_lock(&gv_boss_flag_lock);
			gv_boss_flag		= 1;
			gv_heart_interval	= msg->heart_interval;
			gv_num_falg 		= msg->num_flag;
			gv_total_num		= msg->total_num;
			pthread_mutex_unlock(&gv_boss_flag_lock);
			
			// 修改全局引导控制变量 - 设备类型链表
			RTS_CLEAN();
		} else {
		// 设备数量控制方式 ~ 设备类型数量
			
			// 修改sys_config表
			sprintf(handle->sql_str, "UPDATE mx_sys_config SET usetype = %u", msg->num_flag);
			if ( wt_sql_exec(handle) ){
				xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
				goto SQLED_ERR;
			}
			
			// 将所有设备类型的数量置0
			sprintf(handle->sql_str, "UPDATE mx_shebei_type SET rnum = 0");
			if ( wt_sql_exec(handle) ){
				xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
				goto SQLED_ERR;
			}
	
			// 修改设备类型表	
			int i;
			for(i = 0; i < msg->rt_num; i++ ){
				sprintf(handle->sql_str, "UPDATE mx_shebei_type SET rnum = %u WHERE typecode = '%s'",
						msg->rts[i].num, msg->rts[i].name );
				if ( wt_sql_exec(handle) ){
					xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
					goto SQLED_ERR;
				}
			}
			
			// 修改全局引导控制变量
			pthread_mutex_lock(&gv_boss_flag_lock);
			gv_boss_flag		= 1;
			gv_heart_interval	= msg->heart_interval;
			gv_num_falg 		= msg->num_flag;
			pthread_mutex_unlock(&gv_boss_flag_lock);
			
			// 修改全局引导控制变量 - 设备类型链表
			RTS_CLEAN();
			for(i = 0; i < msg->rt_num; i++){
				RTS_ADD(msg->rts[i].name, msg->rts[i].num);
			}
		}// end if
	}// end if

	// 资源释放
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

	return 0;

SQLED_ERR:
	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
MALLOCED_ERR:
	if(msg){
		free(msg);
	}
	if(md5_msg){
		free(md5_msg);
	}
	return -1;
}

/**
 *@brief  发送sn和基础设置信息
 *@return
 */
int SEND_BASIS_SN(int php_sockfd)
{
	// 数据库连接初始化
	wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
		xyprintf(0, "SQL_INIT_ERROR -- %s %d -- Datebase connect error!", __FILE__, __LINE__);
		free(handle);
		return -1;
	}
	
	// 报文体
	char *buf = malloc( sizeof(bg_msg_head) + sizeof(bg_conn_msg) );
	memset(buf, 0, sizeof(bg_msg_head) + sizeof(bg_conn_msg) );
	bg_conn_msg *msg = (bg_conn_msg*)(buf + sizeof(bg_msg_head));
	msg->php_sockfd = php_sockfd;
	msg->is_streamline = 0;

	// 获取数据库内的配置内容
	SQLBindCol( handle->sqlstr_handle, 1, SQL_C_CHAR,	msg->sn,			64,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 2, SQL_C_CHAR,	msg->company_name,	128, &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 3, SQL_C_CHAR,	msg->company_addr,	256, &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 4, SQL_C_CHAR,	msg->fzr_name,		32,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 5, SQL_C_CHAR,	msg->fzr_tel,		32,  &handle->sql_err);
	SQLBindCol( handle->sqlstr_handle, 6, SQL_C_CHAR,	msg->qq,			16,  &handle->sql_err);
	
	sprintf(handle->sql_str, "SELECT TOP 1 sncode, companyname, companyaddress, fzrname, fzrtel, qq FROM mx_sys_config");
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
	
	//发送连接认证信息
	bg_msg_head *head = (bg_msg_head*)buf;
	head->ver	= BG_HEAD_VER;
	head->head	= BG_HEAD_HEAD;
	head->order	= bg_guide;
	head->len	= sizeof(bg_msg_head) + sizeof(bg_conn_msg);
	snprintf(head->version, sizeof(head->version), "%s", PROFRAM_VERSION);
	
	// 检测BOSS_SOCKFD是否可用
	if(BOSS_SOCKFD <= 0){
		xyprintf(0, "ERROR:%s %d -- sockfd is %d", __FILE__, __LINE__, BOSS_SOCKFD);
		goto SQLED_ERR;
	}
	
	char sn[64];
	memcpy(sn, msg->sn, 64);

	if( SEND_BOSSS_MSG_OF_MD5(PROFRAM_VERSION, BOSS_SOCKFD, buf, head->len) ) {
		xyprintf(errno, "ERROR:%s %d -- Send message error!", __FILE__, __LINE__);
		goto SQLED_ERR;
	}

#if BOSS_DEBUG
	xyprintf(0, "BOSS: --> --> send basis sn(%s) --> -->", sn);
#endif

	// 资源释放
	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
	if(buf){
		free(buf);
	}
	return 0;
SQLED_ERR:
	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
	if(buf){
		free(buf);
	}
	return -1;
}

/**
 *@brief  接收BOSS的指令 并验证
 *@return succ order, failed -1
 */
int RECV_BOSS_DATA(int *len, unsigned int recv_time)
{
	// 检测BOSS_SOCKFD是否可用
	if(BOSS_SOCKFD <= 0){
		xyprintf(0, "ERROR:%s %d -- sockfd is %d", __FILE__, __LINE__, BOSS_SOCKFD);
		return -1;
	}

	//设置超时时间
	struct timeval tv;	// 超时时间结构体
	tv.tv_sec = recv_time;
	tv.tv_usec = 0;
	setsockopt(BOSS_SOCKFD, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	
	// 数据接收
	bg_msg_head	msg;
	int ret = recv( BOSS_SOCKFD, (unsigned char*)&msg, sizeof(msg), 0);
	
	if( ret == 0 ){			// 连接关闭
		xyprintf(0, "ERROR:%s %d -- Connection close!!", __FILE__, __LINE__);
		return -1;
	}
	else if( ret < 0 && errno == EAGAIN ){	// 重试 应该是时间到了
		return 0;
	}
	else if( ret < 0){
		xyprintf(errno, "ERROR:%s %d -- Connection error!!", __FILE__, __LINE__);
		return -1;
	}

	// 数据判断
	if( msg.ver != BG_HEAD_VER || msg.head != BG_HEAD_HEAD || msg.len < sizeof(msg) ){
		xyprintf(0, "ERROR:%s %d -- Result 's data error!", __FILE__, __LINE__);
		return -1;
	}
	
	*len = msg.len - sizeof(msg);

	// 返回order
	return msg.order;
}

/**
 *@brief  连接引导服务器
 *@return 创建的BOSS_SOCKFD
 */
int CONN_BOSS()
{
	int BOSS_SOCKFD;
    struct sockaddr_in addr;	
	
	// socket初始化
	while(( BOSS_SOCKFD = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		xyprintf(errno, "ERROR:%s %d -- socket()", __FILE__, __LINE__);
		sleep(10);
	}

	// 域名解析
	struct hostent* hostent = gethostbyname( BG_BOSS_ADDR );
	struct in_addr ip_addr;
	ip_addr.s_addr = *(unsigned long *)hostent->h_addr;

	// 连接 失败的话 休眠3秒 重新连接
	addr.sin_family			= AF_INET;
	addr.sin_port			= htons( BG_PORT );
	addr.sin_addr.s_addr	= inet_addr( inet_ntoa( ip_addr ) );
	while( connect(BOSS_SOCKFD, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1){
		xyprintf(errno, "ERROR:%s %d -- connect()", __FILE__, __LINE__);
		sleep(10);
	}
	return BOSS_SOCKFD;
}

/** 
 *@brief  BOSS服务器指令处理线程
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* BOSS_PROCESS_THREAD(void* fd)
{
	pthread_detach( pthread_self() );
	xyprintf(0, "** O(∩ _∩ )O ~~ Process thread is running!!!");
	time_t	last_heart_time;			// 最后一次成功心跳的时间
	int		next_heart_time;			// 到下一次发送心跳的时间 可以安心的用来接收数据
	int		heart_err_count = 0;		// 心跳错误次数
	int		data_len = 0;				// 接收到的数据长度
	int		order = 0;					// boss 发来的指令类型
	
	while(1){
		// 连接boss
		BOSS_SOCKFD = CONN_BOSS();

		// 发送sn号
		if( SEND_BASIS_SN( 0 ) ){
			close(BOSS_SOCKFD);
			BOSS_SOCKFD = -1;
			continue;
		}
		
		if( SEND_HEART() ){
			close(BOSS_SOCKFD);
			BOSS_SOCKFD = -1;
			continue;
		}
		
		// 初始化第一次心跳时间
		time(&last_heart_time);
		heart_err_count = 0;

		//循环接收boss服务器的指令
		while(1){
			// 到下一次心跳的时间
			pthread_mutex_lock(&gv_boss_flag_lock);
			next_heart_time = gv_heart_interval - ( time(0) - last_heart_time );
			pthread_mutex_unlock(&gv_boss_flag_lock);

			// 如果时间小于0 即上一次心跳到现在的心跳时间大于 BG_HEART_INTERVAL
			// 增加一次心跳错误次数 次数超过3次 就关闭当前连接 重新连接
			// 不超过三次 就在10秒后 再次发送心跳
			if(next_heart_time <= 0){
				heart_err_count++;
				next_heart_time = 10;
			}

			// 接收报文
			int order = RECV_BOSS_DATA(&data_len, next_heart_time);	// 接收报文

			// 处理
			if( order == -1){						// 接收出错
				xyprintf(0, "ERROR:%s %d -- Order is -1!", __FILE__, __LINE__);
				goto SOCK_ERR;
			}
			else if(order == 0){					// 接收时间到 即需要再次发送心跳包
				if(heart_err_count >= 3){
					xyprintf(0, "ERROR:%s %d -- Not have the answer of three times!", __FILE__, __LINE__);
					goto SOCK_ERR;
				}

				if( SEND_HEART() ){
					xyprintf(0, "ERROR:%s %d -- Send message error!", __FILE__, __LINE__);
					goto SOCK_ERR;
				}
			}
			else if(order == bg_heart){				// 返回的心跳包
#if BOSS_DEBUG
				xyprintf(0, "BOSS: <-- <-- heart <-- <--");
#endif
				time(&last_heart_time);
				heart_err_count = 0;
			}
			else if(order == bg_get_shebei_type){	// 获取设备类型
#if BOSS_DEBUG
				xyprintf(0, "BOSS: <-- <-- get shebei type <-- <--");
#endif
				if( SEND_RTS()){
					xyprintf(0, "ERROR:%s %d -- Send rts error!", __FILE__, __LINE__);
					goto SOCK_ERR;
				}
			}
			else if(order == bg_get_shanghu){		// 获取商户信息
#if BOSS_DEBUG
				xyprintf(0, "BOSS: <-- <-- get shanghu <-- <--");
#endif
				if( SEND_SHANGHU()){
					xyprintf(0, "ERROR:%s %d -- Send sh error!", __FILE__, __LINE__);
					goto SOCK_ERR;
				}
			}
			else if(order == bg_canuse_num){		// 下发可用设备数量
				if( RECV_CANUSE_NUM( data_len) ){
					xyprintf(0, "ERROR:%s %d -- Recv message error!", __FILE__, __LINE__);
					goto SOCK_ERR;
				}
			}
			else if(order == bg_get_sn){			// 获取sn号
#if BOSS_DEBUG
				xyprintf(0, "BOSS: <-- <-- get sn <-- <--");
#endif
				if( SEND_BASIS_SN( 0 ) ){
					xyprintf(0, "ERROR:%s %d -- Send message error!", __FILE__, __LINE__);
					goto SOCK_ERR;
				}
			}
			else if(order == bg_router){
#if BOSS_DEBUG
				xyprintf(0, "BOSS: <-- <-- router <-- <--");
#endif
				if( RECV_ROUTER( data_len ) ){
					xyprintf(0, "ERROR:%s %d -- Recv message error!", __FILE__, __LINE__);
					goto SOCK_ERR;
				}
			}
			else {									// 错误报文
				xyprintf(0, "ERROR:%s %d -- Order(%d) is error!", __FILE__, __LINE__, order);
				goto SOCK_ERR;
			}
		
		}// end while(1)

SOCK_ERR:
		close(BOSS_SOCKFD);
		BOSS_SOCKFD = -1;
	}// end while(1)
ERR:
	xyprintf(0, "ERROR:%s %d -- Status report pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit( NULL );
}
