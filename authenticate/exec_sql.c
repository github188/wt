/*****************************************************
 *
 * 数据库操作封装函数
 *
 *****************************************************/
#include "auth_header.h"

/**
 *@brief  用户上线的数据库操作前半部分
 *@param  handle			类型 wt_sql_handle*		数据库操作handle及其他资源集合
 *@param  msg_head			类型 msg_head_st*		上线报文 报文头
 *@param  msg				类型 user_msg_st*		上线报文 报文体
 *@param  shebei_id			类型 __u32				发送报文设备的 mx_shebei 主键id
 *@param  shanghu_id		类型 __u32				发送报文设备的 mx_shanghu 主键id
 *@param  user_manyou_time	类型 __u32				可漫游时间
 *@param  *shangmac_id		类型 __u32*				返回参数 返回mx_shang_maclist主键id
 *@param  *mac_id			类型 __u32*				返回参数 返回mx_mac_list主键id
 *@return USER_***
 */
unsigned int wt_sql_user_on_first_half(wt_sql_handle *handle, msg_head_st *msg_head, user_msg_st *msg,
		__u32 shebei_id, __u32 shanghu_id, __u32 user_manyou_time, __u32 *shangmac_id, __u32 *mac_id)
{
	//定义变量
	__u32 iscanmanyou = 0, is_new_user = 0, is_in_whitelist = 0, isreopen = 0, is_in_blacklist = 0, isrenzheng = 0, userid = 0;
	__u32 dailiid;
	
	unsigned int user_ret = 0;

	//查询所登陆设备的商户是否开启无感知认证
#ifdef ADVERTISING
	SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG, &dailiid,		20, &handle->sql_err);
	sprintf(handle->sql_str, "SELECT TOP 1 dailiid FROM mx_shanghu WHERE id = %u", shanghu_id);
#else
	SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG, &isreopen,	20, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 2, SQL_C_ULONG, &dailiid,		20, &handle->sql_err);
	sprintf(handle->sql_str, "SELECT TOP 1 isreopen, dailiid FROM mx_shanghu WHERE id = %u", shanghu_id);
#endif
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return USER_SQL_ERROR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);	//释放游标

	//xyprintf(0, "isreopen = %u", isreopen);

	//如果商户开启无感知认证，则查询用户是否在商户的黑名单内，因为无感知的话所有登陆用户都没有跳转页，黑名单用户要有
	//商户没有开启无感知的话，用户是否在黑名单可以不判断，用户将过不去跳转页
	if(isreopen){
		SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG, &is_in_blacklist,	20, &handle->sql_err);
		sprintf(handle->sql_str, "SELECT TOP 1 id FROM mx_shanghu WHERE id = %u AND blacklist LIKE '%%%02x%02x%02x%02x%02x%02x%%'",
				shanghu_id, msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5]);
		if( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			return USER_SQL_ERROR;
		}
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果
		if( handle->sql_ret == SQL_NO_DATA){
			is_in_blacklist = 0;
		}
		else {
			is_in_blacklist = 1;
		}
		SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
	}
		
	//TODO: 白名单 黑名单 本地判断

	//查询用户是否在商户的白名单内
	SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG, &is_in_whitelist,	20, &handle->sql_err);
#ifdef ADVERTISING
	sprintf(handle->sql_str, "SELECT TOP 1 id FROM mx_shanghu_setmaclist WHERE id = %u AND settype = 1 AND mac = '%02x%02x%02x%02x%02x%02x'",
			shanghu_id, msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5]);
#else
	sprintf(handle->sql_str, "SELECT TOP 1 id FROM mx_shanghu WHERE id = %u AND whitelist LIKE '%%%02x%02x%02x%02x%02x%02x%%'",
			shanghu_id, msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5]);
#endif
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return USER_SQL_ERROR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果
	if( handle->sql_ret == SQL_NO_DATA){
		is_in_whitelist = 0;
	}
	else {
		is_in_whitelist = 1;
	}

	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标


	//查询用户的mac表记录，获取用户主键id，并更新表数据
	SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG, mac_id,	20, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 2, SQL_C_ULONG, &isrenzheng,	20, &handle->sql_err);
	sprintf(handle->sql_str, "SELECT TOP 1 id, isrenzheng FROM mx_maclist WHERE mac = '%02x%02x%02x%02x%02x%02x'",
			msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5]);
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return USER_SQL_ERROR;
	}
	handle->sql_ret	= SQLFetch(handle->sqlstr_handle);
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	//xyprintf(0, "mac_id = %u", *mac_id);

	if( handle->sql_ret == SQL_NO_DATA){
		//如果用户不存在，则添加新记录
		if( isreopen && !is_in_blacklist ){
			//无感知 并且不在黑名单
			sprintf(handle->sql_str, "INSERT INTO mx_maclist (mac ,firstrid, firsttime, lastrid, lasttime, lastdotime,\
					lastsetdoingtime,zonlinetime,createdate, isrenzheng)\
				VALUES ('%02x%02x%02x%02x%02x%02x', %u, GETDATE(), %u, GETDATE(), GETDATE(), GETDATE(), 0, GETDATE(), 1)",
					msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5],
					shebei_id, shebei_id);
			//xyprintf(0, "isrenzheng = 1");
		}
		else {
#ifdef ADVERTISING
			sprintf(handle->sql_str, "INSERT INTO mx_maclist (mac ,firstrid, firsttime, lastrid, lasttime, lastdotime,\
					zonlinetime,createdate) VALUES ('%02x%02x%02x%02x%02x%02x', %u, GETDATE(),\
					%u, GETDATE(), GETDATE(), 0, GETDATE())",
					msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5],
					shebei_id, shebei_id);
#else
			sprintf(handle->sql_str, "INSERT INTO mx_maclist (mac ,firstrid, firsttime, lastrid, lasttime, lastdotime,\
					lastsetdoingtime,zonlinetime,createdate) VALUES ('%02x%02x%02x%02x%02x%02x', %u, GETDATE(),\
					%u, GETDATE(), GETDATE(), GETDATE(), 0, GETDATE())",
					msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5],
					shebei_id, shebei_id);
#endif
			//xyprintf(0, "isrenzheng = 0");
		}

		if( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			return USER_SQL_ERROR;
		}
		sprintf(handle->sql_str, "SELECT SCOPE_IDENTITY()");	//获取刚插入记录的主键id号
		if( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			return USER_SQL_ERROR;
		}
		handle->sql_ret		= SQLFetch(handle->sqlstr_handle);//获取查询到的结果
		//xyprintf(0, "mac_id = %u", *mac_id);
		is_new_user = 0;
		//isrenzheng = 1;
		SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
	}
	else {
		//如果用户存在，则修改记录
		if( isreopen && !is_in_blacklist && !isrenzheng ){
			user_ret |= USER_MAC_REN;
		}
		else {
			user_ret |= USER_MAC;
		}

		is_new_user = 1;
	}

	//查询商户用户mac表，根据用户mac表id和商户id
	SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG, shangmac_id,	20, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 2, SQL_C_ULONG, &iscanmanyou,	20, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 3, SQL_C_ULONG, &isrenzheng,	20, &handle->sql_err);
#ifdef ADVERTISING
	sprintf(handle->sql_str, "SELECT TOP 1 id,iscanmanyou, isrenzheng FROM mx_shanghu_maclist WHERE macid = %u AND shanghuid = %u",
			*mac_id, shanghu_id);
#else
	sprintf(handle->sql_str, "SELECT TOP 1 id,iscanmanyou, isrenzheng FROM mx_shang_maclist WHERE macid = %u AND shanghuid = %u",
			*mac_id, shanghu_id);
