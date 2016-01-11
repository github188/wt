/*****************************************************
 *
 * 路由器设置包装报文
 *
 *****************************************************/

#include "auth_header.h"

/** 
 *@brief  富媒体发送函数
 *@param  sockfd	类型 int			要发送到设备的socket套接字
 *@param  shanghuid 类型 __u32			设备对应商户id
 *@param  msg_head  类型 msg_head_st*	已填充设备基本信息的msg_head_st指针
 *@return succ 0 failed -1
 */
int send_gg(int sockfd, __u32 shanghuid, msg_head_st *msg_head)
{

	if( strlen(sgv_rm_script1) != 0 && strlen(sgv_rm_script2) != 0 && strlen(sgv_rm_fudomain) != 0 ){
		char	c_shanghuid[16] = { 0 };
		sprintf( c_shanghuid, "%u", shanghuid );
		int		js_len			= strlen(sgv_rm_script1) + strlen(c_shanghuid) + strlen(sgv_rm_script2);
		int		dns_len			= strlen(sgv_rm_fudomain);
		void* gg_msg			= malloc( sizeof(msg_head_st) + sizeof(simple_gg_msg_st) + js_len + dns_len + 1 );
		memset(gg_msg, 0, sizeof(msg_head_st) + sizeof(simple_gg_msg_st) + js_len + dns_len + 1 );
		msg_head->mode			= 2;
		msg_head->order			= msg_order_simple_gg;
		simple_gg_msg_st *gg	= (simple_gg_msg_st*)( gg_msg + sizeof(msg_head_st) );
		gg->en					= 1;
		gg->js_len				= js_len;
		gg->dns_cnt				= dns_len;
		gg->data_len			= js_len + dns_len;
		msg_head->len			= sizeof(msg_head_st) + sizeof(simple_gg_msg_st) + gg->data_len;
		memcpy(gg_msg, msg_head, sizeof(msg_head_st) );
		
		sprintf( (char*)gg + sizeof(simple_gg_msg_st), "%s%s%s%s", sgv_rm_script1, c_shanghuid, sgv_rm_script2, sgv_rm_fudomain);

		//xyprintf_simple_gg_head(gg);
		//发送包体
		if( send_simple_gg_msg_head(sockfd, gg_msg) ){
			xyprintf(0, "SOCK_ERROR:%d %s %d -- Send simple gg message to router is error!", sockfd, __FILE__, __LINE__);
			free( gg_msg );
			return -1;
		}
		xyprintf(0, "%u: --> --> Send simple_gg_msg success --> -->", msg_head->router_id);
		free(gg_msg);
	}
	return 0;
}

#define WHITE_DEBUG		0

/** 
 *@brief  白名单下发函数
 *@param  sockfd	类型 int			要发送到设备的socket套接字
 *@param  hasweixin 类型 __u8			对应商户是否开启微信认证
 *@param  hasqq		类型 __u8			对应商户是否开启qq认证
 *@param  hasweibo	类型 __u8			对应商户是否开启微博认证
 *@param  msg_head  类型 msg_head_st*	已填充设备基本信息的msg_head_st指针
 *@param  gotodomain类型 char*			商户单独的白名单内容
 *@return succ 0 failed -1
 */
