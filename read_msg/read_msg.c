#include "read_msg.h"



MYSQL msg_mysql;
FILE * log_file;
static phones_t phones;
MYSQL_RES *result;
int main(int argc,char * argv[])
{

	
	int tty_fd;


	//memset(&msg_request_queue,0,sizeof(msg_request_queue));
	


		log_file=fopen(LOG_FILE,"a+");
		if(log_file==NULL)
			{
				perror("fopen");
				return -1;
			}
			
			tty_fd=open(DEVICE_TTYS,O_RDWR|O_NOCTTY);
			//tty_fd=open(DEVICE_TTYS,O_RDWR|O_NOCTTY|O_NONBLOCK);
			if(tty_fd<0)
				{
					printf("open ttyS0 failed\n");
					log_warn(log_file,"open %s failed\n",DEVICE_TTYS);
				}
			fcntl(tty_fd,F_SETFL,0);/*�ָ�����Ϊ����״̬*/
			int is=0;
			is=isatty(tty_fd);
			if(is<0){
			printf("%s is not a tty device\n",DEVICE_TTYS);	
			log_warn(log_file,"%s is not a tty device\n",DEVICE_TTYS);
			}
			//���ô��ڲ���
			init_ttyS(tty_fd);
			//���ö���è����
			set_short_msg_mode(tty_fd);
			log_warn(log_file,"modem set already\n");
	
	while(1)
	{
		
			//read the un read message from the modem	
		//	printf("Begin to read message from modem\n");
			//�û���֤
			
		//�����Ų����ݶ�����������Ӧ�������͵Ķ���ҲҪ��¼��
		//ת���Ѷ��Ķ���
		
		GSM_Read_Message_Of_Type(tty_fd,UNREAD_MESSAGE);
		//printf("END\n");
	
		
		
		//ɾ���Ѷ����ѷ��͵Ķ���(#define DELETE_MESSAGE "at+cmgd=1,2\r")
		if(send_GSM_GPRS_cmd(tty_fd,DELETE_MESSAGE)==0)
			{
				log_warn(log_file,"msg delete succeed\n");
			}
			
		

		sleep(RCV_CIRCLE_TIME);

		
	}
	
	close(tty_fd);
	return 0;
}

int   gsmDecode7bit(const   unsigned   char*   pSrc,   char*   pDst,   int   nSrcLength)
{
		int   nSrc; //   Դ�ַ����ļ���ֵ
		int   nDst; //   Ŀ����봮�ļ���ֵ
		int   nByte; //   ��ǰ���ڴ���������ֽڵ���ţ���Χ��0-6
		unsigned   char   nLeft; //   ��һ�ֽڲ��������
		
		//   ����ֵ��ʼ��
		nSrc   =   0;
		nDst   =   0;
		
		//   �����ֽ���źͲ������ݳ�ʼ��
		nByte   =   0;
		nLeft   =   0;
		
		//   ��Դ����ÿ7���ֽڷ�Ϊһ�飬��ѹ����8���ֽ�
		//   ѭ���ô�����̣�ֱ��Դ���ݱ�������
		//   ������鲻��7�ֽڣ�Ҳ����ȷ����
		while(nSrc<nSrcLength)
		{
		//   ��Դ�ֽ��ұ߲��������������ӣ�ȥ�����λ���õ�һ��Ŀ������ֽ�
		*pDst   =   ((*pSrc   <<   nByte)   |   nLeft)   &   0x7f;
		
		//   �����ֽ�ʣ�µ���߲��֣���Ϊ�������ݱ�������
		nLeft   =   *pSrc   >>   (7-nByte);
		
		//   �޸�Ŀ�괮��ָ��ͼ���ֵ
		pDst++;
		nDst++;
		
		//   �޸��ֽڼ���ֵ
		nByte++;
		
		//   ����һ������һ���ֽ�
		if(nByte   ==   7)
		{
		//   ����õ�һ��Ŀ������ֽ�
		*pDst   =   nLeft;
		//   �޸�Ŀ�괮��ָ��ͼ���ֵ
		pDst++;
		nDst++;
		
		//   �����ֽ���źͲ������ݳ�ʼ��
		nByte   =   0;
		nLeft   =   0;
		}
		
		//   �޸�Դ����ָ��ͼ���ֵ
		pSrc++;
		nSrc++;
	}

	//   ����ַ����Ӹ�������
	*pDst   =   '\0';
	//   ����Ŀ�괮����
	return   nDst;
}

int get_system_status(sys_status_t * status)
{
	if(NULL == status)
		{
				return 0;
		}
	memset(status,0,sizeof(sys_status_t));
	status->cpu = get_cpu();
	status->mem = get_mem();
	status->disk = get_disk();
	
	get_runtime(status->runtime);

	
	return 1;
}






void system_communication(int fd,const msg_t msg)
{
		PRST_PARAM msg_info;
		memset(&msg_info,0,sizeof(msg_info));
		msg_info.TP_PID = 0x00;
		msg_info.TP_DCS = 0x08;
		msg_info.VP=0x01;
		sprintf(msg_info.TPA,"%s",msg.r_addr);
		/*��ʾ��"���ã��밴�����²�����ʾ�ظ���Ӧ������
		101����ѯcpu,mem,disk,runtime����Ϣ
		201��ȡ���澯
		202�������澯
		301���ػ�
		302������"
		3:�澯"
		*/
		/*
		1:��ѯ
		101��cpu,mem,disk,runtime

		2:�޸�
		
		3��ϵͳ����
		301���ػ�
		302������
		 */
		int msg_flag =0;
		if(strstr(msg.data,MSG_REBOOT) != NULL)
			{
				//send information to phoneNum
				msg_flag =1;
				sprintf(msg_info.TP_DA,"�豸��������");
				GSM_Send_PDUMessage(fd,&msg_info);
				insert_send_msg_to_mysql(&msg_mysql,&msg_info);
				system("reboot");
				//reboot();
				return ;
			}
		if(strstr(msg.data,MSG_SHUTDOWN) != NULL)
			{
				//send information to phoneNum
				msg_flag =1;
				sprintf(msg_info.TP_DA,"�豸����60���ػ�");
				GSM_Send_PDUMessage(fd,&msg_info);	
				insert_send_msg_to_mysql(&msg_mysql,&msg_info);
				system("shutdown -s -t 60");
			//	shutdown();
				return ;
			}
		if(strstr(msg.data,MSG_SYSINFO) != NULL)
			{
				msg_flag =1;
				//send information to phoneNum
				sys_status_t status;
				if(get_system_status(&status) != 0)
				{
					sprintf(msg_info.TP_DA,"ϵͳ״̬��\nmem:%.2f\%,cpu:%.2f\%,hard disk:%.2f\%,run_time:%s",status.mem,status.cpu,status.disk,status.runtime);
					GSM_Send_PDUMessage(fd,&msg_info);	
					insert_send_msg_to_mysql(&msg_mysql,&msg_info);					
					return ;
				}
			}
		if(strstr(msg.data,MSG_ALARM_CANCEL) != NULL)
			{
				//send information to phoneNum
				msg_flag =1;
				sprintf(msg_info.TP_DA,"�澯��ȡ��");
				GSM_Send_PDUMessage(fd,&msg_info);	
				//����policy_cfg_alarm_keyword_threshold,valid_flag
				//alarm_config,enabled
				insert_send_msg_to_mysql(&msg_mysql,&msg_info);
				
				char sql_update_command[1000];
				sprintf(sql_update_command,"UPDATE alarm_config SET enabled = 0");
				
				if(connect_mysql(&msg_mysql)==-1){                              //�������ݿ�
						
					return ;
				}
				update_mysql(&msg_mysql,sql_update_command);                //�������ݿ�
				close_mysql(&msg_mysql);
				
				return ;
			}	
		if(strstr(msg.data,MSG_ALARM) != NULL)
			{
				//send information to phoneNum
				msg_flag =1;
				sprintf(msg_info.TP_DA,"�����澯��");
				GSM_Send_PDUMessage(fd,&msg_info);	
				insert_send_msg_to_mysql(&msg_mysql,&msg_info);
				
				char sql_update_command[1000];
				sprintf(sql_update_command,"UPDATE alarm_config  SET enabled = 1");
				
				if(connect_mysql(&msg_mysql)==-1){                              //�������ݿ�
						
					return ;
				}
				update_mysql(&msg_mysql,sql_update_command);                //�������ݿ�
				close_mysql(&msg_mysql);
				
				//����
				return ;
			}	
		if(msg_flag==0)
		{
			sprintf(msg_info.TP_DA,"�밴�����²�����ʾ�ظ���Ӧ������\n101����ѯcpu,mem,disk����Ϣ\n201��ȡ���澯\n202�������澯\n301���ػ�\n302������");
			GSM_Send_PDUMessage(fd,&msg_info);
			insert_send_msg_to_mysql(&msg_mysql,&msg_info);			
			return ;
		}

}