#endif
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return USER_SQL_ERROR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	//xyprintf(0, "*shangmac_id = %u", *shangmac_id);

	if( handle->sql_ret == SQL_NO_DATA ){
		//如果记录不存在，添加shang mac列表 并获取主键ID
		if( isreopen && !is_in_blacklist ){
			// 如果开启无感知 则初始化认证状态
#ifdef ADVERTISING
			sprintf(handle->sql_str, "INSERT INTO mx_shanghu_maclist (macid, dailiid, shanghuid, mac, lastrid, lasttime,\
				isrenzheng, renzhengtype, firsttime, zongrenum,\
				zongtime, lastdotime, lastsetdoingtime, mactype, createdate, outmanyou, reusermanyou) \
				VALUES (%u, %u, %u, '%02x%02x%02x%02x%02x%02x', %u, GETDATE(),\
						0, 99, GETDATE(), 1,\
						0, GETDATE(), GETDATE(), %u, GETDATE(), DATEADD(n, %u, GETDATE()), %u)",
				*mac_id, dailiid, shanghu_id, msg->mac[0], msg->mac[1], msg->mac[2],
				msg->mac[3], msg->mac[4], msg->mac[5], shebei_id, is_new_user, user_manyou_time, user_manyou_time);
#else
			sprintf(handle->sql_str, "INSERT INTO mx_shang_maclist (macid, dailiid, shanghuid, mac, lastrid, lasttime,\
				isrenzheng, renzhengtype, firsttime, zongrenum,\
				zongtime, lastdotime, lastsetdoingtime, mactype, createdate, outmanyou, reusermanyou) \
				VALUES (%u, %u, %u, '%02x%02x%02x%02x%02x%02x', %u, GETDATE(),\
						0, 99, GETDATE(), 1,\
						0, GETDATE(), GETDATE(), %u, GETDATE(), DATEADD(n, %u, GETDATE()), %u)",
				*mac_id, dailiid, shanghu_id, msg->mac[0], msg->mac[1], msg->mac[2],
				msg->mac[3], msg->mac[4], msg->mac[5], shebei_id, is_new_user, user_manyou_time, user_manyou_time);
#endif
		}
		else {
			// 没有开启无感知
#ifdef ADVERTISING
			sprintf(handle->sql_str, "INSERT INTO mx_shanghu_maclist (macid, shanghuid, mac, lastrid, lasttime, isrenzheng, firsttime,\
				zongtime, lastdotime, mactype, createdate, outmanyou, reusermanyou) \
				VALUES (%u, %u, '%02x%02x%02x%02x%02x%02x', %u, GETDATE(), 0, GETDATE(),\
						0, GETDATE(), %u, GETDATE(), DATEADD(n, %u, GETDATE()), %u)",
				*mac_id, shanghu_id, msg->mac[0], msg->mac[1], msg->mac[2],
				msg->mac[3], msg->mac[4], msg->mac[5], shebei_id, is_new_user, user_manyou_time, user_manyou_time);
#else
			sprintf(handle->sql_str, "INSERT INTO mx_shang_maclist (macid, shanghuid, mac, lastrid, lasttime, isrenzheng, firsttime,\
				zongtime, lastdotime, lastsetdoingtime, mactype, createdate, outmanyou, reusermanyou) \
				VALUES (%u, %u, '%02x%02x%02x%02x%02x%02x', %u, GETDATE(), 0, GETDATE(),\
						0, GETDATE(), GETDATE(), %u, GETDATE(), DATEADD(n, %u, GETDATE()), %u)",
				*mac_id, shanghu_id, msg->mac[0], msg->mac[1], msg->mac[2],
				msg->mac[3], msg->mac[4], msg->mac[5], shebei_id, is_new_user, user_manyou_time, user_manyou_time);
#endif
		}

		if( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			return USER_SQL_ERROR;
		}

		sprintf(handle->sql_str, "SELECT SCOPE_IDENTITY()");
		if( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			return USER_SQL_ERROR;
		}
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果
		//xyprintf(0, "*shangmac_id = %u", *shangmac_id);
		iscanmanyou		= 0;
		SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
	}
	else {
		if(iscanmanyou || is_in_whitelist || ( isreopen && !is_in_blacklist ) ){
		//如果记录存在，并且可以直接上网，则修改商mac记录，如果需要跳转，则由平台来修改记录
			if(isreopen && !is_in_blacklist){
				//如果是开启无感知 并且用户不在黑名单
				if(!isrenzheng){
					//如果没有认证
					user_ret |= USER_SHANG_OPEN_REN;
				}
				else {
					//如果已经认证
					user_ret |= USER_SHANG_OPEN;
				}
			}
			else {
				//如果是在白名单或者在漫游状态
				user_ret |= USER_SHANG;
			}
		}
	}
	
	//最后操作，根据用户是否需要认证，返回不同的值
	if( iscanmanyou || (isreopen && !is_in_blacklist) || is_in_whitelist ){
		//不需要认证的时候，修改用户上一次的上网记录，并添加新的上网记录，然后返回
		if(is_in_whitelist){
			return user_ret | USER_IN_WHITE;		//在白名单内
		}
		else {
			return user_ret | USER_CAN_MANYOU;		//可以直接上网
		}
	}
	return user_ret | USER_NEED_AUTH;	// 返回用户需要认证
}

/**
 *@brief  用户上线的数据库操作前半部分
 *@param  handle			类型 wt_sql_handle*		数据库操作handle及其他资源集合
 *@param  shebei_id			类型 __u32				发送报文设备的 mx_shebei 主键id
 *@param  shanghu_id		类型 __u32				发送报文设备的 mx_shanghu 主键id
 *@param  user_manyou_time	类型 __u32				可漫游时间
 *@param  shangmac_id		类型 __u32*				mx_shang_maclist主键id
 *@param  mac_id			类型 __u32*				mx_mac_list主键id
 *@return NULL
 */
int wt_sql_user_on_after_half(wt_sql_handle *handle, __u32 shebei_id, __u32 shanghu_id, __u32 user_manyou_time, __u32 shangmac_id, __u32 mac_id, __u32 user_ret)
{
	// shangmac 认证状态 是1 mac 认证状态 不是1 的处理方法
	if( (user_ret & USER_SHANG_OPEN_REN) && !(user_ret & USER_MAC_REN) ){
		user_ret |= USER_MAC_REN;
	}
	
	if(user_ret & USER_MAC_REN){
		//xyprintf(0,"USER_MAC_REN");
		// 更新mac表 无感知 未认证
		sprintf(handle->sql_str, "UPDATE mx_maclist SET lastrid = %u, lasttime = GETDATE(), lastdotime = GETDATE(), \
				lastsetdoingtime = GETDATE(), isonline = 1, isrenzheng = 1, firstrid = %u, firsttime = GETDATE() WHERE id = %d",
				shebei_id, shebei_id, mac_id);
		if( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			return USER_SQL_ERROR;
		}
		//xyprintf(0, "isrenzheng = 1");
	}
	else if(user_ret & USER_MAC) {
		//xyprintf(0,"USER_MAC");
		// 更新mac表
#ifdef ADVERTISING
		sprintf(handle->sql_str, "UPDATE mx_maclist SET lastrid = %u, lasttime = GETDATE(), lastdotime = GETDATE(), \
				isonline = 1 WHERE id = %d",
				shebei_id, mac_id);
#else
		sprintf(handle->sql_str, "UPDATE mx_maclist SET lastrid = %u, lasttime = GETDATE(), lastdotime = GETDATE(), \
				lastsetdoingtime = GETDATE(), isonline = 1 WHERE id = %d",
				shebei_id, mac_id);
#endif
		if( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			return USER_SQL_ERROR;
		}
		//xyprintf(0, "isrenzheng = 0");
	}

	if( user_ret & USER_SHANG_OPEN_REN ){
		//xyprintf(0,"USER_SHANG_OPEN_REN");
		// 更新shangmac表 无感知 未认证
#ifdef ADVERTISING
		sprintf(handle->sql_str, "UPDATE mx_shanghu_maclist SET lastrid = %u, lasttime = GETDATE(), lastdotime = GETDATE(), \
				lastsetdoingtime = GETDATE(), isonline = 1, ispop = 1, outmanyou = DATEADD(n, %u, GETDATE()), \
				renzhengtype = 99, isrenzheng = 1, firsttime = GETDATE(), zongrenum = 1, reusermanyou = %u WHERE id = %u",
				shebei_id, user_manyou_time, user_manyou_time, shangmac_id);
#else
		sprintf(handle->sql_str, "UPDATE mx_shang_maclist SET lastrid = %u, lasttime = GETDATE(), lastdotime = GETDATE(), \
				lastsetdoingtime = GETDATE(), isonline = 1, ispop = 1, outmanyou = DATEADD(n, %u, GETDATE()), \
				renzhengtype = 99, isrenzheng = 1, firsttime = GETDATE(), zongrenum = 1, reusermanyou = %u WHERE id = %u",
				shebei_id, user_manyou_time, user_manyou_time, shangmac_id);
#endif
		if( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			return USER_SQL_ERROR;
		}
	}
	else if(user_ret & USER_SHANG_OPEN){
		//xyprintf(0,"USER_SHANG_OPEN");
		// 更新shangmac表 无感知
#ifdef ADVERTISING
		sprintf(handle->sql_str, "UPDATE mx_shanghu_maclist SET lastrid = %u, lasttime = GETDATE(), lastdotime = GETDATE(), \
				lastsetdoingtime = GETDATE(), isonline = 1, ispop = 1, outmanyou = DATEADD(n, %u, GETDATE()), \
				zongrenum = zongrenum + 1, reusermanyou = %u WHERE id = %u",
				shebei_id, user_manyou_time, user_manyou_time, shangmac_id);
#else
		sprintf(handle->sql_str, "UPDATE mx_shang_maclist SET lastrid = %u, lasttime = GETDATE(), lastdotime = GETDATE(), \
				lastsetdoingtime = GETDATE(), isonline = 1, ispop = 1, outmanyou = DATEADD(n, %u, GETDATE()), \
				zongrenum = zongrenum + 1, reusermanyou = %u WHERE id = %u",
				shebei_id, user_manyou_time, user_manyou_time, shangmac_id);
#endif
		if( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			return USER_SQL_ERROR;
		}
	}
	else if(user_ret & USER_SHANG){
		//xyprintf(0,"USER_SHANG");
		// 更新shangmac表
#ifdef ADVERTISING
		sprintf(handle->sql_str, "UPDATE mx_shanghu_maclist SET lastrid = %u, lasttime = GETDATE(), lastdotime = GETDATE(), \
				lastsetdoingtime = GETDATE(), isonline = 1, ispop = 1, outmanyou = DATEADD(n, %u, GETDATE()), \
				reusermanyou = %u WHERE id = %u",
				shebei_id, user_manyou_time, user_manyou_time, shangmac_id);
#else
		sprintf(handle->sql_str, "UPDATE mx_shang_maclist SET lastrid = %u, lasttime = GETDATE(), lastdotime = GETDATE(), \
				lastsetdoingtime = GETDATE(), isonline = 1, ispop = 1, outmanyou = DATEADD(n, %u, GETDATE()), \
				reusermanyou = %u WHERE id = %u",
				shebei_id, user_manyou_time, user_manyou_time, shangmac_id);
#endif
		if( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			return USER_SQL_ERROR;
		}
	}

	if((user_ret & USER_IN_WHITE) || (user_ret & USER_CAN_MANYOU)){
		//xyprintf(0,"USER_CAN_MANYOU");
		// log记录操作
		//update临时表记录
		sprintf(handle->sql_str, "UPDATE mx_mac_log_last SET islast = 0 WHERE id IN (SELECT id FROM mx_mac_log_last WHERE macid = %u AND islast = 1)", mac_id);
		if( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			return USER_SQL_ERROR;
		}
		
		//insert临时表记录
		 sprintf(handle->sql_str, "INSERT INTO mx_mac_log_last (macid, shanghuid, rid, lastdotime, createdate, islast, lastsetdoingtime) \
			VALUES (%u, %u, %u, GETDATE(), GETDATE(), 1, GETDATE())",
			mac_id, shanghu_id, shebei_id);
			
		//xyprintf(0, "SQL_LOG: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
		
		if( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			return USER_SQL_ERROR;
		}
	}
	return 0;
}

