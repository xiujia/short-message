#ifndef _READ_MSG_
#define _READ_MSG_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <regex.h>


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
#include <mysql/mysql.h>
#include <pthread.h>
#include <sys/statfs.h>

#include <openssl/md5.h>

#include "op_db.h"

#define DATABASE_MODE 1
#define MODE_MODEM 1
#define MODE   DATABASE_MODE

#define UNREAD_MESSAGE "at+cmgl=0\r"
#define READED_MESSAGE "at+cmgl=1\r"
#define SENDED_MESSAGE "at+cmgl=2\r"
#define ALL_MESSAGE "at+cmgl=4\r"

#define DELETE_MESSAGE "at+cmgd=1,2\r"

#define DEVICE_TTYS "/dev/ttyS0"	//设置设备串口号
#define MAX_LEN_OF_SHORT_MESSAGE	140 //140	//短信最大长度
#define DAUDRATE B9600	//波特率
#define LOG_DIR	"/data/log/"
#define LOG_FILE "/data/log/send_read.log"


#define FILE_DIR "/data/alarm_to_send/"
#define RESPONSE_TIME_WAIT 5
#define RESPONSE_TIME_WAIT_MULTI 500000
#define RECEIVE_BUF_WAIT_3S 3
#define SEPARATOR ','

#define OVER_TIME 900

#define CANCLE_NUMBER_FILE "/data/cancled_number.txt"

#define RCV_CIRCLE_TIME 15
#define SND_CIRCLE_TIME 300

#define MSG_GATEWAY 2
#define MSG_MODEM 1
#define MSG_CLOSE 0

#define MAX_PHONE_NUM 10

#define MAX_ONLINE_TIME 30 //60*20 //20分钟内 没有操作，则认为用户下线。


#define CODE_7BIT  '7'
#define CODE_USC2  '8'

#define MSG_SYSINFO			"101"
#define MSG_ALARM_CANCEL	"201"
#define MSG_ALARM			"202"
#define MSG_SHUTDOWN		"301"
#define MSG_REBOOT			"302"

#define CK_TIME 1

typedef struct sys_status_t
{
	float cpu;
	float mem;
	float disk;
	char runtime[20];
}sys_status_t;

typedef struct msg_t {
	int SMSC_len;
	char SMSC_f[3];
	char SMSC_addr[16];
	char basic_param[3];
	int r_addr_num;
	char r_addr_f[3];
	char r_addr[16];
	char protol_flag[3];
	char TP_DCS;
	char time_stamp[16];
	int TP_UDL ;
	int data_len;
	char data[256];
}msg_t;

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
phone_t array[MAX_PHONE_NUM];
}phones_t;

/*短信查询请求队列*/
/*一个线程用于完成用户的查询请求，一个线程用于维护在线用户信息*/

/*
*/
//在线用户
typedef struct online_user_t
{
	char phone_num[20];
	char username[20];
	time_t mtime ;//上次更新时间，当时间超过阀值时，则让用户下线
	struct online_user_t * next;
} online_user_t;

typedef struct online_user_list_t
{
	 int count ;// 在线用户数量
	online_user_t user;//链表头
} online_user_list_t;

//某个用户使用某个短信号码发出了一条命令。
typedef struct msg_request_t
{
	int cmd_id ;//请求的命令
	char command[24];
	char phone_num[24];//请求的电话号码
	char user_name[24];//请求发起人
	struct msg_request_t  * next;//挑一个请求命令
} msg_request_t;

typedef struct msg_request_queue_t
{
	int count ;
	msg_request_t * p_msg_request_tail ;
	msg_request_t msg_request;//链表的头
}msg_request_queue_t;




/*函数声明*/
void init_ttyS(int fd);
void set_short_msg_mode(int fd);
void log_warn(FILE* file,const char * fmt,...);
int GSM_Send_PDUMessage(int fd,const PRST_PARAM* input);
int GSM_GPRS_send_cmd_read_result(int fd,const char * const send_buf, int rcv_wait);
int read_GSM_GPRS_datas(int fd, char *rcv_buf,int rcv_wait);
int send_GSM_GPRS_cmd(int fd,const char * const send_buf);
phones_t* phone_num_init(char * input);
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
//int process_alarm_message(int fd,char *message)；
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


int GSM_GPRS_send_cmd_save_result(int fd,const char * const send_buf,char * output,int rcv_wait);
//void GSM_Read_UnRead_Message(int fd);
int EncodeUCS2(unsigned char *SourceBuf,unsigned char *DestBuf);
int alpha2int(char alpha);
void display_msg(msg_t msg);
int deal_return_data(const char * buf,char * output);
void GSM_Read_Message_Of_Type(int fd,char * type);
void GSM_Read_UnRead_Message(int fd);
void GSM_Read_Readed_Message(int fd);
msg_t parse_pdu(const char * buf,int len);

void system_communication(int fd,const msg_t msg);
int get_system_status(sys_status_t * status);
//int num_cancle(const char * num);//
//int is_cancled(const char * num);//

void 	GSM_Send_Alarm_Message(int fd);

//void save_log(const char * content);


//void add_online_user(const char * username,const char * phone_num);
int insert_read_msg_to_mysql(MYSQL *m_mysql, msg_t msg);
int insert_send_msg_to_mysql(MYSQL *m_mysql, PRST_PARAM* msg_info);
//int insert_msg_to_mysql(MYSQL *m_mysql, const char *sql_insert_command);

float get_cpu();
float get_mem();
float get_disk();
int get_runtime(char * runtime);
#endif


