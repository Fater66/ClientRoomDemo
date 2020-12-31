#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 8111
#define MESSAGE_LEN 1024
int main(int argc,char* argv[])
{
	int ret = -1;
	int socket_fd,accept_fd;
	int on =1;
	int backlog = 10;
	struct sockaddr_in localaddr,remoteaddr;
	
	char in_buff[MESSAGE_LEN] = {0,};
	socket_fd = socket(AF_INET,SOCK_STREAM,0);
	if(socket_fd ==-1){
		std::cout<<"Failed to create socket!"<<std::endl;
		exit(-1);
	}

     ret = setsockopt(socket_fd, SOL_SOCKET,
         SO_REUSEADDR,
         &on,
         sizeof(on));

	if(ret == -1){
		std::cout<<"Failed to set socket options!"<<std::endl;
	}

	localaddr.sin_family = AF_INET;
	localaddr.sin_port = PORT;
    localaddr.sin_addr.s_addr = INADDR_ANY;
	ret = bind(socket_fd,
         (struct sockaddr *)&localaddr,
         sizeof(struct sockaddr));
	if(ret == -1)
	{
		std::cout<<"Failed to bind addr!"<<std::endl;
		exit(-1);
	}
	
	ret = listen(socket_fd,backlog);
	if(ret == -1)
	{
		std::cout<<"Failed to listen socket!" <<std::endl;
		exit(-1);
	}
	for(;;)
	{
		socklen_t addr_len = sizeof(struct sockaddr);
		accept_fd = accept(socket_fd,(struct sockaddr *)&remoteaddr,&addr_len);
		for(;;)
		{
			ret = recv(accept_fd, (void *)in_buff, MESSAGE_LEN,0);
			if(ret == 0){
			break;}

			std::cout<<"recv:"<<in_buff<<std::endl;
			send(accept_fd,(void*)in_buff,MESSAGE_LEN,0);
		}

		close(accept_fd);
	}

	close(socket_fd);

	return 0;
}