int EncodeUCS2(unsigned char *SourceBuf,unsigned char *DestBuf)
{
    int len=0,i,j=0;
    wchar_t wcbuf[255];

    setlocale(LC_ALL,"");
    len = mbstowcs(wcbuf,SourceBuf,255); /* convert mutibytes string to wide charater string */
    for (i=0;i<len;i++)
    {
        DestBuf[j++] = wcbuf[i]>>8;     /* height byte */
        DestBuf[j++] = wcbuf[i]&0xff;   /* low byte */
    }
    return len*2;
}

int DecodeUCS2(unsigned char *SourceBuf,unsigned char *DestBuf,int len)
{
    wchar_t wcbuf[255];
    int i;

    setlocale(LC_ALL,"");
    for( i=0;i<len/2;i++ ) {
        wcbuf[i]=SourceBuf[2*i];    // height byte
        wcbuf[i]=(wcbuf[i]<<8)+SourceBuf[2*i+1];    // low byte
    }
    return wcstombs(DestBuf,wcbuf,len); /* convert wide charater string to mutibytes string */
}




void display_msg(msg_t msg)
{
	printf("SMSC_len = %d\n",msg.SMSC_len);
	printf("SMSC_f = %s\n",msg.SMSC_f);
	printf("SMSC_addr = %s\n",msg.SMSC_addr);
	printf("basic_param = %s\n",msg.basic_param);
	printf("r_addr_num = %d\n",msg.r_addr_num);
	printf("r_addr_f = %s\n",msg.r_addr_f);
	printf("r_addr = %s\n",msg.r_addr);
	printf("protol_flag = %s\n",msg.protol_flag);
	printf("TP_DCS = %c\n",msg.TP_DCS);
	printf("time_stamp = %s\n",msg.time_stamp);
	printf("TP_UDL = %d\n",msg.TP_UDL);
	printf("data = %s\n",msg.data);
	
}

int alpha2int(char alpha)
{
	if(alpha >= '0' && alpha <= '9')
		return alpha-'0';
	if(alpha >= 'a' && alpha <= 'z')
		return alpha-'a'+10;
	if(alpha >= 'A' && alpha <= 'Z')
		return alpha-'A'+10;
	
	return -1;
}

msg_t parse_pdu(const char * buf,int len)
{
		msg_t msg;
		int SMSC_len =0;
		int first_num;
		int second_num;
		int pos = 0;
		int i=0;
		int addr_num;
		char temp[32]={0};
		int char_num =0;
		//calculate the SMSC  number;

		memset(&msg,0,sizeof(msg_t));
		first_num = alpha2int(buf[pos++]);
		second_num = alpha2int(buf[pos++]);
		SMSC_len = first_num*16+second_num;
		msg.SMSC_len = SMSC_len;
		pos += SMSC_len*2;//
		pos += 2;//basic param
		
		first_num = alpha2int(buf[pos++]);
		second_num = alpha2int(buf[pos++]);
		pos += 2; //tiaoguo �ظ���ַ��ʽ(TON/NPI)
		msg.r_addr_num = first_num*16+second_num;
		addr_num	 = (msg.r_addr_num+1)/2*2;
		for(i=0;i<addr_num;i++)
		{
				temp[i] = buf[pos++];
		}
		
		for(i=0;i<addr_num;i+=2)
		{
				char t ;
				t = temp[i] ;
				temp[i] = temp[i+1];
				temp[i+1] = t;
		}
		if(temp[addr_num-1] == 'F' ||temp[addr_num-1] == 'f')
			{
				temp[addr_num-1] = 0;
				addr_num -= 1;
			}
		sprintf(msg.r_addr,"%s",temp); //get the telephone 
		
		//transform the temp to normal format of telephone number
		
		
		pos += 2; //tiao guo  TP-PID
		pos += 1; //tiao guo the first byte;
		
		if('8' == buf[pos++])
			{
				 //printf("this is the UCS2 \n");
				msg.TP_DCS = CODE_USC2;
			}
		else
			{
				//printf("this is the 7-bit\n");
				msg.TP_DCS = CODE_7BIT;
			
			}
		pos += 14;//tiao guo timeStamp
		
		first_num = alpha2int(buf[pos++]);
		second_num = alpha2int(buf[pos++]);
		msg.TP_UDL = first_num*16 + second_num;
		i=0;
							//printf("parse_pdu#\n");

		if(msg.TP_DCS == CODE_USC2)
			{
					char_num = msg.TP_UDL*2;
			}
			else
			{
					//����
					char_num = 2*((msg.TP_UDL/8)*7+(msg.TP_UDL%8));

			}
			//printf("char_num=%d\n",char_num);

			while(i < char_num)
			{
				//printf("buf[%d]=%c\n",pos,buf[pos]);
				msg.data[i++] = buf[pos++];
	
			}
				//printf("\n");
		msg.data_len = char_num ;//��¼���ݵ��ֽ���
		//printf("parse_pdu#msg.data=%s\n",msg.data);
		return msg;
}



/*return value :
	-1:error
	0: no match
	1:match successful
*/
int deal_return_data(const char * buf,char * output)
{
	int cflag = REG_EXTENDED;
	regmatch_t pmatch[1];
	const size_t nmatch=1;
	int i=0;
	int status =-1;
	regex_t reg;
	const char * pattern="0891683[0-9A-Fa-f]+";
	regcomp(&reg,pattern,cflag);
	status = regexec(&reg,buf,nmatch,pmatch,0);
	if(status == REG_NOMATCH)
		{
			//printf("No match\n");
			regfree(&reg);
			return 0;
		}
	if(status == 0)
		{
			if(output == NULL)
			{
					//printf("Output is NULL\n");
					return -1;
			}
			for(i=pmatch[0].rm_so;i<pmatch[0].rm_eo;i++)
			{
				output[i-pmatch[0].rm_so] = buf[i];
			}
		}
		regfree(&reg);
	//printf("content=%s",strstr(buf,prefix));
	
	return pmatch[0].rm_eo;
	
}


