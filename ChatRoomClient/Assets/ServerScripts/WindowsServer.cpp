typedef struct
{
	int messageID;
	int clientID;
	char message[200];
}UserMsg;

void  talkWithClient(void *p)
{
	const char *str = "hello client";
	UserMsg u;
	SOCKET *pSock = (SOCKET*)p;
	SOCKET socka = *pSock;
	/*byte *b;
	b = (BYTE*)str;
	int i = send(socka, str, strlen(str), 0);*/
	char buff[1024];
	sockaddr_in sa = { AF_INET };
	int i;
	while (true)
	{
        //接收客户端消息
		i = recv(socka, buff, sizeof(buff), 0);
 
        //如果收到消息长度小于等于0说明客户端断开
		if (i <= 0)
		{
			if (GetLastError() == 10054)
				cout << htons(sa.sin_port) << "客户端退出了:" << socka << endl;
			break;
		}
		buff[i] = 0;
		
        //byte数组转化为结构体
		u = *(UserMsg*)buff;
		
		cout << u.message << endl;
		
        //回执信息
		int k = 0, j = 0;
		char p[20] = "hello client";
		while (p[k -1] != '\0')
			u.message[j++] = p[k++];
        //发送回执信息
		int i = send(socka, (char*)(&u), sizeof(UserMsg), 0);
	}
}

int main()
{
	WSAData wd;
	WSAStartup(0x0202, &wd);
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in sa = { AF_INET };
	sa.sin_port = htons(PORT);
	bind(sock, (sockaddr*)&sa, sizeof(sa));
	listen(sock, 5);
	sockaddr_in from = { AF_INET };
	int nLen = sizeof(from);
	while (true) {
		SOCKET socka = accept(sock, (sockaddr*)&from, &nLen);
		cout << inet_ntoa(sa.sin_addr) << htons(from.sin_port) << "登录了" << endl;
		_beginthread(talkWithClient, 0, &socka); //开启一个线程 talkWithClient 参数为&socka
	}
	return 0;
}



