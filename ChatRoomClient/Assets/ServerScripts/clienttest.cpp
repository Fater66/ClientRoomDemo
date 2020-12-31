#include <iostream>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT 8111
#define MESSAGE_LEN 1024
int main(int argc,char* argv[])
{
	int socket_fd;
	int ret;
	struct sockaddr_in serveraddr;
	char sendbuf[MESSAGE_LEN] = {0,};
	char recvbuf[MESSAGE_LEN] = {0,};
	// 创建socket 
	socket_fd = socket(AF_INET,SOCK_STREAM,0);
	std::cout<<"socket number = "<< socket_fd <<std::endl;
	if(socket_fd <0)
	{
		std::cout<<"Failed to creat socket!"<<std::endl;
	}

	serveraddr.sin_family = AF_INET;//ipv4
	serveraddr.sin_port = PORT;
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//连接
	ret = connect(socket_fd,(struct sockaddr *)&serveraddr,sizeof(struct sockaddr));
	if(ret < 0)
	{
		std::cout<<"Failed to connect server! "<<std::endl;
		exit(-1);
	}
	std::cout<<"enter input mode"<<std::endl;
	while(1){
		memset(sendbuf, 0,MESSAGE_LEN);
		//从标准设备读取输入
		gets(sendbuf);
		ret = send(socket_fd,sendbuf,strlen(sendbuf),0);
		std::cout<<"ret = "<<ret <<std::endl;
		std::cout<<"sendbuf = "<<sendbuf <<std::endl;
		if(ret <= 0)
		{
			std::cout<<"Failed to send data! "<<std::endl;
			break;
		}
		if(strcmp(sendbuf,"quit") == 0)
		{
			std::cout<<"received quit signal"<<std::endl;
			break;
		}
		ret = recv(socket_fd,recvbuf,MESSAGE_LEN,0);
		std::cout<<"recvbuf = "<<recvbuf <<std::endl;
		// recvbuf[ret] = '\0';
		// std::cout<<"recv"<<recvbuf <<std::endl;
		
	}
	close(socket_fd);

}
