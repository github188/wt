/*****************************************************
 *
 * 工具函数
 *
 *****************************************************/

#include "header.h"

#define VERSION_ARRAY	tears_in_heaven
#define PASS_ARRAY		my_back_pages

#define LY_BYTE_CODE	heart_of_gold
#define LY_BIT_CODE		you_belong_to_me
#define GET_MD5			tangled_up_in_blue

const char* VERSION_ARRAY[1] = {"5.1.17"};

char PASS_ARRAY[6][10] = {
	{7, 9, 8, 4, 6, 1, 3, 0, 2, 5},
	{0, 9, 2, 7, 4, 1, 5, 3, 6, 8},
	{3, 7, 0, 8, 4, 6, 2, 1, 5, 9},
	{1, 8, 4, 3, 0, 9, 6, 2, 7, 5},
	{3, 6, 9, 2, 8, 5, 0, 1, 7, 4},
	{4, 5, 8, 6, 1, 0, 7, 3, 9, 2},
};

int LY_BYTE_CODE(char* version, char* buf, unsigned int len, int flag)
{
	int pass_no;
	for(pass_no = 0; pass_no < sizeof(VERSION_ARRAY) / sizeof(VERSION_ARRAY[0]); pass_no++){
		if(!strcmp(VERSION_ARRAY[pass_no], version)){
			break;
		}
	}

	if(pass_no >= sizeof(VERSION_ARRAY) / sizeof(VERSION_ARRAY[0])){
		return -1;
	}

	int i, j;
	char temp[10];
	if(flag){
		for(i = 0; i < len / 10; i++){
			memcpy(temp, buf + i * 10, 10);
			for(j = 0; j < 10; j++){
				buf[i * 10 + j] = temp[ PASS_ARRAY[pass_no][j] ];
			}
		}
	} else {
		for(i = 0; i < len / 10; i++){
			memcpy(temp, buf + i * 10, 10);
			for(j = 0; j < 10; j++){
				buf[ i * 10 + PASS_ARRAY[pass_no][j] ] = temp[ j ];
			}
		}
	}
	return 0;
}

void LY_BIT_CODE(char *buf, unsigned int len, int flag)
{
	int i;
	char temp[4];
	if(flag){
		for(i = 0; i < len; i++){
			temp[0] = buf[i] & 0x60;	// 01100000
			temp[1] = buf[i] & 0x10;	// 00010000
			temp[2] = buf[i] & 0x8;		// 00001000
			temp[3] = buf[i] & 0x6;		// 00000110
			buf[i] &= 0x81;				// 10000001
			buf[i] = buf[i] | (temp[0] >> 1) | (temp[1] << 2) | (temp[2] >> 2) | (temp[3] << 1);//03125647
		}
	} else {
		for(i = 0; i < len; i++){
			temp[0] = buf[i] & 0x40;	// 01000000
			temp[1] = buf[i] & 0x30;	// 00110000
			temp[2] = buf[i] & 0xC;		// 00001100
			temp[3] = buf[i] & 0x2;		// 00000010
			buf[i] &= 0x81;				// 10000001
			buf[i] = buf[i] | (temp[0] >> 2) | (temp[1] << 1) | (temp[2] >> 1) | (temp[3] << 2);//01234567
		}
	}
}

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))
#define RL(x, y) (((x) << (y)) | ((x) >> (32 - (y)))) 
//x向左循环移y位
#define PP(x) (x<<24)|((x<<8)&0xff0000)|((x>>8)&0xff00)|(x>>24) 
//将x高低位互换,例如PP(aabbccdd)=ddccbbaa
#define FF(a, b, c, d, x, s, ac) a = b + (RL((a + F(b,c,d) + x + ac),s))
#define GG(a, b, c, d, x, s, ac) a = b + (RL((a + G(b,c,d) + x + ac),s))
#define HH(a, b, c, d, x, s, ac) a = b + (RL((a + H(b,c,d) + x + ac),s))
#define II(a, b, c, d, x, s, ac) a = b + (RL((a + I(b,c,d) + x + ac),s))

