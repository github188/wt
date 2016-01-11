/*****************************************************
 *
 * log文件读取函数
 *
 *****************************************************/
#include "header.h"

/** 
 *@brief  初始化ini, 打开文件， 将文件内容读取到内存中
 *@param  filename  类型 char*	配置文件名
 *@param  fd		类型 int*	存放打开文件的文件描述符
 *@param  buf		类型 char*  存放从配置文件中读取的内容
 *@param  len		类型 int    读取的最长内容
 *@return success 0 failed -1
 */

int init_ini(char* filename, int *fd, char* buf, int len)
{
	if(!filename || !fd || !buf || !len){
		printf("!filename || !fd || !buf || !len\n");
		return -1;
	}

	//打开log文件
	*fd = open(filename, O_RDONLY);
	if(*fd == -1){
		perror("open");
		return -1;
	}
	
	//读取log文件到内存
	if( read(*fd, buf, len) < 0){
		perror("read");
		return -1;
	}
	return 0;
}

/** 
 *@brief  获取配置
 *@param  buf		类型 char*  存放从配置文件中读取的内容
 *@param  key		类型 char*	要查找的配置项
 *@param  value		类型 char*  读取到的值
 *@return success 0 failed -1
 */
int get_ini(char *buf, const char* key, char* value)
{
	char temp_key[32] = { 0 };
	strcpy(temp_key, key);
	int len = strlen(temp_key);
	temp_key[len] = '=';	//组成字符串 key=
	len++;

	char *p = buf;
	char *pv = value;
	while( *p ){
		if( strncmp(p, temp_key, len) == 0 ){	//判断这一行的开头是否是key=
			p+=len;
			while( *p && *p != '\n' ){			//拷贝key=之后的内容到value 直到遇到换行或者结束符
				*pv++ = *p++;
			}
			*pv = 0;
			return 0;
		}
		while( *p && *p++ != '\n');//到下一行
	}
	return -1;
}

/** 
 *@brief  销毁ini文件符
 *@param  fd		类型 int	配置文件的描述符
 *@return 无
 */
void destroy_ini(int fd)
{
	if(fd){
		close(fd);
	}
}