int send_white(int sockfd, __u8 hasios, __u8 hasweixin, __u8 hasqq, __u8 hasweibo, msg_head_st *msg_head, char* gotodomain, char* otherdm){
#if WHITE_DEBUG
	xyprintf(0, "hasios = %u\n\
			hasweixin = %u\n\
			hasqq = %u\n\
			hasweibo = %u",
			hasios,
			hasweixin,
			hasqq,
			hasweibo);
#endif
	// 各个白名单的长度
	int		white_dns_size				= strlen(sgv_white_dns_url);
	int		gotodomain_size				= strlen(gotodomain);
	int		ios_size					= strlen(sgv_white_dns_of_ios_url);
	int		weixin_size					= strlen(sgv_white_dns_of_weixin_url);
	int		qq_size						= strlen(sgv_white_dns_of_qq_url);
	int		weibo_size					= strlen(sgv_white_dns_of_sina_url);
	int		other_size					= strlen(otherdm);
#if WHITE_DEBUG
	xyprintf(0,"white_dns_size = %d\n\
			gotodomain_size = %d\n\
			ios_size = %d\n\
			weixin_size = %d\n\
			qq_size = %d\n\
			weibo_size = %d\n\
			other_size = %d",
			white_dns_size,
			gotodomain_size,
			ios_size,
			weixin_size,
			qq_size,
			weibo_size,
			other_size);
#endif
	int url_len = 0;		// 白名单的长度
	int url_count = 0;		// 白名单种类的数量

	// 普通白名单
	if(white_dns_size){
		url_len += white_dns_size;
		url_count++;
	}
	// 商户单独白名单
	if(gotodomain_size){
		url_len += gotodomain_size;
		url_count++;
	}
	// ios白名单
	if(hasios && ios_size){
		url_len += ios_size;
		url_count++;
	}
	// 微信白名单
	if(hasweixin && weixin_size){
		url_len += weixin_size;
		url_count++;
	}
	// qq白名单
	if(hasqq && qq_size){
		url_len += qq_size;
		url_count++;
	}
	// 微博白名单
	if(hasweibo && weibo_size){
		url_len += weibo_size;
		url_count++;
	}
	//其他白名单
	if(other_size){
		url_len += other_size;
		url_count++;
	}

	if(url_count > 1){
		url_len = url_len + url_count - 1;	// 白名单长度加上白名单种类减2, 白名单之间要加";"
	}

#if WHITE_DEBUG
	xyprintf(0, "url_len = %d", url_len);
#endif

	// 如果所有长度加起来 不是0
	if( url_len ){
		int size = sizeof(msg_head_st) + sizeof(white_dns_msg_st) + url_len;// 报文长度
		void* msg = malloc(size + 1);	// 多申请点空间 防止打印字符串时 出错
		memset(msg, 0, size + 1);		// 清空
		white_dns_msg_st *dns_msg = msg + sizeof(msg_head_st);	// 白名单报文的位置
		char* white_addr = (char*)(dns_msg->url_data);			// 白名单的位置

#if WHITE_DEBUG
		xyprintf(0, "white_addr = %p", white_addr);
#endif

		if( white_dns_size ){			//是否存在全局白名单
			sprintf(white_addr, "%s", sgv_white_dns_url );
			white_addr += white_dns_size;	//地址计数
#if WHITE_DEBUG
			xyprintf(0, "white_dns_size -- white_addr = %p", white_addr);
#endif
		}

		//是否开启ios认证
		if( hasios && ios_size ){
			//如果之前已存在白名单
			if( white_addr != (char*)(dns_msg->url_data) ){
				*white_addr = ';';
				white_addr ++;
#if WHITE_DEBUG
				xyprintf(0, "hasios 1 -- white_addr = %p", white_addr);
#endif
			}
			sprintf( white_addr, "%s", sgv_white_dns_of_ios_url );
			white_addr += ios_size;
#if WHITE_DEBUG
			xyprintf(0, "hasios 2 -- white_addr = %p", white_addr);
#endif
		}

		//是否开启微信认证
		if( hasweixin && weixin_size ){
			//如果之前已存在白名单
			if( white_addr != (char*)(dns_msg->url_data) ){
				*white_addr = ';';
				white_addr ++;
#if WHITE_DEBUG
				xyprintf(0, "hasweixin 1 -- white_addr = %p", white_addr);
#endif
			}
			sprintf( white_addr, "%s", sgv_white_dns_of_weixin_url );
			white_addr += weixin_size;
#if WHITE_DEBUG
			xyprintf(0, "hasweixin 2 -- white_addr = %p", white_addr);
#endif
		}

		//是否开启qq认证
		if( hasqq && qq_size ){
			if( white_addr != (char*)(dns_msg->url_data) ){
				*white_addr = ';';
				white_addr ++;
#if WHITE_DEBUG
				xyprintf(0, "hasqq 1 -- white_addr = %p", white_addr);
#endif
			}
			sprintf( white_addr, "%s", sgv_white_dns_of_qq_url );
			white_addr += qq_size;
#if WHITE_DEBUG
			xyprintf(0, "hasqq 2 -- white_addr = %p", white_addr);
#endif
		}

		//是否开启微博认证
		if( hasweibo && weibo_size ){
			if( white_addr != (char*)(dns_msg->url_data) ){
				*white_addr = ';';
				white_addr ++;
#if WHITE_DEBUG
				xyprintf(0, "hasweibo 1 -- white_addr = %p", white_addr);
#endif
			}
			sprintf( white_addr, "%s", sgv_white_dns_of_sina_url );
			white_addr += weibo_size;
#if WHITE_DEBUG
			xyprintf(0, "hasweibo 2 -- white_addr = %p", white_addr);
#endif
		}
		
		//是否有单独设置的白名单
		if( gotodomain_size ){
			if( white_addr != (char*)(dns_msg->url_data) ){
				*white_addr = ';';
				white_addr ++;
#if WHITE_DEBUG
				xyprintf(0, "gotodomain 1 -- white_addr = %p", white_addr);
#endif
			}
			sprintf( white_addr, "%s", gotodomain );
			white_addr += gotodomain_size;
#if WHITE_DEBUG
			xyprintf(0, "gotodomain 2 -- white_addr = %p", white_addr);
#endif
		}
		
		//是否有其他的白名单
		if( other_size ){
			if( white_addr != (char*)(dns_msg->url_data) ){
				*white_addr = ';';
				white_addr ++;
#if WHITE_DEBUG
				xyprintf(0, "other 1 -- white_addr = %p", white_addr);
#endif
			}
			sprintf( white_addr, "%s", otherdm );
#if WHITE_DEBUG
			xyprintf(0, "other 2 -- white_addr = %p", white_addr);
#endif
		}
		
		// 报文 -- 白名单长度
		dns_msg->url_len	= url_len;
		
		// 报文头
		msg_head->mode		= 2;
		msg_head->order		= msg_order_dns_white;
		msg_head->len		= size;
		memcpy(msg, msg_head, sizeof(msg_head_st));

#if WHITE_DEBUG
		//打印一下
		xyprintf_dns_msg_head(dns_msg);
#endif

		//发送包体
		if( send_dns_msg_head(sockfd, msg) ){
			xyprintf(0, "SOCK_ERROR:%d %s %d -- Send dns message to router is error!", sockfd, __FILE__, __LINE__);
			free(msg);
			return -1;
		}
		xyprintf(0, "%u: --> --> Send dns_msg success --> -->", msg_head->router_id);
		free(msg);
	}//end if
	return 0;
}

