#include"serial_short_msg.h"

FILE * log_file;
dhcc_pool_t * pool;
char center_num[20]={0};
int main(int argc,char * argv[])
//int msg_deal(char * filename)
{

	_daemon_init();
	char command[200]={0};
	sprintf(command,"mkdir -p %s",FILE_DIR);
	system(command);
	sprintf(command,"mkdir -p %s",LOG_DIR);
	system(command);
	
	log_file=fopen(LOG_FILE,"a+");
	if(log_file==NULL)
		{
			perror("fopen");
			return -1;
		}
	int tty_fd;
	tty_fd=open(DEVICE_TTYS,O_RDWR|O_NOCTTY);
	//tty_fd=open(DEVICE_TTYS,O_RDWR|O_NOCTTY|O_NONBLOCK);
	if(tty_fd<0)
		{
			printf("open ttyS0 failed\n");
			log_warn(log_file,"open %s failed\n",DEVICE_TTYS);
		}
	fcntl(tty_fd,F_SETFL,0);/*恢复串口为阻塞状态*/
	int is=0;
	is=isatty(tty_fd);
	if(is<0){
	printf("%s is not a tty device\n",DEVICE_TTYS);	
	log_warn(log_file,"%s is not a tty device\n",DEVICE_TTYS);
	}
	init_ttyS(tty_fd);
	
	/*//获取短信中心号码
	int ret=0;
	ret=read_sms_center_num(tty_fd,center_num);
	if(ret<=0){
		log_warn(log_file,"read center num failed\n");
		return -1;
		}
	printf("get the center num :%s\n",center_num);*/
	set_short_msg_mode(tty_fd);
	log_warn(log_file,"beging to read dir\n");
	
	//处理待发送短信的文件
		char *filename = argv[1];
		char msg_filename[100]={0};
		sprintf(msg_filename,"%s%s",FILE_DIR,filename);
		log_warn(log_file,"read file %s\n",msg_filename);
		/*
		*	判断短信文件的实效性如果超过10分钟则不予处理
		*/
		int judge=0;
		time_t sys_t;
		time(&sys_t);
		time_t file_t=get_file_time(msg_filename);
		if(sys_t-file_t>=OVER_TIME)
		{
			judge=1;
		}
		else
		{
			judge=0;
		}
		if(judge==0)
		{
			pool=dhcc_create_pool(4096);
			process_message(tty_fd,msg_filename);
			dhcc_destroy_pool(pool);
		}
		else
		{
			/*将短信文件名字记录到日志中去。*/
			log_warn(log_file,"Delete the long time not processed file %s\n",msg_filename);
		}
			
		/*删除文件*/
		memset(command,0,sizeof(command));
		sprintf(command,"rm -fr %s",msg_filename);
		system(command);
		
		fclose(log_file);
		close(tty_fd);
		return 0;
	
	/*循环读取目录,查看是否有满足条件的短信文件,如果有则发送短信
	*所有需要发送的短信都完成之后删除该文件
	*/	

}