/**
 *@brief  用户下线的公用操作函数
 *return  success 0, failed -1
 */
int wt_sql_user_off_process(wt_sql_handle *handle, unsigned int shang_pkid, int flag, unsigned int user_manyou_time, unsigned int bw_up, unsigned int bw_down)
{
	__u32 mac_id, shanghu_id;
	SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG, &mac_id,		20, &handle->sql_err);//将变量和查询结果绑定
	SQLBindCol(handle->sqlstr_handle, 2, SQL_C_ULONG, &shanghu_id, 20, &handle->sql_err);//将变量和查询结果绑定
	//获取用户mac表id和登陆设备的商户id
#ifdef ADVERTISING
	sprintf(handle->sql_str, "SELECT TOP 1 macid, shanghuid FROM mx_shanghu_maclist WHERE id = %u", shang_pkid);
#else
	sprintf(handle->sql_str, "SELECT TOP 1 macid, shanghuid FROM mx_shang_maclist WHERE id = %u", shang_pkid);
#endif
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	//更新用户mac表，最后活动时间，最后活动时间。。。总在线时间，在线状态
#ifdef ADVERTISING
	sprintf(handle->sql_str, "UPDATE mx_maclist SET lastdotime = GETDATE(), \
			zonlinetime = zonlinetime + DATEDIFF(n, lastdotime, GETDATE()), isonline = 0 WHERE id = %u and isonline = 1",
			mac_id);
#else
	sprintf(handle->sql_str, "UPDATE mx_maclist SET lastdotime = GETDATE(), lastsetdoingtime = GETDATE(), \
			zonlinetime = zonlinetime + DATEDIFF(n, lastdotime, GETDATE()), isonline = 0 WHERE id = %u and isonline = 1",
			mac_id);
#endif
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}

	if(flag){
		//修改shang mac表，最后活动时间，最后活动时间。。。总在线时间，在线状态，pop状态。。。上下行流量，漫游结束时间
#ifdef ADVERTISING
		sprintf(handle->sql_str, "UPDATE mx_shanghu_maclist SET lastdotime = GETDATE(), \
				zongtime = zongtime + DATEDIFF(n, lastdotime, GETDATE()), isonline = 0, \
				outmanyou = DATEADD(n, %u, GETDATE()) WHERE id = %u and isonline = 1",
				user_manyou_time, shang_pkid);
#else
		sprintf(handle->sql_str, "UPDATE mx_shang_maclist SET lastdotime = GETDATE(), lastsetdoingtime = GETDATE(), \
				zongtime = zongtime + DATEDIFF(n, lastdotime, GETDATE()), isonline = 0, ispop = 0, \
				bw_up = bw_up + %u, bw_down = bw_down + %u, outmanyou = DATEADD(n, %u, GETDATE()) WHERE id = %u and isonline = 1",
				bw_up * 1024, bw_down * 1024, user_manyou_time, shang_pkid);
#endif
	}
	else {
		//修改shang mac列表,最后活动时间两个,总在线时间,在线状态,pop状态,是否可以漫游,漫游结束时间(被踢出,自然是不能漫游了,漫游结束时间也就是现在了)
#ifdef ADVERTISING
		sprintf(handle->sql_str, "UPDATE mx_shanghu_maclist SET lastdotime = GETDATE(), \
			zongtime = zongtime + DATEDIFF(n, lastdotime, GETDATE()), isonline = 0, iscanmanyou = 0, outmanyou = GETDATE() \
			WHERE id = %u and isonline = 1",
			shang_pkid);
#else
		sprintf(handle->sql_str, "UPDATE mx_shang_maclist SET lastdotime = GETDATE(), lastsetdoingtime = GETDATE(), \
			zongtime = zongtime + DATEDIFF(n, lastdotime, GETDATE()), isonline = 0, ispop = 0, iscanmanyou = 0, outmanyou = GETDATE() \
			WHERE id = %u and isonline = 1",
			shang_pkid);
#endif
	}
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
	
	//修改用户上网记录表，最后活动时间，最后活动时间。。。总在线时间，islast状态
#ifdef ADVERTISING
	sprintf(handle->sql_str, "UPDATE mx_mac_log_last SET lastdotime = GETDATE(), \
			zongtime = zongtime + DATEDIFF(n, lastdotime, GETDATE()), islast = 0 WHERE id IN ( SELECT id FROM mx_mac_log_last WHERE macid = %u and islast = 1 )"
			, mac_id );
#else
	sprintf(handle->sql_str, "UPDATE mx_mac_log_last SET lastdotime = GETDATE(), lastsetdoingtime = GETDATE(), \
			zongtime = zongtime + DATEDIFF(n, lastdotime, GETDATE()), islast = 0 WHERE id IN ( SELECT id FROM mx_mac_log_last WHERE macid = %u and islast = 1 )"
			, mac_id );
#endif
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
	
	return 0;
}

/**
 *@brief  用户下线的数据库操作
 *@param  handle			类型 wt_sql_handle*		数据库操作handle及其他资源集合
 *@param  msg_head			类型 msg_head_st*		下线报文 报文头
 *@param  msg				类型 user_msg_st*		下线报文 报文体
 *@param  user_manyou_time	类型 __u32				可漫游时间
 *@return success 0,failed -1
 */
int wt_sql_user_off(wt_sql_handle *handle, msg_head_st *msg_head, user_msg_st *msg, __u32 user_manyou_time)
{
	//user_msg_st报文内的user_id是用来存放平台数据的，路由器会返回上一次下发user_msg_st的值返回
	__u32 shang_pkid = atoi(&(msg->user_id[2]));
	//xyprintf(0,"offline -- shang_pkid = %u", shang_pkid);
	return wt_sql_user_off_process(handle, shang_pkid, 1, user_manyou_time, msg->flow[0], msg->flow[1]);
}

/**
 *@brief  tupdatelistci心跳包的数据库操作
 *@param  handle			类型 wt_sql_handle*		数据库操作handle及其他资源集合
 *@param  shebei_id			类型 __u32				发送报文设备的 mx_shebei 主键id
 *@param  user_manyou_time	类型 __u32				可漫游时间
 *@return success 0,failed -1
 */