void GSM_Read_UnRead_Message(int fd)
{
	//char *send_buf="at+cmgl=0\r";
	char rcv_buf[2048]={0};
	char udl[256]={0};
	char udlBytes[256]={0};
	unsigned char msg_content[256]={0};
	int ret = -1;
	int pos;
	
	char username[32]={0};
	char passwd[32] = {0};
	
	msg_t msg;
  GSM_GPRS_send_cmd_save_result(fd,UNREAD_MESSAGE,rcv_buf,RECEIVE_BUF_WAIT_3S);
  /*
  if(1)
 	{
  		printf("recv_Buf=%s\n",rcv_buf);
  }
  */
  pos = 0;
 
  while(1)
  	{
	  	ret = deal_return_data(rcv_buf+pos,udl);
	  	if(ret <= 0) break;
	  	pos += ret;
	  	msg = parse_pdu(udl,strlen(udl));

			//printf("msg.data=%s\n",msg.data);
	  	string2bytes(msg.data,udlBytes,msg.data_len);
			//	printf("udlBytes=%s\n",udlBytes);
			//	printf("msg.data=%d\n",msg.TP_UDL);
  		  		  	
	  	memset(msg_content,0,256);
	  	//���� USC2����
	  		if(msg.TP_DCS == CODE_USC2)
			{
					DecodeUCS2(udlBytes,msg_content,msg.TP_UDL);
				//	printf("####UCS2:msg_content:%s\n",msg_content);

			}
			else
			{
				//���� 7-bit����
					gsmDecode7bit(udlBytes,msg_content,msg.data_len);	
				//	printf("####7-Bit:msg_content:%s\n",msg_content);

			}
			sprintf(msg.data,"%s",msg_content);
			
//			int identify_user(MYSQL * m_mysql,const char * username,const char * password)
		//�ֻ�����֤
		
			int identify =0;
			char user_phone[15];
			char sql_cmd[200];
			
			memcpy(user_phone , msg.r_addr,strlen(msg.r_addr)-2);
			sprintf(sql_cmd,"select * from alarm_config where tel_number ='%s'",user_phone);
			//sprintf(sql_cmd,"select * from user_phone_number where phone_num ='%s'",user_phone);
			//sprintf(sql_cmd,"select * from alarm_config where tel_number ='%s'",user_phone);//���պͿ�����һ����
			if(connect_mysql(&msg_mysql)==-1){

                return ;
			}
			 if(mysql_real_query(&msg_mysql,sql_cmd,strlen(sql_cmd))!=0)
			{
				printf("query error !\n");
				exit(1);
			}
			 if((result=mysql_store_result(&msg_mysql))!=NULL)
			 {
				identify =1;
			 }
			 else
			 {
				identify =0;
				
			 }
			 close_mysql(&msg_mysql);
			 
/*
		if((msg_content[0] =='d' || msg_content[0] =='D')&&(msg_content[1] =='L' ||msg_content[1] =='l' ))
			{
				//��¼��Ϣ
				sscanf(msg_content,"%*[^.].%[^@]@%s",username,passwd);
				printf("username=%s\n",username);
				printf("password=%s\n",passwd);
				ret = identify_user(&m_mysql,username,passwd);
				//��֤ͨ��
				if(ret == 1)
					{
						//��֤ͨ��������һ��ȷ����Ϣ���û�
					}
					else
					{
						//��֤ûͨ��������ʧ����Ϣ���û���
					}
				
			}
			*/
			if(identify)
			//if(1)
				{
				  //	printf("-------------------------\n");
				  //	display_msg(msg);
					//����ȡ�Ķ��Ŵ������ݿ�
					insert_read_msg_to_mysql(&msg_mysql,msg);
					system_communication(fd,msg);
					
				}
				else
				{
					printf("��֤ʧ��\n");
					log_warn(log_file,"authority failed\n");
				}
				
				//save the command into the command queue
	  		
	  		
	 		
  	}

	//printf("end read all short message\n");	
}

void GSM_Read_Readed_Message(int fd)
{
//char *send_buf="at+cmgl=0\r";
	char rcv_buf[2048]={0};
	char udl[256]={0};
	char udlBytes[256]={0};
	unsigned char msg_content[256]={0};
	int ret = -1;
	int pos;
	msg_t msg;
  GSM_GPRS_send_cmd_save_result(fd,READED_MESSAGE,rcv_buf,RECEIVE_BUF_WAIT_3S);
  
  pos = 0;
 
  while(1)
  	{
	  	ret = deal_return_data(rcv_buf+pos,udl);
	  	pos += ret;
	  		if(ret <=0 ) break;
	  	msg = parse_pdu(udl,strlen(udl));
	
	  	string2bytes(msg.data,udlBytes,msg.TP_UDL*2);
			DecodeUCS2(udlBytes,msg_content,msg.TP_UDL);
			sprintf(msg.data,"%s",msg_content);
			
			printf("-------------------------\n");
	  	display_msg(msg);
	  	printf("-------------------------\n");
  	}

	printf("end read all Readed short message\n");		
}
void GSM_Read_All_Message(int fd){}
void GSM_Read_Sended_Message(int fd){}
 

void GSM_Read_Message_Of_Type(int fd,char * type){

		if(type == UNREAD_MESSAGE)
			{
				GSM_Read_UnRead_Message(fd);
				return ;
			}
		if(type == READED_MESSAGE)
		{
				GSM_Read_Readed_Message(fd);
				return ;
			}
		if(type == SENDED_MESSAGE)
			{
				GSM_Read_Sended_Message(fd);
				return ;
			}
		if(type == ALL_MESSAGE)
			{
					GSM_Read_All_Message(fd);
					return ;
			}
}





void init_ttyS(int fd)
{
	struct termios options;
	bzero(&options, sizeof(options));       // clear options
	
	cfsetispeed(&options,B9600);            // setup baud rate
	cfsetospeed(&options,B9600);
	
	options.c_cflag |= CS8;				/*��������λ*/
	options.c_cflag |= CREAD | CLOCAL;	/*�������Ӻͽ���ʹ��*/
	//options.c_cflag &=~CSIZE;			/*��������c_cflagѡ���λ����λ����*/
	
	options.c_cflag &= ~PARENB;			/*������У��λ*/
	options.c_iflag &= ~INPCK;		
	
	options.c_cflag &= ~CSTOPB;			/*����ֹͣλΪ1λ*/
	options.c_cflag &=~CRTSCTS;			/*������Ӳ������*/

	/*options.c_cflag |= CS8;				
	options.c_cflag |= CREAD | CLOCAL;	
	options.c_cflag &= ~PARENB;			
	options.c_iflag &= ~INPCK;		
	options.c_cflag &= ~CSTOPB;		*/	
	
	//�������豸����Ϊԭʼģʽ
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag &= ~OPOST;
	
	/*options.c_oflag &= ~(ONLCR | OCRNL);    
	options.c_iflag &= ~(ICRNL | INLCR);
	options.c_iflag &= ~(IXON | IXOFF | IXANY);   
	options.c_cc[VTIME] = 0;
	options.c_cc[VMIN] = 0;
	//options.c_cc[VTIME]=1;
       //options.c_cc[VMIN]=1; */
	int status=0;
	status=tcsetattr(fd, TCSANOW, &options);
	if(status<0)
		{
		log_warn(log_file,"tcsetattr of the console failed\n");
		}
	else{
		log_warn(log_file,"init the console sucessed\n");
		}
	tcflush(fd, TCIOFLUSH);
}//end init_ttyS

