/*****************************************************
 *
 * 头文件
 *
 *****************************************************/
#ifndef GUIDE_HEADER_H
#define GUIDE_HEADER_H

#include "list.h"
#include "cJSON.h"

#define AUTHENTICATE_ADDR		"superyd.20080531.com"

//全局变量 读取自配置文件
extern char cgv_sql_name[32];					// sql name
extern char cgv_sql_user[32];					// sql user
extern char cgv_sql_pass[32];					// sql password
extern int	cgv_guide_port;						// 引导服务器开放给设备的端口
extern int	cgv_authenticate_port;				// 认证服务器开放给设备的端口
extern unsigned int	gv_guide_router_count;		// 引导过的设备计数器

#endif // GUIDE_HEADER_H