int wt_sql_heart(wt_sql_handle *handle, __u32 shebei_id, __u32 tupdatelist, __u32 user_manyou_time)
{
	//更新设备表，最后活动时间，最后活动时间。。。总活动时间，在线状态
#ifdef ADVERTISING
	sprintf(handle->sql_str, "UPDATE mx_shebei SET lastdotime = GETDATE(), \
			doingtime = doingtime + DATEDIFF(n, lastdotime, GETDATE()), isonlien = 1 WHERE id = %u",
			shebei_id);
#else
	sprintf(handle->sql_str, "UPDATE mx_shebei SET lastdotime = GETDATE(),lastsetdoingtime = GETDATE(), \
			doingtime = doingtime + DATEDIFF(n, lastdotime, GETDATE()), isonlien = 1 WHERE id = %u",
			shebei_id);
#endif
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
/*	//更新设备表记录表，总时间，最后活动时间
	sprintf(handle->sql_str, "UPDATE mx_shenbei_log_last SET zongtime = zongtime + %u, \
			lastdotime = GETDATE() WHERE rid = %u AND islast = 1",
			tupdatelist / 2, shebei_id);
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
	//更新用户mac表，总在线时间，最后活动时间，最后活动时间。。。
	sprintf(handle->sql_str, "UPDATE mx_maclist SET zonlinetime = zonlinetime + DATEDIFF(n, lastdotime, GETDATE()),\
			lastdotime = GETDATE(), lastsetdoingtime = GETDATE() \
			WHERE lastrid = %u AND isonline = 1 AND isrenzheng = 1",
			shebei_id);
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
	//更新shang_maclist表，总时间，最后活动时间两个，漫游结束时间
	sprintf(handle->sql_str, "UPDATE mx_shang_maclist SET zongtime = zongtime + DATEDIFF(n, lastdotime, GETDATE()), \
			lastdotime = GETDATE(), lastsetdoingtime = GETDATE() , outmanyou = DATEADD(n, %u, GETDATE() ) \
			WHERE lastrid = %u AND isonline = 1 AND isrenzheng = 1",
			user_manyou_time, shebei_id);
	//xyprintf(0, "%s", handle->sql_str);
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
	//更新用户上网记录表，总在线时间，最后活动时间两个。。。
	sprintf(handle->sql_str, "UPDATE mx_mac_log_last SET zongtime = zongtime + DATEDIFF(n, lastdotime, GETDATE()), \
			lastdotime = GETDATE(), lastsetdoingtime = GETDATE() WHERE rid = %u AND islast = 1",
			shebei_id);
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
*/
	//更新shang_maclist表，总时间，最后活动时间两个，漫游结束时间
#ifdef ADVERTISING
	sprintf(handle->sql_str, "UPDATE mx_shanghu_maclist SET outmanyou = DATEADD(n, %u, GETDATE() ) \
			WHERE id IN ( SELECT id FROM mx_shanghu_maclist WHERE lastrid = %u AND isonline = 1 AND isrenzheng = 1)",
			user_manyou_time, shebei_id);
#else
	sprintf(handle->sql_str, "UPDATE mx_shang_maclist SET outmanyou = DATEADD(n, %u, GETDATE() ) \
			WHERE id IN ( SELECT id FROM mx_shang_maclist WHERE lastrid = %u AND isonline = 1 AND isrenzheng = 1)",
			user_manyou_time, shebei_id);
#endif
	//xyprintf(0, "%s", handle->sql_str);
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
	return 0;
}

/**
 *@brief  用户操作回复报文的数据库操作
 *@param  handle		类型 wt_sql_handle*		数据库操作handle及其他资源集合
 *@param  msg_head		类型 msg_head_st*		上线报文 报文头
 *@param  msg			类型 user_msg_st*		上线报文 报文体
 *@return success 0,failed -1
 */
int wt_sql_result(wt_sql_handle *handle, msg_head_st *msg_head, user_msg_st *msg)
{
	//user_msg_st报文内的user_id是用来存放平台数据的，路由器会返回上一次下发user_msg_st的值返回
	//xyprintf_user_msg_head(msg);
	
	char order;
	unsigned int shang_pkid;
	int sockfd;
	char* str = msg->user_id;
	order = *str;
	str++;
	if(*str == '|'){
		str++;
		shang_pkid = atoi(str);
		while( *str && *str != '|'){
			str++;
		}
		if(*str == '|'){
			str++;
			sockfd = atoi(str);
		}
	}
	
	//xyprintf(0, "order = %c", order);
	//xyprintf(0, "sockfd = %d", sockfd);
	//xyprintf(0, "shang_pkid = %u", shang_pkid);

	//SCANSQL 认证后允许用户上网报文 后的回复报文
	if(order == USER_ID_SCANSQL){
		if(msg->result == user_result_succ){	//如果操作成功
			// 发送返回值
			char *res = "1";
			if( send(sockfd, res, strlen(res), 0) <= 0){
				xyprintf(0, "PLATFORM_ERROR:%d %s %d -- Res platform's massage error!", sockfd, __FILE__, __LINE__);
			}
		}
		else {									//如果操作没有成功
			// 发送返回值
			char *res = "2";
			if( send(sockfd, res, strlen(res), 0) <= 0){
				xyprintf(0, "PLATFORM_ERROR:%d %s %d -- Res platform's massage error!", sockfd, __FILE__, __LINE__);
			}

			//打印报文信息
			xyprintf(0, "DATA_ERROR:%s %d -- SQANSQL message result failed! mac = %02x%02x%02x%02x%02x%02x, auth_addr = %s",
					__FILE__, __LINE__, msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5], msg->auth_addr);
		}
		wt_close_sock( &sockfd );
	}
	else if( order == USER_ID_TAKEOUT ){	//踢出的回复报文,将用户添加到黑名单后
		if( msg->result == user_result_succ ){	//成功
			//用户被踢下线也是下线，因此需要修改mac表，shang_mac表，mac记录表
			return wt_sql_user_off_process(handle, shang_pkid, 0, 0, 0, 0);
		}
		else {	//如果失败了
			//打印报文信息
			xyprintf(0, "DATA_ERROR:%s %d -- TAKEOUT message result failed! mac = %02x%02x%02x%02x%02x%02x, auth_addr = %s",
					__FILE__, __LINE__, msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5], msg->auth_addr);
		}
	}
	else if( order == USER_ID_TIMEOUT ){	//到时间 后的回复报文
		//if(msg->result == user_result_succ){
		//为什么不判断是否失败呢? 
			//也要享受下线的待遇 各种更新
			return wt_sql_user_off_process(handle, shang_pkid, 0, 0, 0, 0);
		//}//end if(msg->result == user_result_succ)
	}
	return 0;
}

/**
 *@brief  收到qq上报信息后的数据库操作
 *@param  handle		类型 wt_sql_handle*		数据库操作handle及其他资源集合
 *@param  msg_head		类型 msg_head_st*		qq上报报文 报文头
 *@param  msg			类型 third_qq_msg_st*	qq上报报文 报文体
 *@param  shebei_pkid	类型 __u32				发送报文设备的 mx_shebei 主键id
 *@return success 0,failed -1
 */
int wt_sql_third_qq(wt_sql_handle *handle, msg_head_st *msg_head, third_qq_msg_st *msg, __u32 shebei_pkid)
{
	if(msg->qq_num < 100000){
		return 0;
	}

	__u32 mac_id = 0, mac_qq_id = 0, isrenzheng = 0;
	SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG, &mac_id,		20, &handle->sql_err);//将变量和查询结果绑定
	SQLBindCol(handle->sqlstr_handle, 2, SQL_C_ULONG, &isrenzheng,	20, &handle->sql_err);//将变量和查询结果绑定
	//查询用户mac列表 获取用户主键id 并更新表数据
	sprintf(handle->sql_str, "SELECT TOP 1 id, isrenzheng FROM mx_maclist WHERE mac = '%02x%02x%02x%02x%02x%02x'",
			msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5]);
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	if(handle->sql_ret == SQL_NO_DATA){
		return 0;
	}
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	if(isrenzheng = 0){
		return 0;
	}

	//查询是否存在mac与qq对应的记录
	SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG, &mac_qq_id, 20, &handle->sql_err);//将变量和查询结果绑定
	sprintf(handle->sql_str, "SELECT TOP 1 id FROM mx_maclist_qq WHERE macid = %u AND qq = %u", mac_id, msg->qq_num);
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	//不存在插入 存在更新
	if(handle->sql_ret == SQL_NO_DATA){
		sprintf(handle->sql_str, "INSERT INTO mx_maclist_qq (macid, qq, loginnum, shebeitype, lastrid, lastdate, createdate) \
				VALUES (%u, %u, 1, %u, %u, GETDATE(), GETDATE())", mac_id, msg->qq_num, msg->qq_type, shebei_pkid);
		if( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			return -1;
		}
	}
	else {
/*		sprintf(handle->sql_str, "UPDATE mx_maclist_qq SET loginnum = loginnum + 1, shebeitype = %u, lastrid = %u, lastdate = GETDATE() WHERE id = %u",
				msg->qq_type, shebei_pkid, mac_qq_id);
		if( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			return -1;
		}
*/	
	}
	return 0;
}