void set_short_msg_mode(int fd){
	/*��Ϊҵ����Ŷ����������Խ���������Ϊpduģʽ*/
	char * cmd_buf="at+cmgf=0\r\0";
	int retval=0;
	retval=GSM_GPRS_send_cmd_read_result(fd,cmd_buf,RESPONSE_TIME_WAIT);
	if(retval>=0)
		{
		log_warn(log_file,"set pdu mode succeed\n");
		}
	else{
		log_warn(log_file,"set pdu mode failed\n");
		}
	char *warn_buf="at+cmee=1\r\0";
	retval=GSM_GPRS_send_cmd_read_result(fd,warn_buf,RESPONSE_TIME_WAIT);
	if(retval>=0)
		{
		log_warn(log_file,"set cmee mode succeed\n");
		}
	else{
		log_warn(log_file,"set cmee mode failed\n");
		}
}
void log_warn(FILE* file,const char * fmt,...)
{
	/*�����ж��ļ���С���������������ļ�*/
	int fd=fileno(file);
	if(fd<0)
		return;
	struct stat st;
	if(fstat(fd,&st)<0)
		return;
	if(st.st_size>50000000)
		{
		/*����ļ���д*/
		ftruncate(fd, 0);
		rewind(file);
		}
	/*����־֮ǰ���ϵͳʱ��*/
	va_list arg;
	char t_buffer[40]={0};
	time_t now;
	time(&now);
	ctime_r(&now,t_buffer);
	fwrite(t_buffer,strlen(t_buffer),1,file);	
	va_start(arg,fmt);
	if(!file){
	va_end(arg);
	return;
	}
	vfprintf(file,fmt,arg);
	va_end(arg);
	fflush(file);
	return;
}


		//SND_MESSAGE
void 	GSM_Send_Alarm_Message(int fd)
{
	struct dirent * pdir;
	char filename[128]={0};
	DIR * dir;
		dir = opendir(FILE_DIR);
		while((pdir=readdir(dir)) != NULL)
		{
				if(strncmp(pdir->d_name,"msg_",4) == 0)
				{		
					memset(filename,0,128);
					sprintf(filename,"%s/%s",FILE_DIR,pdir->d_name);
					process_message(fd,filename);
					printf("serial_short_msg.c#GSM_Send_Alarm_Message#process_message Over\n");
				}
		}
	closedir(dir);
	return ;
}

int GSM_Send_PDUMessage(int fd,const PRST_PARAM* input)
{
	int len;
	len=strlen(input->TP_DA);
	char cmd[32]={0};
	char pdu[1024]={0};
	int retval=0;
	
	time_t  t;
	struct tm m_tm;
	char sql[256] = {0};
	char timeStr[256] = {0};
	
	retval=gsm_pdu_encode(input,pdu);
	int tail=0;
	tail=strlen(pdu);
	pdu[tail]=0x1a;
	pdu[tail+1]=0;
	//printf("CSM_Send_PDUMessage#Src pdu=%s\n",input->TP_DA);
	//printf("CSM_Send_PDUMessage#pdu=%s\n",pdu);
	sprintf(cmd,"at+cmgs=%03d\r\0",(retval-32)/2+15);
	//printf("CSM_Send_PDUMessage#cmd=%s\n",cmd);
	int ret=0;
	if(!MODE_MODEM)
		{
			//write the data to database
				memset(sql,0,256);
				time(&t);
				localtime_r(&t,&m_tm);
				memset(timeStr,0,256);
				sprintf(timeStr,"%04d-%02d-%02d %02d:%02d:%02d",m_tm.tm_year+1900,m_tm.tm_mon+1,m_tm.tm_mday,m_tm.tm_hour,m_tm.tm_min,m_tm.tm_sec);
				sprintf(sql,"insert into alarm_sms_log(phone_number,sms_content,log_time) values('%s','%s','%s')",input->TPA,input->TP_DA,timeStr);
				/*
				if(0 == mysql_real_query(&m_mysql,sql,strlen(sql)))
					{
						printf("############INSERT SUCCESSFULLY!!!!\n");
					}
					else
					{
			 			printf("############INSERT FAILED!!!!\n");

					}
				*/
				printf("INSERT DATABASE#%s\n",sql);
	
		}
		else
		{
			ret=GSM_GPRS_send_cmd_read_result(fd,cmd,RESPONSE_TIME_WAIT);
			if(ret==-1)
			{
			printf("send pdu message failed because at+cmgs failed\n");
			return -1;
			}
			ret=GSM_GPRS_send_cmd_read_result(fd, pdu,RESPONSE_TIME_WAIT);
			if(ret==-1){
			log_warn(log_file,"send pdu message failed because pdu code sent failed\n");
			return -1;
			}
		}

	/*	*/
	return 0;
	
} // end GSM_Send_Message

int GSM_GPRS_send_cmd_read_result(int fd,const char * const send_buf, int rcv_wait)
{
	
	char rcv_buf[2048]={0};
	char pdu_buf[2048]={0};
	int ret=0;
	//log_warn(log_file,"send Command %s\n",send_buf);
	if((send_buf==NULL) || (send_GSM_GPRS_cmd(fd,send_buf)==0))
	{	
		bzero(rcv_buf,sizeof(rcv_buf));
		usleep(RESPONSE_TIME_WAIT_MULTI);
		if ((ret=read_GSM_GPRS_datas(fd,rcv_buf,rcv_wait))>0)
			{
				log_warn(log_file,"read data %s\n",rcv_buf);
				//printf("rcv_buf=###%s###\n",rcv_buf);
				
				//printf("content=%s\n",strstr(rcv_buf,prefix));
				//printf("*********************\n");
				ret =	deal_return_data(rcv_buf,pdu_buf);
				if(ret < 0)
					{
						printf("failed to deal with return data\n");
						return -1;
					}
				if(ret == 0){
						//printf("no data match\n");
						return 0;
				}
				//printf("%s\n",pdu_buf);
				//get_pdu_info(pdu_buf,);
				//printf("*********************\n");

		//		printf("rcv_buf=###%s###\n",rcv_buf);
				if(strstr(rcv_buf,"OK")!=NULL||strstr(rcv_buf,"> ")!=NULL)
					return 0;
				else
					return -1;
			}
	            else
	            {
	            	log_warn(log_file,"GSM_GPRS_send_cmd_read_result read failed\n");
	            	return -1; 
	            }

	}
	else
	{
		perror("write");
		log_warn(log_file,"write error\n");
		return -1;
	}
	
}

int GSM_GPRS_send_cmd_save_result(int fd,const char * const send_buf,char * output,int rcv_wait)
{
	
	char rcv_buf[2048]={0};
	int ret=0;
	//log_warn(log_file,"send Command %s\n",send_buf);
	if((send_buf==NULL) || (send_GSM_GPRS_cmd(fd,send_buf)==0))
	{	
		bzero(rcv_buf,sizeof(rcv_buf));
		usleep(RESPONSE_TIME_WAIT_MULTI);
		if ((ret=read_GSM_GPRS_datas(fd,rcv_buf,rcv_wait))>0)
			{
				log_warn(log_file,"read data %s\n",rcv_buf);
				//printf("rcv_buf=###%s###\n",rcv_buf);
				sprintf(output,"%s",rcv_buf);
				//printf("content=%s\n",strstr(rcv_buf,prefix));
				//printf("*********************\n");
				if(strstr(rcv_buf,"OK")!=NULL||strstr(rcv_buf,"> ")!=NULL)
					return 0;
				else
					return -1;
			}
	            else
	            {
	            	log_warn(log_file,"GSM_GPRS_send_cmd_read_result read failed\n");
	            	return -1; 
	            }

	}
	else
	{
		perror("write");
		log_warn(log_file,"write error\n");
		return -1;
	}
	
}

