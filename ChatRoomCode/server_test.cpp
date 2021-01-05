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

#define BUFMAX 100
#define FILENAME "share.txt"

using namespace std;

struct Packet
{
	int userId;
	int id;
	// 0: 消息 1:设置userid 2:退出房间 3.开房间  4.进入房间 5.确认退出房间 6.锁房间 7.心跳报文 8.确认心跳
	int mode;
	int len;

	int roomId;
	char name[16]; 
	char content[BUFMAX];
};

// 房间控制块 RoomControlBlock
struct RoomCB
{
	int roomOwner[100];
	bool isOpen[100];
	int UserCount[100];
};

struct HeartInfo
{
	pid_t pid;
	int timer;
	int connectfd;
};

// 打开句柄上限3个 所以合在一起 同时方便操作
struct SharedMem
{
	struct Packet packet;
	struct RoomCB roomCB;
	struct HeartInfo heartInfo;
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
			close(heartInfo -> connectfd);
			kill(heartInfo -> pid, SIGUSR1); // 用户自定义信号 默认处理:进程终止
			cout << "心跳检测超时，当前timer:" << heartInfo -> timer << endl;
			exit(0);
		}
        sleep(3);   // 定时三秒
    }
}

void PrintPacket(bool isSend, Packet* pack)
{
	if(isSend)
	{
		cout << "发送";
	}
	else
	{
		cout << "收到";
	}
	cout << "||userId:" << pack->userId;
	cout << "||id:" << pack->id;
	cout << "||len:" << pack->len;
	cout << "||mode:" << pack -> mode;
	cout << "||roomId:" << pack->roomId;
	cout << "||name:" << pack->name;
	cout << "||content:" << pack->content;
	cout << endl;
}
void func(int flag)
{
	cout << "write进程退出" << endl;
	cout << "read进程退出" << endl;
	exit(0);
}

void test(pid_t pid, int num)
{
	if (num <= 0)
	{
		if (num == 0)
		{
			cout << "read进程退出" << endl;
			kill(pid, SIGUSR1); // 用户自定义信号 默认处理:进程终止
			exit(0);
		}
		else
		{
			cout << "num<0错误" << endl;
			exit(0);
		}
	}
}

ssize_t readn(int fd, void *content, size_t count)
{
	ssize_t nleft = count;
	ssize_t nread;
	char *bufp = (char *)content;

	while (nleft > 0)
	{
		if ((nread = read(fd, bufp, nleft)) <= 0)
		{
			// cout << "连接中断" << endl;
			return 0;
		}
		nleft -= nread;
		// printf("bufp:%c\n",*bufp);
		bufp += nread;
	}
	return count;
}