void init_ttyS(int fd)
{
	struct termios options;
	bzero(&options, sizeof(options));       // clear options
	
	cfsetispeed(&options,B9600);            // setup baud rate
	cfsetospeed(&options,B9600);
	
	options.c_cflag |= CS8;				/*设置数据位*/
	options.c_cflag |= CREAD | CLOCAL;	/*本地连接和接受使能*/
	//options.c_cflag &=~CSIZE;			/*这是设置c_cflag选项不按位数据位掩码*/
	
	options.c_cflag &= ~PARENB;			/*设置无校验位*/
	options.c_iflag &= ~INPCK;		
	
	options.c_cflag &= ~CSTOPB;			/*设置停止位为1位*/
	options.c_cflag &=~CRTSCTS;			/*不启用硬件控制*/

	/*options.c_cflag |= CS8;				
	options.c_cflag |= CREAD | CLOCAL;	
	options.c_cflag &= ~PARENB;			
	options.c_iflag &= ~INPCK;		
	options.c_cflag &= ~CSTOPB;		*/	
	
	//将串口设备设置为原始模式
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
	/*因为业务短信都是中文所以将短信设置为pdu模式*/
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
	/*首先判断文件大小，如果超过则清空文件*/
	int fd=fileno(file);
	if(fd<0)
		return;
	struct stat st;
	if(fstat(fd,&st)<0)
		return;
	if(st.st_size>50000000)
		{
		/*清空文件再写*/
		ftruncate(fd, 0);
		rewind(file);
		}
	/*在日志之前添加系统时间*/
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


int GSM_Send_PDUMessage(int fd,const PRST_PARAM* input)
{
	int len;
	len=strlen(input->TP_DA);
	char cmd[32]={0};
	char pdu[1024]={0};
	int retval=0;
	retval=gsm_pdu_encode(input,pdu);
	int tail=0;
	tail=strlen(pdu);
	pdu[tail]=0x1a;
	pdu[tail+1]=0;
	sprintf(cmd,"at+cmgs=%03d\r\0",(retval-32)/2+15);
	int ret=0;
	ret=GSM_GPRS_send_cmd_read_result(fd,cmd,RESPONSE_TIME_WAIT);
	/*if(ret==-1)
		{
		printf("send pdu message failed because at+cmgs failed\n");
		return -1;
		}*/
	ret=GSM_GPRS_send_cmd_read_result(fd, pdu,RESPONSE_TIME_WAIT);
	if(ret==-1){
		log_warn(log_file,"send pdu message failed because pdu code sent failed\n");
		return -1;
		}
	return 0;



	
} // end GSM_Send_Message

int GSM_GPRS_send_cmd_read_result(int fd,const char * const send_buf, int rcv_wait)
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
*	将此函数修改成为
*	返回读取到的数据
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
		log_warn(log_file,"暂时不可写\n");	
		count++;
		usleep(RESPONSE_TIME_WAIT_MULTI);
		if(count>3)return -1;
		continue;
	}
	}
} // end send_GSM_GPRS_cmd

/*该函数的主要作用的将文本文件中第一行的电话号码存储在函数中动态分配的
内存空间中，并且返回对应的phone_t结构体*/
phones_t* phone_num_init(char * input,dhcc_pool_t *pool){
	/*计算电话数量*/
	if(pool==NULL||input==NULL)
		return NULL;
	int phone_count=get_phone_count(input);
	if(phone_count<=0)
		return NULL;
	int mem_size=4+sizeof(phone_t)*phone_count;
	phones_t * phone_info;
	phone_info=(phones_t*)dhcc_pcalloc(pool,mem_size);
	int i=0;
	phone_info->phone_num=phone_count;
	for(i=0;i<phone_count;i++){
		if(get_phone_num(input,i,(phone_t*)&(phone_info->array[i]), phone_count)<0)
		{
			log_warn(log_file,"phone num string:%s  is in illegal format\n",input);
			return NULL;
			}
		//printf("%s\n",phone_info->array[i].num);
		}
	return phone_info;
	
}