/*
*	���˺����޸ĳ�Ϊ
*	���ض�ȡ��������
*/
int read_GSM_GPRS_datas(int fd, char *rcv_buf,int rcv_wait)
{
	//tcflush(fd, TCIFLUSH);
	int retval;
	fd_set rfds;
	struct timeval tv;
	int ret,pos;
	
	pos = 0;
	int count=0;
	while (1)
	{
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		tv.tv_sec = rcv_wait;
		tv.tv_usec = 0;
		retval = select(fd+1 , &rfds, NULL, NULL, &tv);
		if (retval <0) 
		{     
			log_warn(log_file,"select error before writing\n");
			perror("select");
			continue;
		}
		else if (retval>0) 
		{
				ret = read(fd, rcv_buf+pos, 2048);
				if(ret==-1){
					perror("read");
					usleep(RESPONSE_TIME_WAIT_MULTI);
					continue;
					return -1;
					}
				pos += ret;
				if(strstr(rcv_buf,"ERROR")!=NULL){
					log_warn(log_file,"read rcv= ~%s~\n",rcv_buf);
					return -1;
				}
				if(strstr(rcv_buf,"OK")==NULL&&strstr(rcv_buf,">")==NULL){
					count++;
					if(count>10)
						break;
					log_warn(log_file,"read rcv= ~%s~\n",rcv_buf);
					usleep(RESPONSE_TIME_WAIT_MULTI);
					continue;
					}
				return pos;
                } 
		else if(retval==0)
		{
			//log_warn(log_file,"No data\n");
			count++;
			if(count>5)
				break;
                     continue;
                     //break;
                }
	}
	log_warn(log_file,"read data %s\n",rcv_buf);
	return pos;
}


int send_GSM_GPRS_cmd(int fd,const char *const send_buf)
{	
	tcflush(fd, TCOFLUSH);
	log_warn(log_file,"send to modem %s\n",send_buf);
	fd_set w_fd;
	int retval;
	int count=0;
	while(1){
	FD_ZERO(&w_fd);
	FD_SET(fd, &w_fd);
	struct timeval tv;
	tv.tv_sec = RESPONSE_TIME_WAIT_MULTI;	
	tv.tv_usec = 0;
	retval=select(fd+1,NULL,&w_fd,NULL,&tv);
	if(retval==-1)
		{
		count++;
		perror("select");
		usleep(RESPONSE_TIME_WAIT_MULTI);
		if(count>3)return -1;
		continue;
		}
	else if(retval){
		int ret=0;
		int bytes_count=strlen(send_buf);
		ret=write(fd,send_buf,bytes_count);
		tcdrain(fd);
		if(ret<0){
		log_warn(log_file,"write device %s error:%s\n", DEVICE_TTYS,strerror(errno));	
		return -1;}
		while(ret<bytes_count){
		int i=0;
		usleep(RESPONSE_TIME_WAIT_MULTI);
		if((i=write(fd,send_buf+ret,bytes_count-ret))<0){
		log_warn(log_file,"write device %s error:%s\n", DEVICE_TTYS,strerror(errno));	
		return -1;
		}
		ret+=i;
		tcdrain(fd);
		}
		log_warn(log_file,"write %d bytes\n",ret);	
		return 0;
		}
	else{
		log_warn(log_file,"��ʱ����д\n");	
		count++;
		usleep(RESPONSE_TIME_WAIT_MULTI);
		if(count>3)return -1;
		continue;
	}
	}
} // end send_GSM_GPRS_cmd

/*�ú�������Ҫ���õĽ��ı��ļ��е�һ�еĵ绰����洢�ں����ж�̬�����
�ڴ�ռ��У����ҷ��ض�Ӧ��phone_t�ṹ��*/
phones_t* phone_num_init(char * input){
	/*����绰����*/
	int i =0;

	int phone_count=get_phone_count(input);
	
	memset(&phones,0,sizeof(phones_t));	
	phones.phone_num = phone_count;
	if(phone_count<=0)
		return NULL;
	
	
	for(i=0;i<phone_count;i++){
		if(get_phone_num(input,i,&phones.array[i], phone_count)<0)
		{
			log_warn(log_file,"phone num string:%s  is in illegal format\n",input);
			return NULL;
			}
		//printf("%s\n",phone_info->array[i].num);
		}
	return NULL;
	
}

/*������ȡ���绰����ĸ���
*@phone_nums �绰�����ַ���ʹ��SEPARATOR�ָ�
*/
int get_phone_count(char * phone_nums)
{
	int len=strlen(phone_nums);
	if(len<=0)
		return -1;
	int count=0;
	int i=0;
	for(i=0;i<len;i++){
		if(phone_nums[i]==SEPARATOR)
			count++;
	}
	return count+1;
}

/*������ȡ���绰�����ַ����еĵ�count���绰����
*@phone_nums �绰�����ַ���ʹ��SEPARATE�ָ�
*@count Ϊ��Ҫȡ���ĵڼ����绰�����1��ʼ
*@num�洢�绰����Ľṹ��struct phones
*@max_count��ʾ��Ϊ���ĵ绰����
*@����ֵ��ʾ�Ƿ���ȷ����
*/
int get_phone_num(char * phone_nums,int count,phone_t *num,int max_count){
	char tmp[20]={0};
	//��Ҫ��������ʽ,������벻Ϊ86��ͷ�����86
	if(count>max_count||count<0)
		return -1;
	char * head,*tail;
	head=get_head_position(phone_nums,count,max_count);
	if(head==NULL){
		return -1;
		}
	tail=get_tail_position(phone_nums,count,max_count);
	if(tail==NULL){
		return -1;}
	memset(num->num,0,sizeof(num->num));
	memcpy(num->num,head,tail-head);
	if(strlen(num->num)<2)
		return -1;
	if(num->num[0]!='8'||num->num[1]!='6'){
		//���û��86�����86��ǰ��
		strncat(tmp,"86",2);
		strncat(tmp,num->num,strlen(num->num));
		memset(num->num,0,sizeof(num->num));
		memcpy(num->num,tmp,strlen(tmp));
	}
	//printf("%s\n",num->num);
	num->len=strlen(num->num);
	return 0;
	
}
/*��ȡ1,2,3,4,5�ַ����е�count��ͷ��ָ��1, ȡ��1��λ��
*/
char * get_head_position(char * phone_nums,int count,int max_count){
	if(count<0||count>=max_count)
		return NULL;
	if(count==0)
		return phone_nums;
	int i=0;
	int times=0;
	for(i=0;i<strlen(phone_nums);i++)
	{
		if(times==count)
			return phone_nums+i;
		if(phone_nums[i]==SEPARATOR)
			times++;
		}
		return NULL;
}
/*��ȡ1,2,3,4,5�ַ����е�count��β�͵�ָ��1,ȡ��,��λ��
*/
char * get_tail_position(char * phone_nums,int count,int max_count){
	if(count<0||count>=max_count)
		return NULL;
	if(count==max_count-1)
		return phone_nums+strlen(phone_nums);
	int i=0;
	int times=0;
	count++;
	for(i=0;i<strlen(phone_nums);i++)
	{
		if(times==count)
			return phone_nums+i-1;
		if(phone_nums[i]==SEPARATOR)
			times++;
		}
		return NULL;
}
/*
*	���ｫ�������ݸĳ�unicode���뷽ʽ��
*	
*/
int ucs2_encode(const char * input,int input_len ,char * output){
	int outlen;//unicode�ַ�����
	wchar_t w_buf[2048]={0};
	const char *tmp_in=input;
	setlocale(LC_ALL,"zh_CN.GB18030");
	//setlocale(LC_ALL, "en_US");
//	�����ַ�ת����Ϊ���ַ�
	outlen=mbstowcs(w_buf,tmp_in,input_len);
	if(outlen<0)
	perror("mbstowcs");
	int i;
	/*����pdu��ʽ��UNICODE�ߵ��ֽڶԵ�*/
	for(i=0;i<outlen;i++){
		*output++=w_buf[i]>>8;
		*output++=w_buf[i]&0xff;
		}
	return outlen*2;
}
/*
*	���ֽ�����ת����Ϊ�ɴ�ӡ�ַ�
*	0xcb 0x78 ת����Ϊ"cb78"
*/
int bytes2string(const char * input,char * output,int input_len){
	const char tab[]="0123456789abcdef";
	const char * tmp_in=input;
	char * tmp_out=output;
	int i;
	for(i=0;i<input_len;i++){
	/*�����4λ*/
	*tmp_out++=tab[((*tmp_in&0xff)>>4)&0xff];
	/*�����4λ*/
	*tmp_out++=tab[*tmp_in&0x0f];
	tmp_in++;
	}
	//*tmp_out='\0';
	return input_len*2;
}
/*
*	���ɴ�ӡ�ַ���ת����Ϊ�ֽ�����
*	"cb78"ת����Ϊ0xcb 0x78 
*/
int string2bytes(const char * input,char * output,int input_len){
	int i;
	//printf("input_len=%d\n",input_len);
	for(i=0;i<input_len;i+=2){
		//�����4λ
		if(*input>='0'&&*input<='9'){
			*output=(*input-'0')<<4;
			}
		else if(*input>='a'&&*input<='z'){
			*output=(*input-'a'+10)<<4;
			}
		else{
			*output=(*input-'A'+10)<<4;
			}
			input++;
		//�����4λ
		if(*input>='0'&&*input<='9'){
			*output|=*input-'0';
			}
		else if(*input>='a'&&*input<='z'){
			*output|=*input-'a'+10;
			}
		else{
			*output|=*input-'A'+10;
			}
		input++;
		output++;
		}
	return input_len/2;
}
/*
*	PDU�����ʱ��任��ʽ
*	"8613851872468" --> "683158812764F8"
*/
int pdu_invert_numbers(const char * input,char * output,int input_len){
	int output_len;
	char ch;
	output_len=input_len;
	int i;
	const char * tmp_in=input;
	char *tmp_out=output;
	/*�����ߵ�*/
	for(i=0;i<input_len;i+=2){
	ch=*tmp_in++;
	*tmp_out++=*tmp_in++;
	*tmp_out++=ch;
	}
	/*����ַ�����������������F*/
	if(input_len&1){
		*(tmp_out-2)='f';
		output_len++;
	}
	//*output='\0';
	return output_len;
}