//获取MD5码
void GET_MD5(char *in, unsigned int len, char *out)
{
    unsigned int x[16]; 
    char szData[64+1]; 
    unsigned int A, B, C, D, a, b, c, d; 
    A=0x67452301; 
    B=0xefcdab89; 
    C=0x98badcfe; 
    D=0x10325476; 
    a=A; b=B; c=C; d=D; 
    memset(&x, 0, 64); 
    memset(szData, 0, sizeof(szData)); 

	memcpy(szData, in, len); 
    szData[len] = 128; 
    memcpy(&x, szData, len+1); 
    x[14] = len%256*8; 
    x[15] = len/256; 
   
	/* Round 1 */ 
    FF (a, b, c, d, x[ 0], 7, 0xd76aa478); /**/
    /* 1 */ 
    FF (d, a, b, c, x[ 1], 12, 0xe8c7b756); /**/
    /* 2 */ 
    FF (c, d, a, b, x[ 2], 17, 0x242070db); /**/
    /* 3 */ 
    FF (b, c, d, a, x[ 3], 22, 0xc1bdceee); /**/
    /* 4 */ 
    FF (a, b, c, d, x[ 4], 7, 0xf57c0faf); /**/
    /* 5 */ 
    FF (d, a, b, c, x[ 5], 12, 0x4787c62a); /**/
    /* 6 */ 
    FF (c, d, a, b, x[ 6], 17, 0xa8304613); /**/
    /* 7 */ 
    FF (b, c, d, a, x[ 7], 22, 0xfd469501); /**/
    /* 8 */ 
    FF (a, b, c, d, x[ 8], 7, 0x698098d8); /**/
    /* 9 */ 
    FF (d, a, b, c, x[ 9], 12, 0x8b44f7af); /**/
    /* 10 */ 
    FF (c, d, a, b, x[10], 17, 0xffff5bb1); /**//* 11 */ 
    FF (b, c, d, a, x[11], 22, 0x895cd7be); /**//* 12 */ 
    FF (a, b, c, d, x[12], 7, 0x6b901122); /**//* 13 */ 
    FF (d, a, b, c, x[13], 12, 0xfd987193); /**//* 14 */
    FF (c, d, a, b, x[14], 17, 0xa679438e); /**//* 15 */
    FF (b, c, d, a, x[15], 22, 0x49b40821); /**//* 16 */ /**//* Round 2 */ 
    GG (a, b, c, d, x[ 1], 5, 0xf61e2562); /**//* 17 */ 
    GG (d, a, b, c, x[ 6], 9, 0xc040b340); /**//* 18 */
    GG (c, d, a, b, x[11], 14, 0x265e5a51); /**//* 19 */ 
    GG (b, c, d, a, x[ 0], 20, 0xe9b6c7aa); /**//* 20 */ 
    GG (a, b, c, d, x[ 5], 5, 0xd62f105d); /**//* 21 */ 
    GG (d, a, b, c, x[10], 9, 0x02441453); /**//* 22 */ 
    GG (c, d, a, b, x[15], 14, 0xd8a1e681); /**//* 23 */ 
    GG (b, c, d, a, x[ 4], 20, 0xe7d3fbc8); /**//* 24 */
    GG (a, b, c, d, x[ 9], 5, 0x21e1cde6); /**//* 25 */ 
    GG (d, a, b, c, x[14], 9, 0xc33707d6); /**//* 26 */ 
    GG (c, d, a, b, x[ 3], 14, 0xf4d50d87); /**//* 27 */ 
    GG (b, c, d, a, x[ 8], 20, 0x455a14ed); /**//* 28 */ 
    GG (a, b, c, d, x[13], 5, 0xa9e3e905); /**//* 29 */ 
    GG (d, a, b, c, x[ 2], 9, 0xfcefa3f8); /**//* 30 */ 
    GG (c, d, a, b, x[ 7], 14, 0x676f02d9); /**//* 31 */ 
    GG (b, c, d, a, x[12], 20, 0x8d2a4c8a); /**//* 32 */ /**//* Round 3 */
    HH (a, b, c, d, x[ 5], 4, 0xfffa3942); /**//* 33 */ 
    HH (d, a, b, c, x[ 8], 11, 0x8771f681); /**//* 34 */ 
    HH (c, d, a, b, x[11], 16, 0x6d9d6122); /**//* 35 */ 
    HH (b, c, d, a, x[14], 23, 0xfde5380c); /**//* 36 */
    HH (a, b, c, d, x[ 1], 4, 0xa4beea44); /**//* 37 */
    HH (d, a, b, c, x[ 4], 11, 0x4bdecfa9); /**//* 38 */ 
    HH (c, d, a, b, x[ 7], 16, 0xf6bb4b60); /**//* 39 */ 
    HH (b, c, d, a, x[10], 23, 0xbebfbc70); /**//* 40 */ 
    HH (a, b, c, d, x[13], 4, 0x289b7ec6); /**//* 41 */
    HH (d, a, b, c, x[ 0], 11, 0xeaa127fa); /**//* 42 */ 
    HH (c, d, a, b, x[ 3], 16, 0xd4ef3085); /**//* 43 */ 
    HH (b, c, d, a, x[ 6], 23, 0x04881d05); /**//* 44 */ 
    HH (a, b, c, d, x[ 9], 4, 0xd9d4d039); /**//* 45 */ 
    HH (d, a, b, c, x[12], 11, 0xe6db99e5); /**//* 46 */ 
    HH (c, d, a, b, x[15], 16, 0x1fa27cf8); /**//* 47 */ 
    HH (b, c, d, a, x[ 2], 23, 0xc4ac5665); /**//* 48 */ /**//* Round 4 */
    II (a, b, c, d, x[ 0], 6, 0xf4292244); /**//* 49 */ 
    II (d, a, b, c, x[ 7], 10, 0x432aff97); /**//* 50 */ 
    II (c, d, a, b, x[14], 15, 0xab9423a7); /**//* 51 */ 
    II (b, c, d, a, x[ 5], 21, 0xfc93a039); /**//* 52 */ 
    II (a, b, c, d, x[12], 6, 0x655b59c3); /**//* 53 */ 
    II (d, a, b, c, x[ 3], 10, 0x8f0ccc92); /**//* 54 */ 
    II (c, d, a, b, x[10], 15, 0xffeff47d); /**//* 55 */ 
    II (b, c, d, a, x[ 1], 21, 0x85845dd1); /**//* 56 */
    II (a, b, c, d, x[ 8], 6, 0x6fa87e4f); /**//* 57 */
    II (d, a, b, c, x[15], 10, 0xfe2ce6e0); /**//* 58 */
    II (c, d, a, b, x[ 6], 15, 0xa3014314); /**//* 59 */ 
    II (b, c, d, a, x[13], 21, 0x4e0811a1); /**//* 60 */ 
    II (a, b, c, d, x[ 4], 6, 0xf7537e82); /**//* 61 */ 
    II (d, a, b, c, x[11], 10, 0xbd3af235); /**//* 62 */
    II (c, d, a, b, x[ 2], 15, 0x2ad7d2bb); /**//* 63 */
    II (b, c, d, a, x[ 9], 21, 0xeb86d391); /**//* 64 */ 
    sprintf(out, "%08x", PP(a+A));
    sprintf(out+8, "%08x", PP(b+B));
    sprintf(out+16, "%08x", PP(c+C));
    sprintf(out+24, "%08x", PP(d+D));
}