/** 
 *@brief  发送获取版本号命令函数
 *@param  sockfd	类型 int			要发送到设备的socket套接字
 *@param  msg_head  类型 msg_head_st*	已填充设备基本信息的msg_head_st指针
 *@return succ 0 failed -1
 */
int send_version_get(int sockfd, msg_head_st *msg_head)
{
	msg_head->mode	= 2;
	msg_head->order	= msg_order_version_get;
	msg_head->len	= sizeof(msg_head_st);

	if( send_version_get_msg(sockfd, msg_head) ){
		return -1;
	}

	xyprintf(0, "%u: --> --> Send version get success --> -->", msg_head->router_id);
	return 0;
}

/** 
 *@brief  发送获取设备无线设置命令函数
 *@param  sockfd	类型 int			要发送到设备的socket套接字
 *@param  msg_head  类型 msg_head_st*	已填充设备基本信息的msg_head_st指针
 *@return succ 0 failed -1
 */
int send_wireless_config_get(int sockfd, msg_head_st *msg_head, int order)
{
	msg_head->mode	= 2;
	msg_head->order	= order;
	msg_head->len	= sizeof(msg_head_st);

	if( send_wireless_config_get_msg(sockfd, msg_head) ){
		xyprintf(0, "SOCK_ERROR:%d %s %d -- Send config get message is %u to router is error!", sockfd, __FILE__, __LINE__, order);
		return -1;
	}
	xyprintf(0, "%u: --> --> Send wireless config get is %u success --> -->", msg_head->router_id, order);
	return 0;
}

/** 
 *@brief  发送升级命令函数
 *@param  sockfd	类型 int			要发送到设备的socket套接字
 *@param  msg_head  类型 msg_head_st*	已填充设备基本信息的msg_head_st指针
 *@param  xinghaoid 类型 int			设备在数据库中存储的型号id
 *@return succ 0 failed -1
 */