/**
 *@brief  获取版本号回复报文的数据库操作
 *@param  handle		类型 wt_sql_handle*			数据库操作handle及其他资源集合
 *@param  msg_head		类型 msg_head_st*			获取版本号报文 报文头
 *@param  msg			类型 char*	获取版本号报文 报文体
 *@param  shebei_pkid	类型 __u32					发送报文设备的 mx_shebei 主键id
 *@return success 0,failed -1
 */
int wt_sql_version_refresh(wt_sql_handle *handle, char *version, __u32 shebei_pkid)
{
	sprintf(handle->sql_str, "UPDATE mx_shebei SET driverversion = '%s' WHERE id = %u", version, shebei_pkid);
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
	return 0;
}

/**
 *@brief  重启操作的回复报文的数据库操作
 *@param  handle		类型 wt_sql_handle*		数据库操作handle及其他资源集合
 *@param  shebei_pkid	类型 __u32				发送报文设备的 mx_shebei 主键id
 *@return success 0,failed -1
 */
int wt_sql_reboot_res_succ(wt_sql_handle *handle, __u32 shebei_pkid)
{
	sprintf( handle->sql_str, "UPDATE mx_shebei SET isrestart = 0 WHERE id = %u",shebei_pkid );
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
	return 0;
}

/**
 *@brief  无线设置上报报文后的数据库操作
 *@param  handle		类型 wt_sql_handle*			数据库操作handle及其他资源集合
 *@param  msg_head		类型 msg_head_st*			无线设置上报报文 报文头
 *@param  msg			类型 wifi_base_conf_msg_st*	无线设置上报报文 报文体
 *@param  shebei_pkid	类型 __u32					发送报文设备的 mx_shebei 主键id
 *@return 需要修改返回1, 不需要修改返回0,failed -1
 */
int wt_sql_wireless_config(wt_sql_handle *handle, msg_head_st *msg_head, wifi_base_conf_msg_st *msg, __u32 shebei_pkid, __u32 shanghuid, int flag_of_5g)
{

#define WIRELESS_DEBUG	0

	//定义变量,对应mx_设备表的各项
	__u32 openwifi = 0, wifitype = 0, pindao = 0, apstatus = 0, outvalue = 0, wifigonglv = 0, pindaomode = 0, yxmoshi = 0, daikuan = 0, kuopindao = 0, isusepass = 0;
	char wifipassword[64] = { 0 };
	char defssid[32] = { 0 };
	SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG, &openwifi,	20, &handle->sql_err);//无线开关 1开 0关 wifi_enable
	SQLBindCol(handle->sqlstr_handle, 2, SQL_C_ULONG, &wifitype,	20, &handle->sql_err);//网络模式 wifi_mode
	SQLBindCol(handle->sqlstr_handle, 3, SQL_C_ULONG, &pindao,		20, &handle->sql_err);//频道 wifi_channel
	SQLBindCol(handle->sqlstr_handle, 4, SQL_C_ULONG, &apstatus,	20, &handle->sql_err);//ap隔离 wifi_isolated
	SQLBindCol(handle->sqlstr_handle, 5, SQL_C_ULONG, &outvalue,	20, &handle->sql_err);//信号低于阀值主动踢下线 wifi_rssi_down
	SQLBindCol(handle->sqlstr_handle, 6, SQL_C_ULONG, &wifigonglv,	20, &handle->sql_err);//无线发射功率 wifi_txpower
	SQLBindCol(handle->sqlstr_handle, 7, SQL_C_CHAR,  &defssid,		32, &handle->sql_err);//默认ssid
	SQLBindCol(handle->sqlstr_handle, 8, SQL_C_ULONG, &yxmoshi,		20, &handle->sql_err);//运行模式1: 增强,0: 普通 run_mode
	SQLBindCol(handle->sqlstr_handle, 9, SQL_C_ULONG, &daikuan,		20, &handle->sql_err);//带宽1: 20/40MHz ,0: 20MHz HT_BW
	SQLBindCol(handle->sqlstr_handle, 10,SQL_C_ULONG, &kuopindao,	20, &handle->sql_err);//扩展频道,仅带宽为20/40MHz时有效 HT_EXTCHA
	SQLBindCol(handle->sqlstr_handle, 11,SQL_C_ULONG, &pindaomode,	20, &handle->sql_err);//频道模式1: 单频, 0: 双频 HT_BSSCoexistence
	SQLBindCol(handle->sqlstr_handle, 12,SQL_C_ULONG, &isusepass,	20, &handle->sql_err);//默认无线是否使用密码
	SQLBindCol(handle->sqlstr_handle, 13,SQL_C_CHAR,  &wifipassword,64, &handle->sql_err);//无线密码

	//查询该设备的wireless配置
	sprintf(handle->sql_str, "SELECT TOP 1 openwifi, wifitype, pindao, apstatus, outvalue, wifigonglv, ssid, yxmoshi, daikuan, \
			kuopindao, pindaomode, isusepass, wifipassword FROM mx_shebei WHERE id = %u",	shebei_pkid);
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果
	if( handle->sql_ret == SQL_NO_DATA){
		return 0;
	}
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	int ischange = 0;	//修改标志 如果有需要修改的项 则置为1 之后根据是0是1 判断是否需要下发无线修改报文 毕竟无线修改报文很大的,.,,,,
	//判断当前路由器基础设置和 数据库内的设置是否存在不同 不同则修改报文
	
	if(flag_of_5g){
		if(msg->wifi_mode != 14){
			ischange = 1;
#if WIRELESS_DEBUG
			xyprintf(0," (΄ ◞ิ౪ ◟ิ ‵) Wireless has change in -- %s %s %d", __FILE__, __func__, __LINE__);
#endif
			msg->wifi_mode = 14;
		}

		if(openwifi != msg->wifi_enable || apstatus != msg->wifi_isolated || outvalue != msg->wifi_rssi_down || wifigonglv != msg->wifi_txpower){
			ischange = 1;
#if WIRELESS_DEBUG
			xyprintf(0," (΄ ◞ิ౪ ◟ิ ‵) Wireless has change in -- %s %s %d", __FILE__, __func__, __LINE__);
			xyprintf(0," openwifi = %u, %u", openwifi, msg->wifi_enable);
			xyprintf(0," apstatus = %u, %u", apstatus, msg->wifi_isolated);
			xyprintf(0," outvalue = %u, %u", outvalue, msg->wifi_rssi_down);
			xyprintf(0," wifigonglv = %u, %u", wifigonglv, msg->wifi_txpower);
#endif
			msg->wifi_enable	= openwifi;
			msg->wifi_isolated	= apstatus;
			msg->wifi_rssi_down = outvalue;
			msg->wifi_txpower	= wifigonglv;
		}
	} else {
		if(openwifi != msg->wifi_enable || wifitype != msg->wifi_mode ||
		   apstatus != msg->wifi_isolated || outvalue != msg->wifi_rssi_down || wifigonglv != msg->wifi_txpower){
			ischange = 1;
#if WIRELESS_DEBUG
			xyprintf(0," (΄ ◞ิ౪ ◟ิ ‵) Wireless has change in -- %s %s %d", __FILE__, __func__, __LINE__);
			xyprintf(0," openwifi = %u, %u", openwifi, msg->wifi_enable);
			xyprintf(0," wifitype = %u, %u", wifitype, msg->wifi_mode);
			xyprintf(0," apstatus = %u, %u", apstatus, msg->wifi_isolated);
			xyprintf(0," outvalue = %u, %u", outvalue, msg->wifi_rssi_down);
			xyprintf(0," wifigonglv = %u, %u", wifigonglv, msg->wifi_txpower);
#endif
			msg->wifi_enable	= openwifi;
			msg->wifi_mode		= wifitype;
			msg->wifi_isolated	= apstatus;
			msg->wifi_rssi_down = outvalue;
			msg->wifi_txpower	= wifigonglv;
		}
	}

	if(flag_of_5g){
//		if(msg->wifi_channel){
//			ischange = 1;
#if WIRELESS_DEBUG
//			xyprintf(0," (΄ ◞ิ౪ ◟ิ ‵) Wireless has change in -- %s %s %d", __FILE__, __func__, __LINE__);
#endif
//			msg->wifi_channel = 40;
//		}
	} else {
		//频道为0时, 随机下发1,6,11,因为在1,6,11时候信号最好
		if(pindao == 0){
			//频道为1,6,11时 信号最好
			if( msg->wifi_channel != 1 && msg->wifi_channel != 6 && msg->wifi_channel != 11 ){
				ischange = 1;
#if WIRELESS_DEBUG
				xyprintf(0," (΄ ◞ิ౪ ◟ิ ‵) Wireless has change in -- %s %s %d", __FILE__, __func__, __LINE__);
#endif
				msg->wifi_channel = (time(0) % 3) * 5 + 1;
			}
		}
		else if( pindao != msg->wifi_channel){
			ischange = 1;
#if WIRELESS_DEBUG
			xyprintf(0," (΄ ◞ิ౪ ◟ิ ‵) Wireless has change in -- %s %s %d", __FILE__, __func__, __LINE__);
#endif
			msg->wifi_channel = pindao;
		}
	}

	// 5g的不开启增强模式
	if(flag_of_5g){
		if( msg->run_mode ){
			ischange = 1;
#if WIRELESS_DEBUG
			xyprintf(0," (΄ ◞ิ౪ ◟ิ ‵) Wireless has change in -- %s %s %d", __FILE__, __func__, __LINE__);
#endif
			msg->run_mode = 0;
		}
	} else {
		if(msg->run_mode != yxmoshi || msg->HT_BW != daikuan || msg->HT_BSSCoexistence != pindaomode ){
/*
			xyprintf(0, "msg->run_mode = %u, yxmoshi = %u\n\
					msg->HT_BW = %u, daikuan = %u\n\
					msg->HT_EXTCHA = %u, kuopindao = %u\n\
					msg->HT_BSSCoexistence = %u, pindaomode = %u",
					msg->run_mode, yxmoshi,
					msg->HT_BW, daikuan,
					msg->HT_EXTCHA, kuopindao,
					msg->HT_BSSCoexistence, pindaomode);
*/
			ischange = 1;
#if WIRELESS_DEBUG
			xyprintf(0," (΄ ◞ิ౪ ◟ิ ‵) Wireless has change in -- %s %s %d", __FILE__, __func__, __LINE__);
#endif
			msg->run_mode = yxmoshi;
			msg->HT_BW = daikuan;
			msg->HT_BSSCoexistence = pindaomode;
		}
	}

	if(daikuan){
		if(msg->HT_EXTCHA != kuopindao){
			ischange = 1;
#if WIRELESS_DEBUG
			xyprintf(0," (΄ ◞ิ౪ ◟ิ ‵) Wireless has change in -- %s %s %d", __FILE__, __func__, __LINE__);
#endif
			msg->HT_EXTCHA = kuopindao;
		}
	}