/*
*	PDU����,���ڶ��ŵı��ƺͷ���
*	�����Ĺ���Ϊ��SM_PARAM�ṹ�Ľṹ��ת����ΪPDU��ʽ���ַ���
*	ע:�������в������SM_PARAM�еı��뷽ʽ���úα��뷽ʽʵ����һ��ʹ��
*	UCS2����
*/
int gsm_pdu_encode(const PRST_PARAM* input,char * output){
	int len;
	int outlen=0;
	char buf[256]={0};
	char * tmp=output;
	//ǰ��λ��Ϊ�̶�д��
	buf[0]=0x00;
	buf[1]=0x11;
	buf[2]=0x00;
	//�������ĳ���
	buf[3]=len=strlen(input->TPA)&0xff;
	//�̶�д��
	buf[4]=0x91;
	outlen+=bytes2string(buf,tmp+outlen,5);
/*TPA��Ŀ�����ת����ŵ�output��ȥ*/
	outlen+=pdu_invert_numbers(input->TPA,tmp+outlen,len);
	memset(buf,0,sizeof(buf));
	buf[0]=input->TP_PID;
	buf[1]=input->TP_DCS;
	buf[2]=input->VP;
	outlen+=bytes2string(buf,tmp+outlen,3);
/*TP-UD*/
	memset(buf,0,sizeof(buf));
	char * tmp_buf=buf;
	len=ucs2_encode(input->TP_DA,strlen(input->TP_DA),tmp_buf);
	char c=(len&0xff);
	outlen+=bytes2string(&c,tmp+outlen,1);
	outlen+=bytes2string(buf,tmp+outlen,len);
	return outlen;
}

/*
*	�ӿ��ж�ȡ�������ĺ���
*	����������ȡSIM���еĶ������ĺ���Ȼ�󽫺������ַ�������ʽ
*	�洢������output��ȥ��
*/
int read_sms_center_num(int fd,char * output){
	char * cmd_buf="at+csca?\r\0";
	char rcv_buf[2048];
	memset(rcv_buf,0,sizeof(rcv_buf));
	int ret=0;
	char * head,* tail;
	log_warn(log_file,"send Command %s\n",cmd_buf);
	if(send_GSM_GPRS_cmd(fd,cmd_buf)==0){
		if((ret=read_GSM_GPRS_datas(fd,rcv_buf,RESPONSE_TIME_WAIT))>0){
			head=strstr(rcv_buf,"\"+");
				if(head==NULL){
					log_warn(log_file,"can not find \"\n");
					return -1;}
			head+=2;
			tail=strstr(head,"\"");
				if(tail==NULL){
					log_warn(log_file,"can not find another\"\n");
					return -1;
					}
			memcpy(output,head,tail-head);
			return tail-head;
		}
		else
			{
			log_warn(log_file,"read failed\n");
			return -1;
			}
		}
	else{
		log_warn(log_file,"write failed\n");
		return -1;
	}
}

/*
*	����������message�澯�ַ���,�����е������ֻ����뷢�Ͷ���
*	�ļ�����Ϊ��(�ļ���ʽ:��һ���ļ�Ϊ�ֻ����벻ͬ���ֻ�����ʹ��","�ָ�)
*	�������Ϊ�ֻ���������,�����������140�ֽ�����Ҫ���Ϊ�������з���
*/