int send_auto_upgrade(int sockfd, msg_head_st *msg_head, int xinghaoid, char* codeurl)
{
	// 升级地址的长度
	int buf_size = strlen(codeurl);

	//申请报文空间
	void *msg = malloc(sizeof(msg_head_st) + sizeof(auto_upgrade_msg_st) + buf_size + 1);
	memset(msg, 0, sizeof(msg_head_st) + sizeof(auto_upgrade_msg_st) + buf_size + 1);
	
	// 报文赋值
	auto_upgrade_msg_st *au_msg = msg + sizeof(msg_head_st);
	au_msg->url_len = buf_size;
	memcpy(&au_msg->url_data, codeurl, buf_size);
	
	// 报文头值修改
	msg_head->mode	= 2;
	msg_head->order	= msg_order_auto_upgrade;
	msg_head->len	= sizeof(msg_head_st) + sizeof(auto_upgrade_msg_st) + buf_size;

	// 拷贝报文头到报文中
	memcpy(msg, msg_head, sizeof(msg_head_st));

	xyprintf_auto_upgrade(au_msg);

	//发送报文
	if( send_auto_upgrade_msg(sockfd, msg) ){
		xyprintf(0, "SOCK_ERROR:%d %s %d -- Send auto upgrade message to router is error!", sockfd, __FILE__, __LINE__);
		free(msg);
		return -1;
	}
	
	xyprintf(0, "%u: --> --> Send auto upgrade success --> -->", msg_head->router_id);
	free(msg);
	return 0;
}

/** 
 *@brief  简单缓存下发函数
 *@param  sockfd	类型 int			要发送到设备的socket套接字
 *@param  msg_head  类型 msg_head_st*	已填充设备基本信息的msg_head_st指针
 *@param  cache		类型 char*			缓存内容
 *@return succ 0 failed -1
 */
/*
int send_simple_cache(int sockfd, msg_head_st* msg_head, char *cache)
{
	char* cache_temp = "http://www.baidu.com<http://www.google.com/<http://upload.wikimedia.org/wikipedia/commons/thumb/b/b5/DengXiaoping.jpg/800px-DengXiaoping.jpg";
	int cache_len = strlen(cache_temp);
	int size = sizeof(msg_head_st) + sizeof(simple_cache_msg_st) + cache_len;
	char* buf = malloc(size);
	msg_head_st* head = (msg_head_st*)buf;
	simple_cache_msg_st* cache_msg = (simple_cache_msg_st*)( buf + sizeof(msg_head_st) );

	//包头
	memcpy(head, msg_head, sizeof(msg_head_st));
	head->order = msg_order_simple_cache;
	head->mode	= 2;
	head->len	= size;

	cache_msg->url_len	= cache_len;
	memcpy(cache_msg->url_data, cache_temp, cache_len);

	xyprintf_simple_cache_msg_st(cache_msg);

	//发送
	if( send_simple_cache_msg(sockfd, buf) ){
		free(buf);
		return -1;
	}
	xyprintf(0, "%u:--> --> Send simple cache to router success --> -->", msg_head->router_id);
	
	free(buf);
	return 0;
}
*/

/** 
 *@brief  USB或内存缓存设置函数
 *@param  sockfd	类型 int			要发送到设备的socket套接字
 *@param  msg_head  类型 msg_head_st*	已填充设备基本信息的msg_head_st指针
 *@param  order		类型 int			要获取的缓存配置类型，msg_order_usb_cache msg_order_memory_cache_set
 *@param  opt		类型 __u16			操作类型 增删查改等
 *@param  name		类型 char*			描述符
 *@param  len		类型 __u16			要缓存的url地址的长度
 *@param  url		类型 char*			要缓存的url地址
 *@return succ 0 failed -1
 */
int send_usb_simple_cache(int sockfd, msg_head_st* msg_head, int order, __u16 opt, char* name, __u16 len, char* url)
{
	int size = sizeof(msg_head_st) + sizeof(usb_simple_cache_st) + len + 1;
	char* buf = malloc(size);
	msg_head_st* head = (msg_head_st*)buf;
	usb_simple_cache_st* cache_msg = (usb_simple_cache_st*)( buf + sizeof(msg_head_st) );

	//包头
	memcpy(head, msg_head, sizeof(msg_head_st));
	head->order = order;
	head->mode	= 2;
	head->len	= size - 1;

	cache_msg->opt	= opt;
	if(name){
		snprintf(cache_msg->name, sizeof(cache_msg->name), "%s", name);
	}
	cache_msg->len	= len;
	if(len && url){
		snprintf(cache_msg->url, len + 1, "%s", url);
	}

	//xyprintf_usb_simple_cache_st(cache_msg);

	//发送
	if( send_usb_simple_cache_msg(sockfd, buf) ){
		free(buf);
		return -1;
	}

	if(order == msg_order_memory_cache_set){
		xyprintf(0, "%u:--> --> Send mem cache msg to router success --> -->", msg_head->router_id);
	}
	else {
		xyprintf(0, "%u:--> --> Send usb cache msg to router success --> -->", msg_head->router_id);
	}
	
	free(buf);
	return 0;
}