int main()
{
	cout << "运行程序" << endl;
	FILE *fp;
	fp = fopen(FILENAME, "rb");
	if (fp == NULL)
		fp = fopen(FILENAME, "wb");
	fclose(fp);

	int shmid;
	void *shm;
	struct SharedMem *sharedMem;
	shmid = shmget((key_t)1234, sizeof(struct SharedMem), 0666 | IPC_CREAT);
	if (shmid == -1)
	{
		cout << "shm共享内存创建失败" << endl;
		exit(0);
	}
	shm = shmat(shmid, (void *)0, 0);
	memset(shm, 0, sizeof(SharedMem));
	if (shm != (void *)-1)
		cout << "shm共享内存格式化成功" << endl;
	sharedMem = (struct SharedMem *)shmat(shmid, (void *)0, 0);
	if (sharedMem == (void *)-1)
		cout << "shm shmat失败" << endl;
	struct Packet *pack;
	struct RoomCB *roomCB;
	// int shmid;
	// void *shm;
	// struct Packet *pack;
	// shmid = shmget((key_t)1234, sizeof(struct Packet), 0666 | IPC_CREAT);
	// if (shmid == -1)
	// {
	// 	cout << "pack共享内存创建失败" << endl;
	// 	exit(0);
	// }
	// shm = shmat(shmid, (void *)0, 0);
	// memset(shm, 0, sizeof(Packet));
	// if (shm != (void *)-1)
	// 	cout << "pack共享内存格式化成功" << endl;
	// pack = (struct Packet *)shmat(shmid, (void *)0, 0);
	// if (pack == (void *)-1)
	// 	cout << "pack shmat失败" << endl;

	// int shm_RoomCBid;
	// void *shm_RoomCB;
	// struct RoomCB *roomCB;
	// shm_RoomCBid = shmget((key_t)4321, sizeof(struct RoomCB), 0666 | IPC_CREAT);
	// if (shm_RoomCBid == -1)
	// {
	// 	cout << "RoomCB共享内存创建失败" << endl;
	// 	exit(0);
	// }
	// shm_RoomCB = shmat(shm_RoomCBid, (void *)0, 0);
	// memset(shm_RoomCB, 0, sizeof(RoomCB));
	// if (shm_RoomCB != (void *)-1)
	// 	cout << "RoomCB共享内存格式化成功" << endl;
	// roomCB = (struct RoomCB *)shmat(shm_RoomCBid, (void *)0, 0);
	// if (roomCB == (void *)-1)
	// 	cout << "roomCB shmat失败" << endl;

	// int shm_HeartInfoid;
	// void *shm_HeartInfo;
	// struct HeartInfo *heartInfo;
	// shm_HeartInfoid = shmget((key_t)1111, sizeof(struct HeartInfo), 0666 | IPC_CREAT);
	// if (shm_HeartInfoid == -1)
	// {
	// 	cout << "HeartInfo共享内存创建失败" << endl;
	// 	exit(0);
	// }
	// shm_HeartInfo = shmat(shm_HeartInfoid, (void *)0, 0);
	// memset(shm_HeartInfo, 0, sizeof(HeartInfo));
	// if (shm_HeartInfo != (void *)-1)
	// 	cout << "HeartInfo共享内存格式化成功" << endl;
	// heartInfo = (struct HeartInfo *)shmat(shm_HeartInfoid, (void *)0, 0);
	// if (heartInfo == (void *)-1)
	// 	cout << "heartInfo shmat失败" << endl;

	char objname[16] = {0};
	int num, nlen;
	pid_t pid;
	int listenfd, connectfd;
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(8888);
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		cout << "socket错误" << endl;
	if ((bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) == -1)
		cout << "bind错误" << endl;
	if (listen(listenfd, 10) == -1)
		cout << "listen 错误" << endl;

	int UID = 1;
	while (1)
	{
		if ((connectfd = accept(listenfd, NULL, NULL)) == -1)
			cout << "accept错误" << endl;
		else
			cout << "连接成功" << endl;
		// 第一次fork，保证不断监听
		pid = fork();
		if(pid == 0)
		{
			close(listenfd);
			pid = fork();
			// 第二次fork，创建读写进程
			if (pid > 0)
			{
				sharedMem = (struct SharedMem *)shmat(shmid, (void *)0, 0);
				if (sharedMem == (void *)-1)
					cout << "shm shmat失败" << endl;
				struct Packet *pack= &sharedMem -> packet;

				struct HeartInfo *heartInfo = &sharedMem -> heartInfo;
				heartInfo -> pid = pid;
				heartInfo -> timer = 0;
				heartInfo -> connectfd = connectfd;
				pthread_t heartId;     // 创建心跳检测线程
				int ret = pthread_create(&heartId, NULL, heart_handler, heartInfo);
				if(ret != 0)
				{
					cout << "无法创建心跳检测线程" << endl;
				}
				

				while (1)
				{
					// 读取pack len
					// 注意此处要用Packet大小，sizeof(pack)是指针大小=8字节
					num = readn(connectfd, pack, sizeof(Packet));
					// 判断是否还有数据
					// test(pid,num);
					// if (num <= 0)
					// 	return 0;
				}
			}
			if (pid == 0) //子进程，负责发送数据
			{
				signal(SIGUSR1, func);
				int id_tmp = -1;
				int roomId_cur = -1;

				sharedMem = (struct SharedMem *)shmat(shmid, (void *)0, 0);
				if (sharedMem == (void *)-1)
					cout << "shm shmat失败" << endl;
				struct Packet *pack= &sharedMem -> packet;
				struct RoomCB *roomCB = &sharedMem -> roomCB;
				struct HeartInfo *heartInfo = &sharedMem -> heartInfo;
				// cout << &sharedMem -> packet << endl;
				// pack = (struct Packet *)shmat(shmid, (void *)0, 0);
				// if (pack == (void *)-1)
				// 	cout << "shmat失败" << endl;

				
				// heartInfo = (struct HeartInfo *)shmat(shm_HeartInfoid, (void *)0, 0);
				// if (heartInfo == (void *)-1)
				// {
				// 	perror("shmat");
					// cout << "heartInfo shmat失败" << endl;
				// }
					
				
				// 发送UID设置报文
				pack -> mode = 1;
				cout << pack -> mode << endl;
				pack -> userId = UID;
				int zz = write(connectfd,pack,sizeof(Packet));
				if (zz <= 0)
					cout << "************write 失败 ***" << endl;
					
				while (1)
				{
					while (1)
					{
						sleep_ms(200); // 200ms
						// 根据id判断是否是已发送报文
						if(pack->id == id_tmp)
							continue;
						// 消息报文 
						// 1.判断是否是当前房间的报文
						if (pack -> mode == 0 && pack -> roomId == roomId_cur  && roomId_cur != -1)
						{
							// 收到当前报文，break进入转发流程
							cout << "收到消息报文" << endl;
							break;
						}
						// 退出房间报文 
						// 1.判断是否是当前socket的用户发出
						else if(pack -> mode == 2 && pack -> userId == UID)
						{
							PrintPacket(false,pack);
							break;
						}
						else if(pack -> mode == 3  && pack -> userId == UID)
						{
							PrintPacket(false,pack);
							break;
						}
						else if(pack -> mode == 4 && pack -> roomId != roomId_cur && pack -> userId == UID)
						{
							PrintPacket(false,pack);
							break;
						}
						else if(pack -> mode == 6)
						{

						}
						else if(pack -> mode == 7 && pack -> userId == UID)
						{
							//重置心跳报文
							heartInfo -> timer = 0;
							cout << "收到心跳报文" << endl;
							break;
						}
					}
					id_tmp = pack -> id;
					// 模拟tcp ack机制
					pack -> id ++;
					if(pack -> mode == 0)
					{

					}
					// 2:退出房间
					else if(pack -> mode == 2)
					{
						// 首先判断当前房间是否是房主，如果是 关闭房间
						if(roomCB -> roomOwner[roomId_cur] == pack -> userId)
						{
							roomCB -> roomOwner[roomId_cur] = 0;
							roomCB -> isOpen[roomId_cur] = false;
							roomCB -> UserCount[roomId_cur] = 0;
							
						}
						else
						{
							roomCB -> UserCount [roomId_cur] --;
						}
						roomId_cur = -1;
						pack -> mode = 5;
						strcpy(pack -> content,"退出房间");
					}
					else if(pack -> mode == 3)
					{
						// 判断是否有人开了房间
						if(roomCB -> roomOwner[pack -> roomId] == 0)
						{
							roomCB -> roomOwner[pack -> roomId] = pack -> userId;
							roomCB -> isOpen[pack -> roomId] = true;
							roomId_cur = pack -> roomId;
							strcpy(pack -> content,"房间创建成功");
						}
						else
						{
							strcpy(pack -> content,"房间已被占用");
						}
					}
					else if(pack -> mode == 4)
					{
						if(roomCB -> roomOwner[pack -> roomId] == 0)
						{
							strcpy(pack -> content,"房间不存在");
						}
						else
						{
							if(roomCB -> isOpen[pack -> roomId])
							{
								roomId_cur = pack -> roomId;
								// cout << "更改后的roomId_tmp" << roomId_cur << endl;
								// cout << "UID" << UID << endl;
								strcpy(pack -> content,"进入房间");
							}
							else
							{
								strcpy(pack -> content,"房间已锁");
							}
						}
					}
					else if(pack -> mode == 7)
					{
						pack -> mode = 8;
						strcpy(pack -> content,"收到心跳报文");
					}
					int zz = write(connectfd,pack,sizeof(Packet));
					if (zz <= 0)
						cout << "************write 失败 ***" << endl;
				}
			}
		}
		else if (pid > 1) //父进程 继续监听
		{
			UID++;
			close(connectfd);
		}
		else
		{
			cout << "1pid错误" << endl;
			exit(0);
		}
	}
	return 0;
}