int process_message(int fd,char * filename){
	FILE *fp =NULL;
	if((fp=fopen(filename,"r"))==NULL){
 		log_warn(log_file,"fopen file %s failed\n",filename);
 		return -1;
		}
	char buf[10240]={0};
	//���ȶ�ȡ��һ�л�ȡ�����ź���
	if(fgets(buf,1024,fp)==NULL){
		log_warn(log_file,"fgets file %s failed\n",filename);
		return -1;
		}
		
	printf("process_message#phone_num:%s\n",buf);
	int len=strlen(buf);
	//ȥ������\r��\n����
	while(len>=1&&(buf[len-1]=='\n'||buf[len-1]=='\r' ||buf[len-1]==',')){
	buf[len-1]=0;
	len--;
	}
	phones_t * ps_t;
	phone_num_init(buf);
	
	int num_count;
	num_count=phones.phone_num;//ps_t->phone_num;
	//�����������������
	PRST_PARAM msg_info;
	memset(&msg_info,0,sizeof(msg_info));
	msg_info.TP_DCS=8;
	msg_info.TP_PID=0;
	//������д�ظ�����,��Ϣ����
	memset(buf,0,sizeof(buf));
	while(fgets(buf,10240,fp)){
		//buf��Ϊ���ŵ�����,���ڿ�ʼ���ж��Ų��
			printf("process_message#msg_content:%s\n",buf);

		int msg_count;
		msg_count=pducode_count(buf)/MAX_LEN_OF_SHORT_MESSAGE+1;
		printf("process_message#1316#msg_count=%d\n",msg_count);
		//char msg[161]={0};
		//��buf�е�;ȫ���滻��\n����
		int rt_count=substitute(buf);
		log_warn(log_file,"replaced %d count ';'  message content is %s\n",rt_count,buf);
		int i,j,pos=0;
		//�ȴ������,��һ��һ�����뷢�Ͷ��ţ����ȸ�һ�����뷢�����зֶζ���
		for(i=0;i<num_count;i++){
			pos=0;
			for(j=0;j<msg_count;j++){
			int n_count=0;
			memset(msg_info.TPA,0,sizeof(msg_info.TPA));
			memset(msg_info.TP_DA,0,sizeof(msg_info.TP_DA));
			memcpy(msg_info.TPA,phones.array[i].num,sizeof(msg_info.TPA));
			memcpy(msg_info.TP_DA,buf+pos,MAX_LEN_OF_SHORT_MESSAGE);
			//�������ַ�����pdu���ȿ��ܳ��������ֵ��			
			int pdu_count=pducode_count(msg_info.TP_DA);
			if(pdu_count>MAX_LEN_OF_SHORT_MESSAGE)
			{
				int virtual_len=strlen(msg_info.TP_DA);
				int pducut_len=pdu_count-virtual_len;
				char * temp=msg_info.TP_DA;
				int cut_len=pducut_count(temp,pducut_len);
				//�����������cut_len���ֽ�ȥ��
				int j;
				for(j=virtual_len-1;j>=virtual_len-cut_len;j--)
				{
					msg_info.TP_DA[j]=0;
				}
			}
			pos+=strlen(msg_info.TP_DA);
			printf("pos=%d\t content=%s\n",pos,msg_info.TP_DA);
			if(pos>0){
				char * tmp=msg_info.TP_DA;
				int is_first_byte=is_lead_char(tmp,strlen(msg_info.TP_DA));
				//ȷ���������ֽڵĺ��ִ��м佫�����ֽڷֿ���
				if(is_first_byte==0)
				{
				msg_info.TP_DA[strlen(msg_info.TP_DA)>0?strlen(msg_info.TP_DA)-1:0]=0;
				//˵�������һ���ֽ����º��ֵ����ֽڽ�pos--���ҽ�msg_info.TP_DA�е����һλ��0
				pos--;
				}
				}
			int retal=0;
			//show_msg_info(&msg_info);
send:
 
						retal=GSM_Send_PDUMessage(fd,&msg_info);
			 
		
			printf("send PDU Over and retValue=%d\n",retal);
			if(retal<0){
				n_count++;
				if(n_count<2){
				set_short_msg_mode(fd);
				goto send;
				}
				log_warn(log_file,"send message failed\n");
				}
			}
		}
		memset(buf,0,sizeof(buf));
		}
		
	fclose(fp);
	printf("$$delete file=%s\n",filename);
	remove(filename);//
	
	return 0;
	
}
void get_time_string(char *output){
	time_t t;
	time(&t);
	struct tm *now;
	now=gmtime(&t);
	int tmp=0;
	tmp=(now->tm_year+1900)%100;
	sprintf(output+strlen(output),"%02d",tmp);
	tmp=(now->tm_mon+1);
	sprintf(output+strlen(output),"%02d",tmp);
	tmp=now->tm_mday;
	sprintf(output+strlen(output),"%02d",tmp);
	tmp=now->tm_hour+8;
	sprintf(output+strlen(output),"%02d",tmp);
	tmp=now->tm_min;
	sprintf(output+strlen(output),"%02d",tmp);
	tmp=now->tm_sec;
	sprintf(output+strlen(output),"%02d",tmp);
	//�й�Ϊ��8��
	tmp=8;
	sprintf(output+strlen(output),"%02d",tmp);
	return;
}
int _daemon_init(void)
{
	pid_t pid;
	int ret;
        if ((pid = fork()) < 0){
        	return -1;
	}
        else if (pid != 0){
        	exit(0); 
	}
	if((ret=setsid()) < 0){
		printf("unable to setsid.\n");
	}
	setpgrp();
	return 0;
}

void show_msg_info(const PRST_PARAM* input){
	printf("TPA=%s\n",input->TPA);
	printf("TP_DA=%s\n",input->TP_DA);
	printf("TP_PID=%02x\n",input->TP_PID);
	printf("TP_DCS=%02x\n",input->TP_DCS);
	printf("VP=%02x\n",input->VP);
}
int code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int outlen)
{
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;

	cd = iconv_open(to_charset,from_charset);
	if (cd==0) return -1;
	memset(outbuf,0,outlen);
	if (iconv(cd,pin,(size_t *)&inlen,pout,(size_t*)&outlen)==-1) return -1;
	iconv_close(cd);
	return 0;
}
int gbk2unicode(char *inbuf,int inlen,char *outbuf,int outlen)
{
return code_convert("GBK\0","UNICODE//IGNORE\0",inbuf,inlen,outbuf,outlen);
}

int is_lead_char(char *buf,int strnum)
{
	int num=0;
	int i=0;
for(i=0;i<strnum;i++)
{
	if((unsigned char)(*buf)>=0x40)
	{
		if((num%2)==0)
		{
			if((unsigned char)(*buf)>=0x81)
			num++;
			else
			num=0;
		}
		else
		num++;
	}
else
	num=0;
buf++;
}
if((num%2)==1)
return 0;
else
return -1;
}
/*
*	@��������pInput�е�;�滻Ϊ\n
*	@pInput Ϊ��Ҫ�޸ĵ��ַ���
*	@���α�ʾ�滻�˶��ٸ���Ӧ���ַ�Ϊ�س���
*/
int substitute(char *pInput)
{
	char    *pi;
	pi = pInput;
	if(pi)
	{
	int len=strlen(pi);
	int i;
	int count=0;
	for(i=0;i<len;i++){
		if(*(pi+i)==';'){
		*(pi+i)='\n';
		count++;
		}
		}
	return count;
	}
	else{
	    return 0;
	}
}
/*
*	����������Ϊ�����ַ����ڽ���pdu�����ĳ���(ע:һ��asc����pdu����
*	ģʽ��Ҳռ�������ֽ�)
*	@pInput Ϊ���
*	@����ֵΪ��Ӧ�����ַ�����pdu���볤��
*/
int pducode_count(char * pInput){
	int len=strlen(pInput);
	int i;
	int count=0;
	char *pi=pInput;
	if(pi){
	for(i=0;i<len;i++){
	if((unsigned char)*(pi+i)<0x80)
	{
		count+=2;
		}
	else{
		count ++;
	}
	}
	return count;
	}
	else
	{
		return 0;
	}
}
/*
*	����������Ϊ���ַ���β����ʼ����ʵ����Ҫ�����pdu����ת����Ϊ��Ӧ�ַ�������
*	@pInput ��Ҫ������ַ���ָ��
*	@pducut_lenΪ��Ҫcut����pdu���볤��
*/
int pducut_count(char * pInput,int pducut_len)
{
	char * pi=pInput;
	int len=strlen(pi);
	int i;
	for(i=len-1;i>0;i--)
	{
		if((unsigned char )*(pi+i)<0x80){
			pducut_len-=2;
		}
		else{
			pducut_len--;
		}
		if(pducut_len<=0)
		break;
	}
	return len-i;
}
unsigned long get_file_time(const char *filename)
{
        struct stat buf;
        if(stat(filename, &buf)<0)
        {
                return 0;
        }
        return (unsigned long)buf.st_mtime;
        
}