/**
 *@brief  设备usb缓存修改
 *@return succ 0 failed -1
 */
int send_all_usb_simple_cache(wt_sql_handle* handle, sock_list *sock, int order)
{
			
	msg_head_st *msg_head = malloc(sizeof(msg_head_st));
	memset(msg_head, 0, sizeof(msg_head_st));
			
	msg_head->ver			= sock->ver;			// 版本
	msg_head->sec_flag		= sock->sec_flag;		// 加密方式
	msg_head->router_id		= sock->router_id;		// 设备id
	memcpy(msg_head->hard_seq, sock->hard_seq, 64);	// 序列号
	
	int cobj = 0;
	if( order == msg_order_memory_cache_set ){
		cobj = 0;
	}
	else {
		cobj = 1;
	}

	// 清空当前缓存 清空后下发修改后的缓存
	if( send_usb_simple_cache( sock->sockfd, msg_head, order, 3, NULL, 0, NULL ) ){
		xyprintf(0, "SOCK_ERROR:%d %s %d -- Send memory cache del_all to router is error!", sock->sockfd, __FILE__, __LINE__);
		socklist_sockfd_err(sock->sockfd);
	}
	else {
/*		//用shanghuid去查询缓存详情
		__u32 uid = 0;
		char url[512] = { 0 };
		char sid[32] = { 0 };
		SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG,	&uid,		512, &handle->sql_err);	//设备主键ID
		SQLBindCol(handle->sqlstr_handle, 2, SQL_C_CHAR,	url,		512, &handle->sql_err);	//设备主键ID
		sprintf(handle->sql_str, "SELECT id, curl from mx_shanghu_cache_list WHERE shanghuid = %u and isok= 1 and cobj= %d \
			and id in ( SELECT min(id) FROM mx_shanghu_cache_list WHERE shanghuid = %u and isok= 1 and cobj= %d group by curl )",
			sock->shanghuid, cobj, sock->shanghuid, cobj);
		if( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			if(msg_head){
				free(msg_head);
			}
			return -1;
		}
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条	
		
		while( handle->sql_ret != SQL_NO_DATA){
			snprintf(sid, sizeof(sid), "%u", uid);
			if( send_usb_simple_cache(sock->sockfd, msg_head, order, 0, sid, strlen(url), url) ){
				xyprintf(0, "SOCK_ERROR:%d %s %d -- Send memory cache add to router is error!", sock->sockfd, __FILE__, __LINE__);
				socklist_sockfd_err(sock->sockfd);
				break;
			}
			handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条	
		}
				
		SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
*/
	} // end if
	
	if(msg_head){
		free(msg_head);
	}
	return 0;
}

/**
 *@brief  设备关键字过滤修改
 *@return succ 0 failed -1
 */
int send_all_web_keyword(wt_sql_handle* handle, sock_list *sock)
{
	msg_head_st *msg_head = malloc(sizeof(msg_head_st));
	memset(msg_head, 0, sizeof(msg_head_st));
			
	msg_head->ver			= sock->ver;			// 版本
	msg_head->sec_flag		= sock->sec_flag;		// 加密方式
	msg_head->router_id		= sock->router_id;		// 设备id
	memcpy(msg_head->hard_seq, sock->hard_seq, 64);	// 序列号
	
	if( send_web_keyword(sock->sockfd, msg_head, "del", "name", "name") ){
		xyprintf(0, "SOCK_ERROR:%d %s %d -- Send web keyword to router is error!", sock->sockfd, __FILE__, __LINE__);
		socklist_sockfd_err(sock->sockfd);
	}
	else {
		//用shanghuid去查询关键词列表
		char keyword[256];
		SQLBindCol(handle->sqlstr_handle, 1, SQL_C_CHAR,	keyword,		256, &handle->sql_err);	//设备主键ID
		sprintf(handle->sql_str, "SELECT keyword FROM mx_shanghu_webkeyword WHERE shanghuid = %u AND isok = 1", sock->shanghuid);
		if( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			if(msg_head){
				free(msg_head);
			}
			return -1;
		}
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条	
		
		while( handle->sql_ret != SQL_NO_DATA){
			if( send_web_keyword(sock->sockfd, msg_head, "add", "name", keyword) ){
				xyprintf(0, "SOCK_ERROR:%d %s %d -- Send web keyword to router is error!", sock->sockfd, __FILE__, __LINE__);
				socklist_sockfd_err(sock->sockfd);
				break;
			}
			handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条	
		}
				
		SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
	} // end if
	
	if(msg_head){
		free(msg_head);
	}
	return 0;
}

