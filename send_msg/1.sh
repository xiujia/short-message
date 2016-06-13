gcc -o sms_server send_msg.c op_db.c sms_srv.c  -lmysqlclient -lpthread
gcc -o sms_client sms_cli.c 
