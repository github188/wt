/*****************************************************
 *
 * 数据库操作封装函数
 *
 *****************************************************/
#include "sauth_header.h"

/** 
 *@brief  引导服务器链表数据添加
 *@return nothing
 */
void guide_list_add(unsigned int id, char* url, unsigned int port)
{
	guide_node *node = malloc( sizeof(guide_node) );
	memset(node, 0, sizeof(guide_node) );
	node->id = id;
	node->port = port;
	memcpy(node->url, url, GUIDE_URL_LEN );

	pthread_mutex_lock(&gv_guide_lock);
	list_add(&(node->node), &gv_guide_head);
	xyprintf(0, "GUIDE_ADD:id -- %u\n\turl -- %s\n\tport = %u",
			node->id, node->url, node->port);
	pthread_mutex_unlock(&gv_guide_lock);
}

/** 
 *@brief  引导服务器链表数据清除
 *@return nothing
 */
void guide_list_clean()
{
	struct list_head* pos = gv_guide_head.next;
	
	pthread_mutex_lock(&gv_guide_lock);
	while( pos != &gv_guide_head ){
		
		list_del( pos );
		if(pos){
			free( pos );
		}
		pos = gv_guide_head.next;
	}
	
	pthread_mutex_unlock(&gv_guide_lock);
}

/** 
 *@brief  通过引导服务器的id获取其他数据
 *@return found return 0, else return -1
 */
int guide_list_get(unsigned int id, char* url, unsigned int *port)
{
	struct list_head* pos;
	guide_node *node = NULL;
	int ret = -1;
	
	pthread_mutex_lock(&gv_guide_lock);
	for( pos = gv_guide_head.next; pos != &gv_guide_head; pos = pos->next ){
		node = (guide_node*)pos;
		if( node->id == id ){
			memcpy(url, node->url, GUIDE_URL_LEN);
			*port = node->port;
			pthread_mutex_unlock(&gv_guide_lock);
			return 0;
		}
	}
	pthread_mutex_unlock(&gv_guide_lock);

	return -1;
}

/** 
 *@brief  读取引导服务器
 *@return succ 0 failed -1
 */
int get_guide_list()
{
	wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	
	if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
		xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
		goto ERR;
	}

	//读取设备升级地址
	unsigned id, port;
	char url[ GUIDE_URL_LEN ] = {0};
	SQLBindCol(handle->sqlstr_handle, 1,	SQL_C_ULONG,	&id,	20,				&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 2,	SQL_C_CHAR,		url,	GUIDE_URL_LEN,	&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 3,	SQL_C_ULONG,	&port,	20,				&handle->sql_err);

	//3.3执行SQL语句
	sprintf(handle->sql_str, "SELECT id, ydservaddr, ydservprot from mx_s_user");
	if (wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		goto SQL_ERR;
	}

	guide_list_clean();

	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	while(handle->sql_ret != SQL_NO_DATA){
		guide_list_add(id, url, port);
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	}
	
	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
	return 0;

SQL_ERR:
	wt_sql_destroy(handle);
ERR:
	if(handle){
		free(handle);
	}
	return -1;
}