int insert_read_msg_to_mysql(MYSQL *m_mysql, msg_t msg)
{
	char sql_insert_command[1000];
	char t_buffer[100];
	time_t now;
	struct tm st_time;
	
	memset(t_buffer,0,sizeof(t_buffer));
	memset(sql_insert_command,0,sizeof(sql_insert_command));
	
	time(&now);
	localtime_r(&now,&st_time);

	sprintf(t_buffer,"%04d-%02d-%02d %02d:%02d:%02d",st_time.tm_year+1900,st_time.tm_mon+1,st_time.tm_mday,st_time.tm_hour,st_time.tm_min,st_time.tm_sec);
	/*
	time(&now);
	st_time =localtime(&now);
	strftime(t_buffer,sizeof(t_buffer),"%Y-%m-%d:%H:%M:%S",st_time);
	*/
	sprintf(sql_insert_command,"insert into read_msg_info(phone_num,msg_content,col_time) VALUES('%s','%s','%s')",msg.r_addr,msg.data,t_buffer);
	//sql_insert_command[strlen(sql_insert_command)]='\0';
	if(connect_mysql(m_mysql)==-1){                             
						
		return -1;
	}
	insert_mysql(m_mysql,sql_insert_command);                //�������ݿ�
	close_mysql(m_mysql);
	return 0;
}
int insert_send_msg_to_mysql(MYSQL *m_mysql, PRST_PARAM* msg_info)
{
	char sql_insert_command[1000];
	char t_buffer[100];
	time_t now;
	struct tm st_time;
	
	memset(t_buffer,0,sizeof(t_buffer));
	memset(sql_insert_command,0,sizeof(sql_insert_command));
	
	time(&now);
	localtime_r(&now,&st_time);

	sprintf(t_buffer,"%04d-%02d-%02d %02d:%02d:%02d",st_time.tm_year+1900,st_time.tm_mon+1,st_time.tm_mday,st_time.tm_hour,st_time.tm_min,st_time.tm_sec);
	/*
	time(&now);
	st_time =localtime(&now);
	strftime(t_buffer,sizeof(t_buffer),"%Y-%m-%d:%H:%M:%S",st_time);
	*/
	sprintf(sql_insert_command,"insert into send_msg_info(phone_num,msg_content,col_time) VALUES('%s','%s','%s')",msg_info->TPA,msg_info->TP_DA,t_buffer);
	sql_insert_command[strlen(sql_insert_command)]='\0';
	if(connect_mysql(m_mysql)==-1){                              //�������ݿ�
						
		return -1;
	}
	insert_mysql(m_mysql,sql_insert_command);                //�������ݿ�
	close_mysql(m_mysql);
	return 0;
}

float get_cpu()

{
        FILE *fp;
        char buf[128];
        char cpu[5];
        long int user,nice,sys,idle,iowait,irq,softirq;

        long int all1,all2,idle1,idle2;
        float usage;
		float cpu_usage;
		int count =0;
        while(count<2)
        {
			count ++;
                fp = fopen("/proc/stat","r");
                if(fp == NULL)
                {
                        perror("fopen:");
                        exit (0);
                }


                fgets(buf,sizeof(buf),fp);

                sscanf(buf,"%s%d%d%d%d%d%d%d",cpu,&user,&nice,&sys,&idle,&iowait,&irq,&softirq);

                all1 = user+nice+sys+idle+iowait+irq+softirq;
                idle1 = idle;
                rewind(fp);
                /*�ڶ���ȡ����*/
                sleep(CK_TIME);
                memset(buf,0,sizeof(buf));
                cpu[0] = '\0';
                user=nice=sys=idle=iowait=irq=softirq=0;
                fgets(buf,sizeof(buf),fp);

                sscanf(buf,"%s%d%d%d%d%d%d%d",cpu,&user,&nice,&sys,&idle,&iowait,&irq,&softirq);

                all2 = user+nice+sys+idle+iowait+irq+softirq;
                idle2 = idle;

                usage = (float)(all2-all1-(idle2-idle1)) / (all2-all1)*100 ;

				if(count ==2)
				{
					//printf("all=%d\n",all2-all1);
					//printf("ilde=%d\n",all2-all1-(idle2-idle1));
					 //printf("cpu use = %.2f\%\n",usage); 
					cpu_usage =usage;
					//printf("=======================\n");
				}
                fclose(fp);
        }
        return cpu_usage;
}

float get_mem()
{
	FILE * fp;
	char buf[128];
	char memTotal[20];
	char memFree[20];
	char kb[5];
	float usage;
	long int m_total,m_free;
	fp =fopen("/proc/meminfo","r");
	if(fp == NULL)
	{
		perror("fopen:");
		
	}
	fgets(buf,sizeof(buf),fp);
	//printf("buf=%s",buf);
	sscanf(buf,"%s%d%s",memTotal,&m_total,kb);
	//printf("memToal:%d\n",m_total);
	fgets(buf,sizeof(buf),fp);
	//printf("buf=%s",buf);
	sscanf(buf,"%s%d%s",memFree,&m_free,kb);
	//printf("memFree:%d\n",m_free);
	 usage = (float)(m_total-m_free)/m_total*100;
	// printf("mem use = %.2f\%\n",usage); 
	return usage;
}

float get_disk()
{
	struct statfs diskInfo;
	
	statfs("/data", &diskInfo);
	unsigned long long blocksize = diskInfo.f_bsize;	
	unsigned long long totalsize = blocksize * diskInfo.f_blocks; 	
	/*
	printf("Total_size = %llu B = %llu KB = %llu MB = %llu GB\n", 
		totalsize, totalsize>>10, totalsize>>20, totalsize>>30);
		*/
	int d_total =totalsize>>30;
	/*
	printf("total==%d\n",d_total);
	*/
	unsigned long long freeDisk = diskInfo.f_bfree * blocksize;
	unsigned long long availableDisk = diskInfo.f_bavail * blocksize; 	
	int d_free=freeDisk>>30;
	/*
	printf("Disk_free = %llu MB = %llu GB\nDisk_available = %llu MB = %llu GB\n", 
		freeDisk>>20, freeDisk>>30, availableDisk>>20, availableDisk>>30);
	*/
	float usage = (float)(d_total-d_free)/d_total*100;
	
	
	return usage;
}

int get_runtime(char * runtime)
{
	FILE * fp;
	char buf[128],str1[50],str2[50];
	double run_time_sec;
	//char time_idle[100];
	char run_time[100];

	fp =fopen("/proc/uptime","r");
	if(fp == NULL)
	{
		perror("fopen:");
		
	}
	fgets(buf,sizeof(buf),fp);
	sscanf(buf,"%s%s",str1,str2);
	run_time_sec=atof(str1);
	int day =run_time_sec/(3600*24);
	double hour =(((int)run_time_sec)%(3600*24))/3600.0;
	sprintf(run_time,"%d��%.2lfСʱ",day,hour);
	memcpy(runtime,run_time,strlen(run_time));
	return 0;
}
/*
int insert_msg_to_mysql(MYSQL *m_mysql, const char *sql_insert_command)
{
	if(connect_mysql(&m_mysql)==-1){                              //�������ݿ�
						
		return -1;
	}
	insert_mysql(&m_mysql,sql_insert_command);                //�������ݿ�
	close_mysql(&m_mysql);
	return 0;
}
*/
//DROP TABLE IF EXISTS `read_send_msg_info` ; 
/*
DROP TABLE IF EXISTS `read_msg_info`;  
CREATE TABLE `read_msg_info` (  
  `id` int(11) NOT NULL AUTO_INCREMENT,  
  `phone_num` varchar(20) NOT NULL,  
  `msg_content` char(100) NOT NULL,
  `col_time` datetime  NOT NULL ,
  PRIMARY KEY (`id`)  
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;  
*/
/*
DROP TABLE IF EXISTS `send_msg_info`;  
CREATE TABLE `send_msg_info` (  
  `id` int(11) NOT NULL AUTO_INCREMENT,  
  `phone_num` varchar(20) NOT NULL,  
  `msg_content` char(100) NOT NULL,
  `col_time` datetime  NOT NULL ,
  PRIMARY KEY (`id`)  
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;  
DROP TABLE IF EXISTS `user_phone_number`;  
CREATE TABLE `user_phone_number` (  
  `id` int(11) NOT NULL AUTO_INCREMENT,  
  `phone_num` varchar(20) NOT NULL,  

  PRIMARY KEY (`id`)  
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;  
*/