/*
void get_random_array(int max_num)
{
	srand(time(0));
	int buf[ max_num ];
	int i;
	for(i = 0; i <  max_num ; i++){
		buf[i] = 0;
	}

	int count = 0;

	printf("\t\t{");

	while(count <  max_num ){
		i = rand() %  max_num ;
		if( !buf[i] ){
			buf[i] = 1;
			count++;
			printf("%d", i);
			if( count % 20 == 0 ){
				printf("\n\t\t");
			}
			if(count != max_num){
				printf(", ");
			}
		}
	}
	printf("}\n");
}*/

int SEND_BOSSS_MSG_OF_MD5(char* version, int sockfd, char* buf, int len)
{
	int ret = 0;
	char md5[33] = {0};

	// 获取报文的md5
	char *msg = buf + sizeof(bg_msg_head);
	if(len - sizeof(bg_msg_head) > 64){
		GET_MD5(msg, 64, md5);
	}
	else {
		GET_MD5(msg, len - sizeof(bg_msg_head), md5);
	}
	

	// 报文头长度值修改
	bg_msg_head *head = (bg_msg_head*)buf;
	head->len = len + 32;
	
	// 申请新的报文空间
	char *md5_buf = malloc(head->len);
	memset(md5_buf, 0, head->len);

	// 拷贝 报文头 md5 报文到报文空间
	memcpy(md5_buf, head, sizeof(bg_msg_head));
	memcpy(md5_buf + sizeof(bg_msg_head), md5, 32);
	memcpy(md5_buf + sizeof(bg_msg_head) + 32, msg, len - sizeof(bg_msg_head) );

	// byte加密 报文部分
	if(LY_BYTE_CODE(version, md5_buf + sizeof(bg_msg_head), head->len - sizeof(bg_msg_head), 1) ){
		return -1;
	}
	// bit 加密 报文部分
	LY_BIT_CODE(md5_buf + sizeof(bg_msg_head), head->len - sizeof(bg_msg_head), 1);

	//xyprintf_bg_msg_head(md5_buf);

	// 发送
	ret = wt_send_block( sockfd, md5_buf, head->len );


	if(md5_buf){
		free(md5_buf);
	}

	return ret;
}

int MD5_MSG_RESOLOVE(char* version, char* md5_msg, int md5_len, char *msg, int *len)
{
	char md5[33] = {0};
	char omd5[33] = {0};

	// bit 解密
	LY_BIT_CODE(md5_msg, md5_len, 0);

	// byte加密
	if(LY_BYTE_CODE(version, md5_msg, md5_len, 0) ){
		return -1;
	}

	// 拷贝 报文头 报文到报文空间
	memcpy(omd5, md5_msg, 32);
	memcpy(msg, md5_msg + 32, md5_len - 32);

	*len = md5_len - 32;

	// 获取报文的md5
	if(*len > 64){
		GET_MD5(msg + 32, 64, md5);
	}
	else {
		GET_MD5(msg + 32, *len, md5);
	}

	// 验证md5值
	if( !memcmp(md5, omd5, 33) ){
		return -1;
	}
	
	return 0;
}