/*
	int i;
	printf("\n");
	for(i = 0; i < 32; i++){
		printf("%02x ", (unsigned char)defssid[i]);
		if(i % 8 == 7 && i != 0){
			printf("\n");
		}
	}

	printf("\n");
	for(i = 0; i < 32; i++){
		printf("%02x ", msg->list[0].ssid[i]);
		if(i % 8 == 7 && i != 0){
			printf("\n");
		}
	}
	printf("\n");

*/
	if(flag_of_5g){
		if(strlen(defssid) < sizeof(defssid) - 3){
			strcpy(defssid + strlen(defssid), "5G");
		}
	}
		

	//判断当前路由器前两个ssid设置 是否被修改
	__u32 ssid_num;
	for(ssid_num = 0; ssid_num < 2; ssid_num++){
			//xyprintf(0, "1.ssid -- %x %x %x %x %x %x %x %x %x %x %x %x", defssid[0], defssid[1],
			//		defssid[2], defssid[3],	defssid[4], defssid[5],
			//		defssid[6], defssid[7],	defssid[8], defssid[9],
			//		defssid[10], defssid[11]);

		if( strncmp(defssid, msg->list[ssid_num].ssid, sizeof(msg->list[ssid_num].ssid)) ||
				msg->list[ssid_num].hide ||
				msg->list[ssid_num].isolated ||
				msg->list[ssid_num].ssid_charset != ssid_num){
			ischange = 1;
#if WIRELESS_DEBUG
			xyprintf(0," (΄ ◞ิ౪ ◟ิ ‵) Wireless has change in -- %s %s %d", __FILE__, __func__, __LINE__);
#endif
			strncpy( msg->list[ssid_num].ssid, defssid, sizeof(msg->list[ssid_num].ssid));
			//xyprintf(0, "2.ssid -- %x %x %x %x %x %x %x %x %x %x %x %x", msg->list[ssid_num].ssid[0], msg->list[ssid_num].ssid[1],
			//		msg->list[ssid_num].ssid[2], msg->list[ssid_num].ssid[3],
			//		msg->list[ssid_num].ssid[4], msg->list[ssid_num].ssid[5],
			//		msg->list[ssid_num].ssid[6], msg->list[ssid_num].ssid[7],
			//		msg->list[ssid_num].ssid[8], msg->list[ssid_num].ssid[9],
			//		msg->list[ssid_num].ssid[10], msg->list[ssid_num].ssid[11]);
			msg->list[ssid_num].hide			= 0;
			msg->list[ssid_num].isolated		= 0;
			msg->list[ssid_num].ssid_charset	= ssid_num;
		}
		
		//如果使用加密的话 判断密码设置是否被更改
		if(isusepass){
			//加密方式是否为6
			if(msg->list[ssid_num].security_mode != 6){	//使用加密
				ischange = 1;
#if WIRELESS_DEBUG
				xyprintf(0," (΄ ◞ิ౪ ◟ิ ‵) Wireless has change in -- %s %s %d", __FILE__, __func__, __LINE__);
#endif
				msg->list[ssid_num].security_mode = 6;
			}
			//密码设置是否相同
			if( strcmp( msg->list[ssid_num].key.wpa.key, wifipassword ) || 
				msg->list[ssid_num].key.wpa.rekeyinterval != 3600 ||
				msg->list[ssid_num].key.wpa.encrypt_type != 0 ){
					ischange = 1;
#if WIRELESS_DEBUG
					xyprintf(0," (΄ ◞ิ౪ ◟ิ ‵) Wireless has change in -- %s %s %d", __FILE__, __func__, __LINE__);
#endif
					strcpy( msg->list[ssid_num].key.wpa.key, wifipassword );	//密码
					msg->list[ssid_num].key.wpa.rekeyinterval	= 3600;			//密钥更新间隔(网络字节序)
					msg->list[ssid_num].key.wpa.encrypt_type	= 0;			//1:TKIP,2:AES,0:TKIP/AES

			}
		}
		else {
			//没有使用加密
			if(msg->list[ssid_num].security_mode != 0){
				ischange = 1;
#if WIRELESS_DEBUG
				xyprintf(0," (΄ ◞ิ౪ ◟ิ ‵) Wireless has change in -- %s %s %d", __FILE__, __func__, __LINE__);
#endif
				msg->list[ssid_num].security_mode = 0;
			}
		}// end if
	}// end for

	//查询是否设置了其他ssid
	char ssid[32], thispassword[64];
	__u32 ishide, isgeli, thiscode;
	SQLBindCol(handle->sqlstr_handle, 1, SQL_C_CHAR,  ssid,			32, &handle->sql_err);//ssid		ssid
	SQLBindCol(handle->sqlstr_handle, 2, SQL_C_ULONG, &ishide,		20, &handle->sql_err);//是否隐藏	hide
	SQLBindCol(handle->sqlstr_handle, 3, SQL_C_ULONG, &isgeli,		20, &handle->sql_err);//是否隔离	isolated
	SQLBindCol(handle->sqlstr_handle, 4, SQL_C_ULONG, &thiscode,	20, &handle->sql_err);//编码 0GB2112 1UTF-8	ssid_charset
	SQLBindCol(handle->sqlstr_handle, 5, SQL_C_CHAR,  thispassword, 64, &handle->sql_err);//密码		key
	SQLBindCol(handle->sqlstr_handle, 6, SQL_C_ULONG, &isusepass,	20, &handle->sql_err);//是否加密	security_mode
	sprintf(handle->sql_str, "SELECT TOP 3 ssid, ishide, isgeli, thiscode, thispassword, isusepass FROM mx_shebei_wifi WHERE rid = %u and shanghuid = %u",
			shebei_pkid, shanghuid);
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果
	while( handle->sql_ret != SQL_NO_DATA){

		if(ssid[0] != 0 || msg->list[ssid_num].ssid[0] != 0){	//如果两个都为空 即没有启用 便不判断其他值
			//判断设置的其他ssid和当前路由器设置是否相同

			if(flag_of_5g){
				if(strlen(ssid) < sizeof(ssid) - 3){
					strcpy(ssid + strlen(ssid), "5G");
				}
			}
			
			if( strncmp(ssid, msg->list[ssid_num].ssid, sizeof(msg->list[ssid_num].ssid)) ||
					msg->list[ssid_num].hide != ishide ||
					msg->list[ssid_num].isolated != isgeli ||
					msg->list[ssid_num].ssid_charset != thiscode){
				ischange = 1;
#if WIRELESS_DEBUG
				xyprintf(0," (΄ ◞ิ౪ ◟ิ ‵) Wireless has change in -- %s %s %d", __FILE__, __func__, __LINE__);
#endif
				strncpy(msg->list[ssid_num].ssid, ssid, sizeof(msg->list[ssid_num].ssid));
				msg->list[ssid_num].hide			= ishide;
				msg->list[ssid_num].isolated		= isgeli;
				msg->list[ssid_num].ssid_charset	= thiscode;
			}
		
			//如果使用加密的话 判断密码设置是否被更改
			if(isusepass){
				//加密方式是否为6
				if(msg->list[ssid_num].security_mode != 6){	//使用加密
					ischange = 1;
#if WIRELESS_DEBUG
					xyprintf(0," (΄ ◞ิ౪ ◟ิ ‵) Wireless has change in -- %s %s %d", __FILE__, __func__, __LINE__);
#endif
					msg->list[ssid_num].security_mode = 6;
				}
				//密码设置是否相同
				if( strcmp( msg->list[ssid_num].key.wpa.key, thispassword ) || 
					msg->list[ssid_num].key.wpa.rekeyinterval != 3600 ||
					msg->list[ssid_num].key.wpa.encrypt_type != 0 ){
						ischange = 1;
#if WIRELESS_DEBUG
						xyprintf(0," (΄ ◞ิ౪ ◟ิ ‵) Wireless has change in -- %s %s %d", __FILE__, __func__, __LINE__);
#endif
						strcpy( msg->list[ssid_num].key.wpa.key, thispassword );	//密码
						msg->list[ssid_num].key.wpa.rekeyinterval	= 3600;			//密钥更新间隔(网络字节序)
						msg->list[ssid_num].key.wpa.encrypt_type	= 0;			//1:TKIP,2:AES,0:TKIP/AES

				}
			}
			else {
				//没有使用加密
				if(msg->list[ssid_num].security_mode != 0){
					ischange = 1;
#if WIRELESS_DEBUG
					xyprintf(0," (΄ ◞ิ౪ ◟ิ ‵) Wireless has change in -- %s %s %d", __FILE__, __func__, __LINE__);
#endif
					msg->list[ssid_num].security_mode = 0;
				}
			}
		}
		ssid_num++;
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果
	}
	
	for(; ssid_num < 5; ssid_num++){
		if(msg->list[ssid_num].ssid[0] != 0){
			ischange = 1;
#if WIRELESS_DEBUG
			xyprintf(0," (΄ ◞ิ౪ ◟ิ ‵) Wireless has change in -- %s %s %d", __FILE__, __func__, __LINE__);
#endif
			memset(&msg->list[ssid_num], 0, sizeof(struct wifi_base_list_st));
		}
	}

	return ischange;
}