/** 
 *@brief  行为管理 关键词过滤 设置函数 测试
 *@param  sockfd	类型 int			要发送到设备的socket套接字
 *@param  msg_head  类型 msg_head_st*	已填充设备基本信息的msg_head_st指针
 *@param  opt		类型 char*			要执行的操作 增删查改	
 *@param  name		类型 char*			规则描述
 *@param  keyword	类型 char*			关键字 GB2312编码
 *@return succ 0 failed -1
 */
int send_web_keyword(int sockfd, msg_head_st* msg_head, char* opt, char* name, char* keyword)
{
	int size = sizeof(msg_head_st) + sizeof(web_keyword_st) + 1;
	char* buf = malloc(size);
	memset(buf, 0, size);
	msg_head_st* head = (msg_head_st*)buf;
	web_keyword_st* msg = (web_keyword_st*)( buf + sizeof(msg_head_st) );

	//包头
	memcpy(head, msg_head, sizeof(msg_head_st));
	head->order = msg_order_web_keyword_set;
	head->mode	= 2;
	head->len	= size - 1;

	snprintf(msg->opt, sizeof(msg->opt), "%s", opt);
	sprintf(msg->def, "1");
	sprintf(msg->log, "0");
	sprintf(msg->block, "1");
	sprintf(msg->en, "1");
	snprintf(msg->name, sizeof(msg->name), "%s", name);

	
	unsigned char gb2312[32] = { 0 };
	unsigned char utf8[64] = { 0 };
	memcpy(gb2312, keyword, 24);
	
	if( code_convert("GB2312", "UTF-8", gb2312, strlen(gb2312), utf8, 64) ){
		xyprintf(0, "DATA_ERROR:%s %d -- code_convert() has error!", __FILE__, __LINE__);
		return -1;
	}

	//printf("%2x %2x %2x %2x %2x %2x\n", utf8[0], utf8[1], utf8[2], utf8[3], utf8[4], utf8[5]);
	//printf("%2x %2x %2x %2x %2x %2x\n", gb2312[0], gb2312[1], gb2312[2], gb2312[3], gb2312[4], gb2312[5]);


	int i;
	for(i = 0; i < 21; i++){
		sprintf(&msg->keyword_gb2312[i * 3], "%%%2X", gb2312[i]);
		if(gb2312[i + 1] == 0){
			break;
		}
	}
	for(i = 0; i < 21; i++){
		sprintf(&msg->keyword_utf8[i * 3], "%%%2X", utf8[i]);
		if(utf8[i + 1] == 0){
			break;
		}
	}
	
	//xyprintf_web_keyword_st(msg);

	//发送
	if( send_web_keyword_msg(sockfd, buf) ){
		free(buf);
		return -1;
	}

	xyprintf(0, "%u:--> --> Send web keyword msg to router success --> -->", msg_head->router_id);
	
	free(buf);
	return 0;
}

/** 
 *@brief  微信分享设置
 *@param  sockfd	类型 int			要发送到设备的socket套接字
 *@param  msg_head  类型 msg_head_st*	已填充设备基本信息的msg_head_st指针
 *@param  rid		类型 unsigned int	设备的主键id
 *@return succ 0 failed -1
 */
int send_weixin_share_set(int sockfd, msg_head_st* msg_head, unsigned int rid)
{
	char srid[16] = { 0 };
	snprintf(srid, sizeof(srid), "%u", rid);
	int size = sizeof(msg_head_st) + sizeof(weixin_share_set_st) + strlen(sgv_weixinhtml1)
		+ strlen(sgv_weixinhtml2) + strlen(srid) + 1;
	char* buf = malloc(size);
	memset(buf, 0, size);
	msg_head_st* head = (msg_head_st*)buf;
	weixin_share_set_st* msg = (weixin_share_set_st*)( buf + sizeof(msg_head_st) );

	//包头
	memcpy(head, msg_head, sizeof(msg_head_st));
	head->order = msg_order_weixin_share_set;
	head->mode	= 2;
	head->len	= size - 1;
	
	//xyprintf_msg_head(head);

	msg->enable = 1;
	sprintf(msg->html_data, "%s%s%s", sgv_weixinhtml1, srid, sgv_weixinhtml2);
	msg->html_len = strlen(msg->html_data);
	snprintf(msg->redirt_url, sizeof(msg->redirt_url), "%s", sgv_noweixingotourl);
	
	//xyprintf_weixin_share_set(msg);

	//发送
	if( send_weixin_share_set_msg(sockfd, buf) ){
		free(buf);
		return -1;
	}

	xyprintf(0, "%u:--> --> Send weixin share set msg to router success --> -->", msg_head->router_id);
	
	free(buf);
	return 0;
}

