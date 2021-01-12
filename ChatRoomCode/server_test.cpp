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
#define MAX_USER 100
#define MAX_USER_SAME_TIME 20

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
	int curUID;
};

struct UserInfo
{
	int roomInfo[MAX_USER];
	char name[MAX_USER][16];
};

// 打开句柄上限3个 所以合在一起 同时方便操作
struct SharedMem
{
	struct Packet packet;
	struct RoomCB roomCB;
	// struct HeartInfo heartInfo;
	struct UserInfo userInfo;
	// 记录连接的客户端<fd,写pid,UID, count,读pid>
	// 记录连接的客户端<UID,写pid,读pid, count>
	int heartMap[MAX_USER_SAME_TIME][4];
};

static void sleep_ms(unsigned int secs)
{
	struct timeval tval;
	tval.tv_sec = secs / 1000;
	tval.tv_usec = (secs * 1000) % 1000000;
	select(0, NULL, NULL, NULL, &tval);
}

void *heart_handler(void *arg)
{
	// cout << "心跳检测进程启动id" << (int)getpid() << endl;
	struct SharedMem *sharedMem = (struct SharedMem *)arg;
	struct RoomCB *roomCB = &sharedMem->roomCB;
	while (1)
	{
		// cout << "heartMap size:" << sharedMem -> heartMap.size() << endl;
		for (int i = 0; i < MAX_USER_SAME_TIME; i++)
		{
			if (sharedMem->heartMap[i][0] == 0)
			{
				// 当前UID = 0，没有连接
				continue;
			}
			if (sharedMem->heartMap[i][3] >= 0 && sharedMem->heartMap[i][3] < 3)
			{
				sharedMem->heartMap[i][3]++;
				// cout << "当前UID" << sharedMem -> heartMap[i][0] << ",UID" << sharedMem -> heartMap[i][2] << ",timer:" << sharedMem -> heartMap[i][3] << endl;
			}
			else if (sharedMem->heartMap[i][3] == 3)
			{
				// close(sharedMem -> heartMap[i][0]);
				kill(sharedMem->heartMap[i][1], SIGUSR1);
				kill(sharedMem->heartMap[i][2], SIGUSR2);
				// cout << "kill 进程:" << sharedMem -> heartMap[i][1] << endl;
				for (int j = 0; j < sizeof(roomCB->roomOwner) / sizeof(roomCB->roomOwner[1]); j++)
				{
					if (roomCB->roomOwner[j] == sharedMem->heartMap[i][0])
					{
						roomCB->roomOwner[j] = 0;
						roomCB->isOpen[j] = false;
						cout << "房间" << j << "重置" << endl;
					}
				}
				cout << "心跳检测超时，当前i:" << i << ",UID" << sharedMem->heartMap[i][0] << ".timer:" << sharedMem->heartMap[i][3] << endl;
				//重置heartMap
				sharedMem->heartMap[i][0] = 0;
				sharedMem->heartMap[i][1] = 0;
				sharedMem->heartMap[i][2] = 0;
				sharedMem->heartMap[i][3] = 0;
			}
		}
		sleep(3); // 定时三秒
	}
}

void PrintPacket(bool isSend, Packet *pack)
{
	if (isSend)
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
	cout << "||mode:" << pack->mode;
	cout << "||roomId:" << pack->roomId;
	cout << "||name:" << pack->name;
	cout << "||content:" << pack->content;
	cout << endl;
}
void func(int flag)
{
	cout << "write进程退出" << endl;
	exit(0);
}

void func2(int flag)
{
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
			cout << "socket连接中断" << endl;
			// exit(0);
			return 0;
		}
		nleft -= nread;
		// printf("bufp:%c\n",*bufp);
		bufp += nread;
	}
	return count;
}