/*本函数取出电话号码的个数
*@phone_nums 电话号码字符串使用SEPARATOR分隔
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

/*本函数取出电话号码字符串中的第count个电话号码
*@phone_nums 电话号码字符串使用SEPARATE分隔
*@count 为需要取出的第几个电话号码从1开始
*@num存储电话号码的结构体struct phones
*@max_count表示其为最大的电话数量
*@返回值表示是否正确分析
*/
int get_phone_num(char * phone_nums,int count,phone_t *num,int max_count){
	char tmp[20]={0};
	//需要处理号码格式,如果号码不为86开头则添加86
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
		//如果没有86则添加86在前面
		strncat(tmp,"86",2);
		strncat(tmp,num->num,strlen(num->num));
		memset(num->num,0,sizeof(num->num));
		memcpy(num->num,tmp,strlen(tmp));
	}
	//printf("%s\n",num->num);
	num->len=strlen(num->num);
	return 0;
	
}
/*获取1,2,3,4,5字符串中第count个头的指针1, 取得1的位置
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
}
/*获取1,2,3,4,5字符串中第count个尾巴的指针1,取得,的位置
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
}
/*
*	这里将短信内容改成unicode编码方式。
*	
*/
int ucs2_encode(const char * input,int input_len ,char * output){
	int outlen;//unicode字符长度
	wchar_t w_buf[2048]={0};
	const char *tmp_in=input;
	setlocale(LC_ALL,"zh_CN.GB18030");
	//setlocale(LC_ALL, "en_US");
//	将多字符转换成为宽字符
	outlen=mbstowcs(w_buf,tmp_in,input_len);
	if(outlen<0)
	perror("mbstowcs");
	int i;
	/*按照pdu格式将UNICODE高低字节对调*/
	for(i=0;i<outlen;i++){
		*output++=w_buf[i]>>8;
		*output++=w_buf[i]&0xff;
		}
	return outlen*2;
}
/*
*	将字节数据转化成为可打印字符
*	0xcb 0x78 转换成为"cb78"
*/
int bytes2string(const char * input,char * output,int input_len){
	const char tab[]="0123456789abcdef";
	const char * tmp_in=input;
	char * tmp_out=output;
	int i;
	for(i=0;i<input_len;i++){
	/*输出高4位*/
	*tmp_out++=tab[((*tmp_in&0xff)>>4)&0xff];
	/*输出低4位*/
	*tmp_out++=tab[*tmp_in&0x0f];
	tmp_in++;
	}
	//*tmp_out='\0';
	return input_len*2;
}
/*
*	将可打印字符串转化成为字节数据
*	"cb78"转换成为0xcb 0x78 
*/
int string2bytes(const char * input,char * output,int input_len){
	int i;
	for(i=0;i<input_len;i+=2){
		//输出高4位
		if(*input>='0'&&*input<='9'){
			*output=(*input-'0')<<4;
			}
		else if(*input>='a'&&*input<='z'){
			*output=(*input-'a'+10)<<4;
			}
		else{
			*output=(*input-'A'+10)<<4;
			}
		//输出低4位
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
*	PDU号码和时间变换格式
*	"8613851872468" --> "683158812764F8"
*/
int pdu_invert_numbers(const char * input,char * output,int input_len){
	int output_len;
	char ch;
	output_len=input_len;
	int i;
	const char * tmp_in=input;
	char *tmp_out=output;
	/*俩俩颠倒*/
	for(i=0;i<input_len;i+=2){
	ch=*tmp_in++;
	*tmp_out++=*tmp_in++;
	*tmp_out++=ch;
	}
	/*如果字符串长度是奇数补个F*/
	if(input_len&1){
		*(tmp_out-2)='f';
		output_len++;
	}
	//*output='\0';
	return output_len;
}

/*
*	PDU编码,用于短信的编制和发送
*	函数的功能为将SM_PARAM结构的结构体转换成为PDU格式的字符串
*	注:本函数中不论入参SM_PARAM中的编码方式采用何编码方式实际上一律使用
*	UCS2编码
*/
int gsm_pdu_encode(const PRST_PARAM* input,char * output){
	int len;
	int outlen=0;
	char buf[256]={0};
	char * tmp=output;
	//前三位都为固定写法
	buf[0]=0x00;
	buf[1]=0x11;
	buf[2]=0x00;
	//后面号码的长度
	buf[3]=len=strlen(input->TPA)&0xff;
	//固定写法
	buf[4]=0x91;
	outlen+=bytes2string(buf,tmp+outlen,5);
/*TPA将目标号码转化后放到output中去*/
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
*	从卡中读取短信中心号码
*	本函数将读取SIM卡中的短信中心号码然后将号码以字符串的形式
*	存储到出参output中去。
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
*	本函数处理filename文件,向其中的所有手机号码发送短信
*	文件可能为空(文件格式:第一行文件为手机号码不同的手机号码使用","分隔)
*	下面的行为手机短信内容,内容如果超过160字节则需要拆分为多条进行发送
*/
int process_message(int fd,char * filename){
	FILE *fp =NULL;
	if((fp=fopen(filename,"r"))==NULL){
 		log_warn(log_file,"fopen file %s failed\n",filename);
 		return -1;
		}
	char buf[10240]={0};
	//首先读取第一行获取到短信号码
	if(fgets(buf,1024,fp)==NULL){
		log_warn(log_file,"fgets file %s failed\n",filename);
		return -1;
		}
	int len=strlen(buf);
	//去掉最后的\r和\n符号
	while(len>=1&&(buf[len-1]=='\n'||buf[len-1]=='\r')||buf[len-1]==','){
	buf[len-1]=0;
	len--;
	}
	phones_t * ps_t;
	ps_t=phone_num_init(buf,pool);
	if(ps_t==NULL){
		log_warn(log_file,"process the phone nums:%s failed\n",buf);
		return -1;
		}
	int num_count;
	num_count=ps_t->phone_num;
	//接下来处理短信内容
	PRST_PARAM msg_info;
	memset(&msg_info,0,sizeof(msg_info));
	msg_info.TP_DCS=8;
	msg_info.TP_PID=0;
	//下面填写回复号码,信息内容
	memset(buf,0,sizeof(buf));
	while(fgets(buf,10240,fp)){
		//buf中为短信的内容,现在开始进行短信拆分
		int msg_count;
		msg_count=pducode_count(buf)/MAX_LEN_OF_SHORT_MESSAGE+1;
		//char msg[161]={0};
		//将buf中的;全都替换成\n符号
		int rt_count=substitute(buf);
		log_warn(log_file,"replaced %d count ';'  message content is %s\n",rt_count,buf);
		int i,j,pos=0;
		//先处理号码,即一个一个号码发送短信，优先给一个号码发完所有分段短信
		for(i=0;i<num_count;i++){
			pos=0;
			for(j=0;j<msg_count;j++){
			int n_count=0;
			memset(msg_info.TPA,0,sizeof(msg_info.TPA));
			memset(msg_info.TP_DA,0,sizeof(msg_info.TP_DA));
			memcpy(msg_info.TPA,ps_t->array[i].num,sizeof(msg_info.TPA));
			memcpy(msg_info.TP_DA,buf+pos,MAX_LEN_OF_SHORT_MESSAGE);
			//拷贝的字符串的pdu长度可能超过了最大值。			
			int pdu_count=pducode_count(msg_info.TP_DA);
			if(pdu_count>MAX_LEN_OF_SHORT_MESSAGE)
			{
				int virtual_len=strlen(msg_info.TP_DA);
				int pducut_len=pdu_count-virtual_len;
				char * temp=msg_info.TP_DA;
				int cut_len=pducut_count(temp,pducut_len);
				//将最后多出来的cut_len个字节去掉
				int j;
				for(j=virtual_len-1;j>=virtual_len-cut_len;j--)
				{
					msg_info.TP_DA[j]=0;
				}
			}
			pos+=strlen(msg_info.TP_DA);
			//printf("pos=%d\t content=%s\n",pos,msg_info.TP_DA);
			if(pos>0){
				char * tmp=msg_info.TP_DA;
				int is_first_byte=is_lead_char(tmp,strlen(msg_info.TP_DA));
				//确保不将两字节的汉字从中间将两个字节分开。
				if(is_first_byte==0)
				{
				msg_info.TP_DA[strlen(msg_info.TP_DA)>0?strlen(msg_info.TP_DA)-1:0]=0;
				//说明其最后一个字节是下汉字的首字节将pos--并且将msg_info.TP_DA中的最后一位置0
				pos--;
				}
				}
			int retal=0;
			//show_msg_info(&msg_info);
send:
			retal=GSM_Send_PDUMessage(fd,&msg_info);//发短信
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
	//中国为东8区
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
*	@本函数将pInput中的;替换为\n
*	@pInput 为需要修改的字符串
*	@出参表示替换了多少个对应的字符为回车键
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
*	本函数功能为计算字符串在进行pdu编码后的长度(注:一个asc码在pdu中文
*	模式下也占用两个字节)
*	@pInput 为入参
*	@返回值为对应输入字符串的pdu编码长度
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
*	本函数功能为从字符串尾部开始计算实际需要清除的pdu长度转化成为对应字符串长度
*	@pInput 需要处理的字符串指针
*	@pducut_len为需要cut掉的pdu编码长度
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