/** 
 *@brief  USB路径文件列表获取
 *@param  sockfd	类型 int			要发送到设备的socket套接字
 *@param  msg_head  类型 msg_head_st*	已填充设备基本信息的msg_head_st指针
 *@param  dir		类型 char*			要获取的路径
 *@return succ 0 failed -1
 */
int send_usb_file_get(int sockfd, msg_head_st* msg_head, char* dir)
{
	int size = sizeof(msg_head_st) + sizeof(usb_file_get_st) + 1;
	char* buf = malloc(size);
	memset(buf, 0, size);
	msg_head_st* head = (msg_head_st*)buf;
	usb_file_get_st* msg = (usb_file_get_st*)( buf + sizeof(msg_head_st) );

	//包头
	memcpy(head, msg_head, sizeof(msg_head_st));
	head->order = msg_order_usb_file_get;
	head->mode	= 2;
	head->len	= size - 1;

	snprintf(msg->dir, sizeof(msg->dir), "%s", dir);
	
	//xyprintf_usb_file_get(msg);

	//发送
	if( send_usb_file_get_msg(sockfd, buf) ){
		free(buf);
		return -1;
	}

	xyprintf(0, "%u:--> --> Send usb file get (%s) msg to router success --> -->", msg_head->router_id, dir);
	
	free(buf);
	return 0;
}

/** 
 *@brief  发送获取路由器实时流量获取函数
 *@param  sockfd	类型 int			要发送到设备的socket套接字
 *@param  msg_head  类型 msg_head_st*	已填充设备基本信息的msg_head_st指针
 *@return succ 0 failed -1
 */
int send_speed_get(int sockfd, msg_head_st *msg_head)
{
	msg_head->mode	= 2;
	msg_head->order	= msg_order_speed_get;
	msg_head->len	= sizeof(msg_head_st);

	if( send_head_get_msg(sockfd, msg_head) ){
		return -1;
	}

	xyprintf(0, "%u: --> --> Send speed get success --> -->", msg_head->router_id);
	return 0;
}

/** 
 *@brief  发送简单广告函数
 *@param  sockfd	类型 int			要发送到设备的socket套接字
 *@param  msg_head  类型 msg_head_st*	已填充设备基本信息的msg_head_st指针
 *@return succ 0 failed -1
 */
