#ifndef _SERAIL_SHORT_MSG_
#define _SERAIL_SHORT_MSG_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>	
#include <unistd.h>	
#include <termios.h>	
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <locale.h>
#include <iconv.h>
#include "dhcc_mempool.h"

#define DEVICE_TTYS "/dev/ttyS0"	//设置设备串口号
#define MAX_LEN_OF_SHORT_MESSAGE	140	//短信最大长度
#define DAUDRATE B9600	//波特率
#define LOG_DIR	"/data/log/"
#define LOG_FILE "/data/log/msg.log"
//#define FILE_DIR "/home/xl/data/"
#define FILE_DIR "/data/alarm_msg/"
#define RESPONSE_TIME_WAIT 5
#define RESPONSE_TIME_WAIT_MULTI 500000
#define SEPARATOR ','
//OVER_TIME seconds
#define OVER_TIME 900

typedef struct {
// 短消息服务中心号码(SMSC地址)根据每个地方不同而不同。
    char SCA[16];       
// 目标号码或回复号码(TP-DA或TP-RA)
    char TPA[16];       
// 用户信息协议标识(TP-PID)使用00表示使用点对点GSM方式发送短信
    char TP_PID;        
// 用户信息编码方式(TP-DCS) 00 表示7-bit编码 08表示 UCS2编码
    char TP_DCS;        
// 服务时间戳字符串(TP_SCTS), 接收时用到格式为年月日时分秒时区110920
    char TP_SCTS[16];   
// 原始用户信息(编码前或解码后的TP-UD)
    char TP_UD[161];    
// 短消息序号，在读取时用到
    char index;         
} SM_PARAM;


typedef struct {
	//协议号0
	char TP_PID;
	//编码方式08表示UCS2编码
	char TP_DCS;
	//目标电话号码
	char TPA[16];
	//VP 01
	char VP;
	//短信内容
	char TP_DA[161];
}PRST_PARAM;



typedef struct phone_s{
char num[20];
int len;
}phone_t;

typedef struct phones_s{
int phone_num;
phone_t array[1];
}phones_t;
/*函数声明*/
void init_ttyS(int fd);
void set_short_msg_mode(int fd);
void log_warn(FILE* file,const char * fmt,...);
int GSM_Send_PDUMessage(int fd,const PRST_PARAM* input);
int GSM_GPRS_send_cmd_read_result(int fd,const char * const send_buf, int rcv_wait);
int read_GSM_GPRS_datas(int fd, char *rcv_buf,int rcv_wait);
int send_GSM_GPRS_cmd(int fd,const char * const send_buf);
phones_t* phone_num_init(char * input,dhcc_pool_t *pool);
int get_phone_count(char * phone_nums);
int get_phone_num(char * phone_nums,int count,phone_t *num,int max_count);
char * get_head_position(char * phone_nums,int count,int max_count);
char * get_tail_position(char * phone_nums,int count,int max_count);
int ucs2_encode(const char * input,int input_len ,char * output);
int bytes2string(const char * input,char * output,int input_len);
int string2bytes(const char * input,char * output,int input_len);
int pdu_invert_numbers(const char * input,char * output,int input_len);
int gsm_pdu_encode(const PRST_PARAM* input,char * output);
int read_sms_center_num(int fd,char * output);
int process_message(int fd,char * filename);
void get_time_string(char *output);
int _daemon_init(void);
void show_msg_info(const PRST_PARAM* input);
int code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int outlen);
int gbk2unicode(char *inbuf,int inlen,char *outbuf,int outlen);
int is_lead_char(char *buf,int strnum);
int substitute(char *pInput);
int pducode_count(char * pInput);
int pducut_count(char * pInput,int pducut_len);
unsigned long get_file_time(const char *filename);
#endif
