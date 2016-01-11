/*****************************************************
 *
 * 数据库操作函数
 *
 *****************************************************/

#include "header.h"

/**
 *@brief  数据库连接初始化函数
 *@param  handle		类型 wt_sql_handle*		数据库操作handle及其他资源集合
 *@return success 0, falied -1
 */
int wt_sql_init(wt_sql_handle *handle, char* sql_name, char* sql_user, char* sql_pass)
{
	//1.1获取环境句柄 
	handle->sql_ret = SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&handle->env_handle);//获得环境句柄 参数1：句柄类型 2：
	if ((handle->sql_ret != SQL_SUCCESS) && (handle->sql_ret != SQL_SUCCESS_WITH_INFO)){
		xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- SQLAllocHandle() failed!", __func__, __FILE__, __LINE__);
		goto ERR;
	}
	//1.2设定ODBC版本
	handle->sql_ret = SQLSetEnvAttr(handle->env_handle, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);//设定ODBC版本
	if ((handle->sql_ret != SQL_SUCCESS) && (handle->sql_ret != SQL_SUCCESS_WITH_INFO)){
		xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- SQLSerEnvAttr() failed!", __func__, __FILE__, __LINE__);
		goto ENV_ERR;
	}
	//2.1获取连接句柄
	handle->sql_ret = SQLAllocHandle(SQL_HANDLE_DBC, handle->env_handle, &handle->conn_handle);//获得连接句柄
	if ((handle->sql_ret != SQL_SUCCESS) && (handle->sql_ret != SQL_SUCCESS_WITH_INFO)){
		xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- SQLAllocHandle() failed!", __func__, __FILE__, __LINE__);
		goto ENV_ERR;
	}
	//2.2设定连接超时参数
	SQLSetConnectAttr(handle->conn_handle, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0);//设定连接超时参数
	//2.3连接数据库
	handle->sql_ret = SQLConnect(handle->conn_handle, (SQLCHAR*) sql_name, SQL_NTS, (SQLCHAR*) sql_user, SQL_NTS, (SQLCHAR*) sql_pass, SQL_NTS);//连接数据库
	if ((handle->sql_ret != SQL_SUCCESS) && (handle->sql_ret != SQL_SUCCESS_WITH_INFO)){
		SQLGetDiagRec(SQL_HANDLE_DBC, handle->conn_handle, 1, handle->sql_stat, (SQLINTEGER*)&handle->sql_err, handle->err_msg, 100, &handle->sql_mlen);//获取错误信息
		xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- SQLConnect() failed, error msg: %s -- (%d)!", __func__, __FILE__, __LINE__, handle->err_msg, (int)(handle->sql_err));
		goto CONN_ERR;
	}
	
	//3.1获取SQL语句句柄
	handle->sql_ret=SQLAllocHandle(SQL_HANDLE_STMT, handle->conn_handle, &handle->sqlstr_handle);//获取SQL语句句柄
	if ((handle->sql_ret != SQL_SUCCESS) && (handle->sql_ret != SQL_SUCCESS_WITH_INFO)){
		SQLGetDiagRec(SQL_HANDLE_DBC, handle->conn_handle, 1, handle->sql_stat, (SQLINTEGER*)&handle->sql_err, handle->err_msg, 100, &handle->sql_mlen);//获取错误信息
		xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- SQLAllocHandle() failed, error msg: %s -- (%d)!", __func__, __FILE__, __LINE__, handle->err_msg, (int)(handle->sql_err));
		goto CONN_ERR;
	}
	return 0;

CONNED_ERR:
    SQLDisconnect(handle->conn_handle);//关闭与数据库的连接
CONN_ERR:
    SQLFreeHandle(SQL_HANDLE_DBC, handle->conn_handle);
ENV_ERR:
	SQLFreeHandle(SQL_HANDLE_ENV, handle->env_handle);//释放句柄
ERR:
	return -1;
}

/**
 *@brief  数据库连接销毁函数
 *@param  handle		类型 wt_sql_handle*		数据库操作handle及其他资源集合
 *@return empty
 */
void wt_sql_destroy(wt_sql_handle *handle)
{
	SQLFreeHandle(SQL_HANDLE_STMT, handle->sqlstr_handle);
	SQLDisconnect(handle->conn_handle);//关闭与数据库的连接
	SQLFreeHandle(SQL_HANDLE_DBC, handle->conn_handle);
	SQLFreeHandle(SQL_HANDLE_ENV, handle->env_handle);//释放句柄
}

/**
 *@brief  数据库语句执行函数
 *@param  handle		类型 wt_sql_handle*		数据库操作handle及其他资源集合
 *@return success 0,failed -1
 */
int inline wt_sql_exec(wt_sql_handle *handle)
{
	//3.3执行SQL语句
	handle->sql_ret=SQLExecDirect(handle->sqlstr_handle, handle->sql_str, SQL_NTS);//执行语句
	//if ((handle->sql_ret != SQL_SUCCESS) && (handle->sql_ret != SQL_SUCCESS_WITH_INFO)){
	if (handle->sql_ret != SQL_SUCCESS){
		SQLGetDiagRec(SQL_HANDLE_STMT, handle->sqlstr_handle, 1, handle->sql_stat, (SQLINTEGER*)&handle->sql_err, handle->err_msg, 100, &handle->sql_mlen);//获取错误信息
		xyprintf(0, "SQL_EXEC_ERROR:SQL_HANDLE_STMT: sql_ret = %d, sql_stat = %s, sql_err = %d, sql_mlen = %d, err_msg : %s",
				handle->sql_ret,
				handle->sql_stat,
				(int)(handle->sql_err),
				handle->sql_mlen,
				handle->err_msg );
		SQLGetDiagRec(SQL_HANDLE_DBC, handle->conn_handle, 1, handle->sql_stat, (SQLINTEGER*)&handle->sql_err, handle->err_msg, 100, &handle->sql_mlen);//获取错误信息
		xyprintf(0, "SQL_EXEC_ERROR:SQL_HANDLE_DBC: sql_ret = %d, sql_stat = %s, sql_err = %d, sql_mlen = %d, err_msg : %s",
				handle->sql_ret,
				handle->sql_stat,
				(int)(handle->sql_err),
				handle->sql_mlen,
				handle->err_msg );
		//xyprintf(0, "SQL_ERROR -- EXEC: sql_ret = %d, err_msg : %s, sql_err = %d", handle->sql_ret, handle->err_msg, (int)(handle->sql_err) );
		return -1;
	}
	return 0;
}
