#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/shm.h>
#include <unistd.h>
using namespace std;

struct packet
{
	ushort len;
	char name[16];		//4+16->20
	char content[20];
};


static void sleep_ms(unsigned int secs)

{

    struct timeval tval;

    tval.tv_sec=secs/1000;

    tval.tv_usec=(secs*1000)%1000000;

    select(0,NULL,NULL,NULL,&tval);

}

int main()
{
	int shmid = shmget((key_t)1234, sizeof(int), 0666 | IPC_CREAT);
	if (shmid == -1)
	{
		cout << "共享内存创建失败" << endl;
		exit(0);
	}
	int *userId = (int *)shmat(shmid, (void *)0, 0);
	pid_t pid;
	pid = fork();
	if(pid == 0)
	{
		
		cout << "子进程" << *userId <<endl;
	}
	else
	{
		(*userId) ++;
		cout << "父进程" << *userId <<endl;
	}
}