/**
 *@brief  设备流量存储
 *@param  handle		类型 wt_sql_handle*			数据库操作handle及其他资源集合
 *@param  msg			类型 third_speed_st*		流量报文 报文体
 *@param  shebei_pkid	类型 __u32					设备的 mx_shebei 主键id
 *@param  shanghu_id    类型 __u32					设备的商户id
 *@return success 0,failed -1
 */
int wt_sql_speed_insert(wt_sql_handle *handle, third_speed_st *msg, __u32 shebei_pkid, __u32 shanghu_id)
{
	sprintf(handle->sql_str, "INSERT INTO mx_shebei_datasize_log (shanghuid, rid, upsize, downsize, createdate) \
			VALUES(%u, %u, %u, %u, GETDATE())", shanghu_id, shebei_pkid, msg->speed_up, msg->speed_down);
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
	return 0;
}



































/** 
 *@brief  读取升级地址
 *@return succ 0 failed -1
 */
int wt_sql_get_upurl(wt_sql_handle* handle)
{
	//读取设备升级地址
	int id_temp, ishavewifi_temp, ishaveacserver_temp;
	char version_temp[32], codeurl_temp[128];
	SQLBindCol(handle->sqlstr_handle, 1,	SQL_C_ULONG,	&id_temp,				20,		&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 2,	SQL_C_CHAR,		version_temp,			32,		&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 3,	SQL_C_CHAR,		codeurl_temp,			128,	&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 4,	SQL_C_ULONG,	&ishavewifi_temp,		20,		&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 5,	SQL_C_ULONG,	&ishaveacserver_temp,	20,		&handle->sql_err);

	//3.3执行SQL语句
	sprintf(handle->sql_str, "SELECT id, drivecode, driveurl, ishavewifi, ishaveacserver from mx_shebei_type");
	if (wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}

	pthread_mutex_lock(&gv_up_addr_lock);
	free(sgv_up_addr);
	sgv_up_addr = NULL;
	gv_router_type_num = 0;

	handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
	while(handle->sql_ret != SQL_NO_DATA){
		if(!sgv_up_addr){
			sgv_up_addr = malloc(0);
		}
		gv_router_type_num++;
		sgv_up_addr = realloc(sgv_up_addr, gv_router_type_num * sizeof(upgrade_addr));
		sgv_up_addr[gv_router_type_num - 1].id				= id_temp;
		sgv_up_addr[gv_router_type_num - 1].ishavewifi		= ishavewifi_temp;
		sgv_up_addr[gv_router_type_num - 1].ishaveacserver	= ishaveacserver_temp;
		memcpy(sgv_up_addr[gv_router_type_num - 1].version, version_temp, 32);
		memcpy(sgv_up_addr[gv_router_type_num - 1].codeurl, codeurl_temp, 128);
		handle->sql_ret=SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
	}
	xyprintf(0, "** O(∩ _∩ )O ~~ Get upurl of datebase is success!");
	int i = 0;
	for(; i < gv_router_type_num; i++){
		xyprintf(0, "Router' upgrafde addr: id = %d\n\t\t\tversion = %s\n\t\t\tcodeurl = %s\n\t\t\tishavewifi = %d\n\t\t\tishaveacserver = %d",
				sgv_up_addr[i].id, sgv_up_addr[i].version, sgv_up_addr[i].codeurl, sgv_up_addr[i].ishavewifi, sgv_up_addr[i].ishaveacserver);
	}
	pthread_mutex_unlock(&gv_up_addr_lock);
	
	return 0;
}

/** 
 *@brief  修改其他全局配置
 *@return succ 0 failed -1
 */
int wt_sql_get_other(wt_sql_handle* handle)
{
	//3.2绑定变量和SQL语句查询结果
	SQLBindCol(handle->sqlstr_handle, 1,	SQL_C_ULONG,	&sgv_tfree_manyou_time,		20,					&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 2,	SQL_C_CHAR,		sgv_trezhengurl,			256,				&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 3,	SQL_C_ULONG,	&sgv_tupdatelist,			20,					&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 4,	SQL_C_ULONG,	&sgv_testmacuserout,		20,					&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 5,	SQL_C_CHAR,		sgv_acserverurl,			256,				&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 6,	SQL_C_ULONG,	&sgv_acserverport,			20,					&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 7,	SQL_C_CHAR,		sgv_acacserverurl,			256,				&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 8,	SQL_C_ULONG,	&sgv_acacserverport,		20,					&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 9,	SQL_C_ULONG,	&sgv_tdatasize,				20,					&handle->sql_err);

	//3.3执行SQL语句
	sprintf(handle->sql_str, "SELECT TOP 1 tfreemanyou, trezhengurl, tupdatelist, tsetmacuserout, \
			acserverurl, acserverport, acacserverurl, acacserverport, tdatasize FROM mx_rconfig");
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}

	handle->sql_ret=SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
	
	if(handle->sql_ret == SQL_NO_DATA){
		xyprintf(0, "DATA_ERROR:%s %d -- Can not has config in datebase!", __FILE__, __LINE__);
		return -1;
	}
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	if(sgv_tdatasize == 0){
		sgv_tdatasize = 10;
	}

	xyprintf(0, "** O(∩ _∩ )O ~~ Get other config of datebase is success!");
	xyprintf(0, "tfree_manyou_time = %u\n\
			trezhengurl = %s\n\
			tupdatelist = %u\n\
			testmacuserout = %u\n\
			tdatasize = %u\n",
			sgv_tfree_manyou_time, sgv_trezhengurl, sgv_tupdatelist,
			sgv_testmacuserout, sgv_tdatasize);
	xyprintf(0, "acserverurl = %s\n\
			acserverport = %u\n\
			acacserverurl = %s\n\
			acacserverport = %u\n",
			sgv_acserverurl, sgv_acserverport,
			sgv_acacserverurl, sgv_acacserverport);

	SQLBindCol(handle->sqlstr_handle, 1,	SQL_C_CHAR,		&sgv_monitor_servurl,		64,					&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 2,	SQL_C_ULONG,	&sgv_monitor_servport,		20,				&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 3,	SQL_C_ULONG,	&sgv_monitor_time,			20,					&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 4,	SQL_C_ULONG,	&sgv_monitor_timeout,		20,					&handle->sql_err);

	//3.3执行SQL语句
	sprintf(handle->sql_str, "SELECT TOP 1 servurl, servport, freetime, outtime FROM mx_tz_config");
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}

	handle->sql_ret=SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
	
	if(handle->sql_ret == SQL_NO_DATA){
		xyprintf(0, "DATA_ERROR:%s %d -- Can not has config in datebase!", __FILE__, __LINE__);
		return -1;
	}
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	xyprintf(0, "** O(∩ _∩ )O ~~ Get other config of datebase is success!");
	xyprintf(0, "monitor_servurl = %s\n\
			monitor_servport = %u\n\
			monitor_time = %u\n\
			monitor_timeout = %u\n",
			sgv_monitor_servurl, sgv_monitor_servport, sgv_monitor_time,
			sgv_monitor_timeout);
	return 0;
}