int find_single_connect(int heartMap[MAX_USER_SAME_TIME][4], int target)
{
	for (int i = 0; i < MAX_USER_SAME_TIME; i++)
	{
		if (heartMap[i][0] == target)
			return i;
	}
	return -1;
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

	pthread_t heartId; // 创建心跳检测线程
	int ret = pthread_create(&heartId, NULL, heart_handler, sharedMem);

	if (ret != 0)
	{
		cout << "无法创建心跳检测线程" << endl;
	}

	int UID = 1;
	while (1)
	{
		if ((connectfd = accept(listenfd, NULL, NULL)) == -1)
			cout << "accept错误" << endl;
		else
			cout << "连接成功" << endl;
		// 第一次fork，保证不断监听
		pid = fork();
		if (pid == 0)
		{
			close(listenfd);
			pid = fork();
			// 第二次fork，创建读写进程
			if (pid > 0)
			{
				signal(SIGUSR2, func2);
				sharedMem = (struct SharedMem *)shmat(shmid, (void *)0, 0);
				if (sharedMem == (void *)-1)
					cout << "shm shmat失败" << endl;
				// vector<int> userinfo(4,0);
				// userinfo[0] = connectfd;
				// userinfo[1] = pid;
				// userinfo[2] = UID;
				int avaLoc = find_single_connect(sharedMem->heartMap, 0);
				if (avaLoc == -1)
				{
					cout << "连接已满，需要排队" << endl;
					return 0;
				}
				cout << "avaLoc = " << avaLoc << endl;
				sharedMem->heartMap[avaLoc][0] = UID;
				sharedMem->heartMap[avaLoc][1] = pid;
				sharedMem->heartMap[avaLoc][2] = (int)getpid();
				// sharedMem -> heartMap.insert(make_pair(connectfd,userinfo));
				// cout << " heartMap avaloc:" << avaLoc << ",connectfd" << connectfd << ",pid" << pid << ",UID" << UID <<endl;
				struct Packet *pack = &sharedMem->packet;

				// cout << "此连接connectfd" << connectfd << ",读进程pid" << (int)getpid() << "UID:" << UID <<endl;
				while (1)
				{
					// 注意此处要用Packet大小，sizeof(pack)是指针大小=8字节
					num = readn(connectfd, pack, sizeof(Packet));
					if (num == 0)
					{
						cout << "当前socket已关闭,read进程退出" << endl;
						//关闭write进程
						kill(pid, SIGUSR1);
						exit(0);
					}
				}
			}
			if (pid == 0) //子进程，负责发送数据
			{
				signal(SIGUSR1, func);
				int id_tmp = -1;
				int roomId_cur = -1;

				// cout << "此连接connectfd" << connectfd << ",写进程pid" << (int)getpid() << "UID:" << UID <<endl;
				sharedMem = (struct SharedMem *)shmat(shmid, (void *)0, 0);
				if (sharedMem == (void *)-1)
					cout << "shm shmat失败" << endl;
				struct Packet *pack = &sharedMem->packet;
				struct RoomCB *roomCB = &sharedMem->roomCB;
				// struct HeartInfo *heartInfo = &sharedMem -> heartInfo;
				struct UserInfo *userInfo = &sharedMem->userInfo;

				// 发送UID设置报文
				pack->mode = 1;
				pack->userId = UID;
				int zz = write(connectfd, pack, sizeof(Packet));
				if (zz <= 0)
					cout << "************write 失败 ***" << endl;
				cout << "发送UID设置报文,当前UID:" << UID << endl;
				while (1)
				{
					while (1)
					{
						sleep_ms(200); // 200ms
						// 根据id判断是否是已发送报文
						// PrintPacket(false,pack);
						if (pack->id == id_tmp)
							continue;
						// 消息报文
						// 1.判断是否是当前房间的报文
						if (pack->mode == 0 && pack->roomId == roomId_cur && roomId_cur != -1)
						{
							// 收到当前报文，break进入转发流程
							cout << "收到消息报文" << endl;
							break;
						}
						// 退出房间报文
						// 1.判断是否是当前socket的用户发出
						else if (pack->mode == 2 && pack->userId == UID)
						{
							PrintPacket(false, pack);
							break;
						}
						else if (pack->mode == 3 && pack->userId == UID)
						{
							PrintPacket(false, pack);
							break;
						}
						else if (pack->mode == 4 && pack->roomId != roomId_cur && pack->userId == UID)
						{
							PrintPacket(false, pack);
							break;
						}
						else if (pack->mode == 6)
						{
						}
						else if (pack->mode == 7 && pack->userId == UID)
						// else if(pack -> mode == 7)
						{
							cout << "收到心跳报文" << endl;
							break;
						}
						else if (pack->mode == 9 && pack->userId == UID)
						{
							PrintPacket(false, pack);
							break;
						}
					}
					if (pack->mode == 0)
					{
					}
					// 2:退出房间
					else if (pack->mode == 2)
					{
						// 首先判断当前房间是否是房主，如果是 关闭房间
						if (roomCB->roomOwner[roomId_cur] == pack->userId)
						{
							roomCB->roomOwner[roomId_cur] = 0;
							roomCB->isOpen[roomId_cur] = false;
							roomCB->UserCount[roomId_cur] = 0;
						}
						else
						{
							roomCB->UserCount[roomId_cur]--;
						}
						roomId_cur = -1;
						userInfo->roomInfo[pack->userId] = -1;
						pack->mode = 5;
						strcpy(pack->content, "退出房间");
					}
					else if (pack->mode == 3)
					{
						// 判断是否有人开了房间
						if (roomCB->roomOwner[pack->roomId] == 0)
						{
							roomCB->roomOwner[pack->roomId] = pack->userId;
							roomCB->isOpen[pack->roomId] = true;
							roomId_cur = pack->roomId;
							userInfo->roomInfo[pack->userId] = pack->roomId;
							strcpy(pack->content, "房间创建成功");
						}
						else
						{
							strcpy(pack->content, "房间已被占用");
						}
					}
					else if (pack->mode == 4)
					{
						if (roomCB->roomOwner[pack->roomId] == 0)
						{
							strcpy(pack->content, "房间不存在");
						}
						else
						{
							if (roomCB->isOpen[pack->roomId])
							{
								roomId_cur = pack->roomId;
								userInfo->roomInfo[pack->userId] = roomId_cur;
								// cout << "更改后的roomId_tmp" << roomId_cur << endl;
								// cout << "UID" << UID << endl;
								strcpy(pack->content, "进入房间");
							}
							else
							{
								strcpy(pack->content, "房间已锁");
							}
						}
					}
					else if (pack->mode == 7)
					{
						int mapLoc = find_single_connect(sharedMem->heartMap, UID);
						sharedMem->heartMap[mapLoc][3] = 0; // 重置定时器
						// cout << "client" << connectfd << "UID" << sharedMem -> heartMap[mapLoc][2] << endl;
						pack->mode = 8;
						strcpy(pack->content, "收到心跳报文");
						cout << "发送心跳确认报文至" << sharedMem->heartMap[mapLoc][0] << endl;
					}
					else if (pack->mode == 9)
					{
						cout << "收到重连报文" << endl;
						pack->mode = 10;
						int pre_userId = stoi(pack->content);
						// 如果原有房间已被关闭
						if (roomCB->isOpen[userInfo->roomInfo[pre_userId]] == false)
						{
							pack->roomId = -1;
						}
						else
						{
							pack->roomId = userInfo->roomInfo[pre_userId];
						}

						strcpy(pack->name, userInfo->name[pre_userId]);
						// strcpy(pack -> content,"重连成功");
					}
					// cout << "拷贝前pack id" << pack -> id << endl;
					// struct Packet sendPacket= *pack;
					// sendPacket.id ++;
					// cout << "拷贝后pack id" << pack -> id << endl;
					// cout << "靠背后sendPack id" << sendPacket.id << endl;
					pack->id++;
					id_tmp = pack->id;
					strcpy(userInfo->name[pack->userId], pack->name);
					int zz = write(connectfd, pack, sizeof(Packet));
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