int send_gg2(int sockfd, unsigned int shanghuid, msg_head_st *msg_head)
{
	pthread_mutex_lock(&gv_simple_gg2_list_lock);
	unsigned int max_size = sizeof(msg_head_st) + sizeof(simple_gg2_head_st) + gv_simple_gg2_list_count * 5120;
	pthread_mutex_unlock(&gv_simple_gg2_list_lock);
	
	char* buf = malloc(max_size);
	memset(buf, 0, max_size);
	msg_head_st* head = (msg_head_st*)buf;
	simple_gg2_head_st* msg = (simple_gg2_head_st*)( buf + sizeof(msg_head_st) );

	//包头
	memcpy(head, msg_head, sizeof(msg_head_st));
	head->order = msg_order_simple_gg2_set;
	head->mode	= 2;
	head->len	= sizeof(msg_head_st) + sizeof(simple_gg2_head_st);

	msg->en = 1;
	msg->cnt = 0;
	msg->data_len = 0;

	simple_gg2_st *sg2 = (simple_gg2_st*)( buf + sizeof(msg_head_st) + sizeof(simple_gg2_head_st) );
	char shanghu[16] = {0};									// 商户id的字符串存放
	
	simple_gg2_node *node;	// 富媒体内容存放节点
	struct list_head *pos;	// 富媒体内容遍历

	// 上锁
	pthread_mutex_lock(&gv_simple_gg2_list_lock);
	
	for( pos = sgv_simple_gg2_list_head.next; pos != &sgv_simple_gg2_list_head; pos = pos->next ){
	
		// 清空上面申请的三块空间
		memset(shanghu, 0, 16);

		// 拿节点
		node = (simple_gg2_node*)pos;

		// 富媒体内容包赋值
		sg2->replace_pos_flag = node->location;					// 位置
		memcpy(sg2->replace_pos, node->categories, 128);		// 标示
		sg2->dns_len = strlen(node->fudomain);					// dns长度
		
		// js数据长度 脚本1 + 商户长度 + 脚本2
		snprintf(shanghu, sizeof(shanghu), "%u", shanghuid);
		sg2->js_len = strlen(node->script1) + strlen(node->script2) + strlen(shanghu);

		sg2->data_len = sg2->dns_len + sg2->js_len;				// dns和脚本长度

		// 判断数据长度是不是超过最大长度
		if( sg2->data_len >= 4090 || sg2->dns_len >= 1020){
			continue;
		}
		
		// 数据拼装
		sprintf(sg2->data, "%s%s%s%s", node->script1, shanghu, node->script2, node->fudomain);
	
		head->len += sizeof(simple_gg2_st) + sg2->data_len;
		msg->cnt++;
		msg->data_len += sizeof(simple_gg2_st) + sg2->data_len;

		sg2 = (simple_gg2_st*)( ((void*)sg2) + sizeof(simple_gg2_st) + sg2->data_len );
	}
	pthread_mutex_unlock(&gv_simple_gg2_list_lock);

//	xyprintf_simple_gg2_head_st(msg);

	//发送
	if( send_simple_gg2_head_st(sockfd, buf) ){
		free(buf);
		return -1;
	}

	xyprintf(0, "%u:--> --> Send simple_gg2 msg to router success --> -->", msg_head->router_id);
	
	free(buf);
	return 0;
}

/** 
 *@brief  获取路由器登陆密码
 *@param  sockfd	类型 int			要发送到设备的socket套接字
 *@param  msg_head  类型 msg_head_st*	已填充设备基本信息的msg_head_st指针
 *@return succ 0 failed -1
 */
int send_http_passwd_get(int sockfd, msg_head_st *msg_head)
{
	msg_head->mode	= 2;
	msg_head->order	= msg_order_httpd_pwd_get;
	msg_head->len	= sizeof(msg_head_st);

	if( send_head_get_msg(sockfd, msg_head) ){
		return -1;
	}

	xyprintf(0, "%u: --> --> Send http passwd get success --> -->", msg_head->router_id);
	return 0;
}

/** 
 *@brief  获取路由器登陆密码
 *@param  sockfd	类型 int			要发送到设备的socket套接字
 *@param  msg_head  类型 msg_head_st*	已填充设备基本信息的msg_head_st指针
 *@return succ 0 failed -1
 */
int send_monitor_set_st(int sockfd, msg_head_st *msg_head, unsigned int id, wt_sql_handle* handle)
{
	int state = 0;
	SQLBindCol(handle->sqlstr_handle, 1,	SQL_C_ULONG,		&state,			1024,				&handle->sql_err);
	//3.3执行SQL语句
	sprintf(handle->sql_str, "SELECT TOP 1 isopentz FROM mx_shebei WHERE id = %u", id);
	if (wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		return WT_SQL_ERROR;
	}
	handle->sql_ret=SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	int size = sizeof(msg_head_st) + sizeof(monitor_set_st) + 1;
	char* buf = malloc(size);
	memset(buf, 0, size);
	msg_head_st* head = (msg_head_st*)buf;
	monitor_set_st* msg = (monitor_set_st*)( buf + sizeof(msg_head_st) );

	//包头
	memcpy(head, msg_head, sizeof(msg_head_st));
	head->order = msg_order_wifi_monitor_set;
	head->mode	= 2;
	head->len	= size - 1;
	
	msg->state = state;
	msg->port = sgv_monitor_servport;
	msg->time = sgv_monitor_time;
	msg->timeout = sgv_monitor_timeout;
	sprintf(msg->address, "%s", sgv_monitor_servurl);

	xyprintf_monitor_set_st(msg);

	//发送
	if( send_monitor_set_st_msg(sockfd, buf) ){
		free(buf);
		return -1;
	}

	xyprintf(0, "%u:--> --> Send monitor set msg to router success --> -->", msg_head->router_id);
	
	free(buf);
	return 0;
}