/** 
 *@brief  修改富媒体内容
 *@return succ 0 failed -1
 */
int wt_sql_get_gg(wt_sql_handle* handle)
{
	//获取数据库内的配置内容
	SQLBindCol(handle->sqlstr_handle, 1,	SQL_C_CHAR,		sgv_rm_script1,				DNS_WHITE_URL_SIZE, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 2,	SQL_C_CHAR,		sgv_rm_script2,				DNS_WHITE_URL_SIZE, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 3,	SQL_C_CHAR,		sgv_rm_fudomain,			DNS_WHITE_URL_SIZE, &handle->sql_err);

	//3.3执行SQL语句
	sprintf(handle->sql_str, "SELECT TOP 1 script1, script2, fudomain FROM mx_rconfig");
	if (wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}

	handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
	
	if(handle->sql_ret == SQL_NO_DATA){
		xyprintf(0, "DATA_ERROR:%s %d -- Can not has config in datebase!", __FILE__, __LINE__);
		return -1;
	}
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
	
	xyprintf(0, "** O(∩ _∩ )O ~~ Get gg of datebase is success!");
	xyprintf(0, "script1 = %s", sgv_rm_script1);
	xyprintf(0, "script2 = %s", sgv_rm_script2);
	xyprintf(0, "fudomain = %s\n", sgv_rm_fudomain);
	return 0;
}

/**
 *@brief  修改白名单内容
 *@return succ 0 failed -1
 */
int wt_sql_get_white(wt_sql_handle* handle)
{
	SQLBindCol(handle->sqlstr_handle, 1,	SQL_C_CHAR,		sgv_white_dns_url,			DNS_WHITE_URL_SIZE, &handle->sql_err);
#ifdef ADVERTISING

	//3.3执行SQL语句
	sprintf(handle->sql_str, "SELECT TOP 1 openurl FROM mx_rconfig");
	if (wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
#else
	SQLBindCol(handle->sqlstr_handle, 2,	SQL_C_CHAR,		sgv_white_dns_of_ios_url,	DNS_WHITE_URL_SIZE, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 3,	SQL_C_CHAR,		sgv_white_dns_of_qq_url,	DNS_WHITE_URL_SIZE, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 4,	SQL_C_CHAR,		sgv_white_dns_of_sina_url,	DNS_WHITE_URL_SIZE, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 5,	SQL_C_CHAR,		sgv_white_dns_of_weixin_url,DNS_WHITE_URL_SIZE, &handle->sql_err);

	//3.3执行SQL语句
	sprintf(handle->sql_str, "SELECT TOP 1 openurl, openurlios, openurlforqq, openurlforsina, openurlforweixin FROM mx_rconfig");
	if (wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
#endif

	handle->sql_ret=SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
	
	if(handle->sql_ret == SQL_NO_DATA){
		xyprintf(0, "DATA_ERROR:%s %d -- Can not has config in datebase!", __FILE__, __LINE__);
		return -1;
	}
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
	
	xyprintf(0, "** O(∩ _∩ )O ~~ Get white of datebase is success!");
	xyprintf(0, "white_dns_url = %s", sgv_white_dns_url);
#ifdef ADVERTISING
#else
	xyprintf(0, "white_dns_of_qq_url = %s", sgv_white_dns_of_qq_url);
	xyprintf(0, "white_dns_of_sina_url = %s", sgv_white_dns_of_sina_url);
	xyprintf(0, "white_dns_of_ios_url = %s", sgv_white_dns_of_ios_url);
	xyprintf(0, "white_dns_of_weixin_url = %s\n", sgv_white_dns_of_weixin_url);
#endif

	return 0;
}

/**
 *@brief  修改微信设置内容
 *@return succ 0 failed -1
 */
int wt_sql_get_weixin(wt_sql_handle* handle)
{
#ifdef ADVERTISING
#else
	SQLBindCol(handle->sqlstr_handle, 1,	SQL_C_CHAR,		sgv_weixinhtml1,			1024,				&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 2,	SQL_C_CHAR,		sgv_weixinhtml2,			2048,				&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 3,	SQL_C_CHAR,		sgv_noweixingotourl,		256,				&handle->sql_err);
	//3.3执行SQL语句
	sprintf(handle->sql_str, "SELECT TOP 1 weixinhtml1, weixinhtml2, noweixingotourl FROM mx_rconfig");
	if (wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return -1;
	}
	handle->sql_ret=SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
	
	if(handle->sql_ret == SQL_NO_DATA){
		xyprintf(0, "DATA_ERROR:%s %d -- Can not has config in datebase!", __FILE__, __LINE__);
		return -1;
	}
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
	
	xyprintf(0, "** O(∩ _∩ )O ~~ Get weixin of datebase is success!");
	xyprintf(0, "weixinhtml1 = %s", sgv_weixinhtml1);
	xyprintf(0, "weixinhtml2 = %s", sgv_weixinhtml2);
	xyprintf(0, "noweixingotourl = %s\n", sgv_noweixingotourl);
#endif
	return 0;
}














/** 
 *@brief  修改富媒体内容
 *@return succ 0 failed -1
 */
int wt_sql_get_gg2(wt_sql_handle* handle)
{
	pthread_mutex_lock(&gv_simple_gg2_list_lock);
	
	// 删除链表内的数据
	struct list_head *pos;
	for( pos = sgv_simple_gg2_list_head.next; pos != &sgv_simple_gg2_list_head; pos = pos->next ){
		struct list_head *temp = pos;
		pos = temp->prev;
		list_del( temp );
		if(temp){
			free(temp);
		}
		gv_simple_gg2_list_count--;
		xyprintf(0, "GG2: gg clean, count = %d", gv_simple_gg2_list_count);
	}
	
	gv_simple_gg2_list_count = 0;

	int					location;
	char				categories[128];
	char				script1[ DNS_WHITE_URL_SIZE ];
	char				script2[ DNS_WHITE_URL_SIZE ];
	char				fudomain[ DNS_WHITE_URL_SIZE ];
	
	//获取数据库内的配置内
	SQLBindCol(handle->sqlstr_handle, 1,	SQL_C_ULONG,	&location,	DNS_WHITE_URL_SIZE, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 2,	SQL_C_CHAR,		categories,	128,				&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 3,	SQL_C_CHAR,		script1,	DNS_WHITE_URL_SIZE, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 4,	SQL_C_CHAR,		script2,	DNS_WHITE_URL_SIZE, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 5,	SQL_C_CHAR,		fudomain,	DNS_WHITE_URL_SIZE, &handle->sql_err);

	//3.3执行SQL语句
	sprintf(handle->sql_str, "SELECT replaceflag, replacetags, script1, script2, replacedns FROM mx_sys_fumeiti");
	if (wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		pthread_mutex_unlock(&gv_simple_gg2_list_lock);
		return -1;
	}

	handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
	
	xyprintf(0, "** O(∩ _∩ )O ~~ Get gg of datebase is success!");
	
	while(handle->sql_ret != SQL_NO_DATA){
	
		simple_gg2_node* node = malloc( sizeof(simple_gg2_node) );
		memset(node, 0, sizeof(simple_gg2_node) );
		node->location = location;
		memcpy(node->categories,	categories, 128);
		memcpy(node->script1,		script1,	DNS_WHITE_URL_SIZE);
		memcpy(node->script2,		script2,	DNS_WHITE_URL_SIZE);
		memcpy(node->fudomain,		fudomain,	DNS_WHITE_URL_SIZE);
	
		list_add(&(node->node), &sgv_simple_gg2_list_head);
		gv_simple_gg2_list_count++;

		xyprintf(0, "♨ ♨ ♨ ♨  -- count is %d", gv_simple_gg2_list_count);
		xyprintf(0, "location = %d",	node->location);
		xyprintf(0, "categories = %s",	node->categories);
		xyprintf(0, "script1 = %s",		node->script1);
		xyprintf(0, "script2 = %s",		node->script2);
		xyprintf(0, "fudomain = %s\n",	node->fudomain);
		
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
	}
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
	
	xyprintf(0, "gv_simple_gg2_list_count = %d\n\n", gv_simple_gg2_list_count);

	pthread_mutex_unlock(&gv_simple_gg2_list_lock);

	return 0;
}
