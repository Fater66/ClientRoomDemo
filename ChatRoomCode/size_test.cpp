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
#include <pthread.h>

using namespace std;

struct packet
{
	ushort len;
	char name[16];		//4+16->20
	char content[20];
};

struct HeartInfo
{
	pid_t pid;
	int timer;
};


static void sleep_ms(unsigned int secs)

{

    struct timeval tval;

    tval.tv_sec=secs/1000;

    tval.tv_usec=(secs*1000)%1000000;

    select(0,NULL,NULL,NULL,&tval);

}

void* heart_handler(void* arg)
{
    cout << "心跳检测线程启动" << endl;
	HeartInfo* heartInfo = (struct HeartInfo*) arg;
    while(1)
    {
        if(heartInfo -> timer >= 0 && heartInfo -> timer < 3)
		{
			(heartInfo -> timer)++;
			cout << "当前心跳次数" << heartInfo -> timer << endl;
		}
		else
		{
			kill(heartInfo -> pid, SIGUSR1); // 用户自定义信号 默认处理:进程终止
			cout << "心跳检测超时，当前timer:" << heartInfo -> timer << endl;
		}
        sleep(3);   // 定时三秒
    }
}


int main()
{
	int shmid = shmget((key_t)4444, sizeof(int[100]), 0666 | IPC_CREAT);
	if (shmid == -1)
	{
		cout << "共享内存创建失败" << endl;
		exit(0);
	}
	int* rooms = (int *)shmat(shmid, (void *)0, 0);

	pid_t pid;
	pid = fork();
	if(pid == 0)
	{
		pthread_t heartId;     // 创建心跳检测线程
		struct HeartInfo heartInfo;
		heartInfo.pid = pid;
		heartInfo.timer = 0;
		int ret = pthread_create(&heartId, NULL, heart_handler, &heartInfo);
		cout << "helloworld" <<endl;
		if(ret != 0)
		{
			cout << "无法创建心跳检测线程" << endl;
		}
		else{
			cout << ret << endl;
		}
		cout << "子进程" << endl;
		while (1)
		{
			/* code */
		}
		
	}
	else
	{
		cout << "父进程" <<endl;
	}
}