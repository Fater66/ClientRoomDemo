#include<iostream>
#include<stdio.h>
#include<cstdlib>
#include <unistd.h>
#include<cstring>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<signal.h>
#include<sys/shm.h>
#include<unistd.h>
#define BUFMAX 200
#define FILENAME "share.txt"
 
 using namespace std;

struct packet
{
	ushort len;
	char name[16];		//4+16->20
	char content[BUFMAX];
};
 
void func(int flag)
{
	cout<<"write进程退出"<<endl;
	exit(0);
}
 
void test(pid_t pid,int num)
{
	if(num<=0)
    {
		  if(num==0)
		 {
   			     cout<<"read进程退出"<<endl;
				 kill(pid,SIGUSR1); // 用户自定义信号 默认处理:进程终止
			     exit(0);
		 }
		   else
		   {
			    cout<<"num<0错误"<<endl;
				 exit(0);
		   }
	}
 
}
 
 
 ssize_t readn(int fd, void  *content, size_t count)
	{
		ssize_t nleft=count;
		ssize_t nread;
		char *bufp=(char *)content;
	
		while(nleft>0)
		{
			if( (nread=read(fd,bufp,nleft))<=0 )
			{
					cout<<"连接中断"<<endl;
					return 0;
			}
			nleft-=nread;
			bufp+=nread;
 
		}
		return count;
	}
 
 
 
ssize_t writen(int fd, void *content, size_t count)
        {
                ssize_t nleft=count;
                ssize_t nwrite;
                char *bufp=(char *)content;
 
                while(nleft>0)
                {
                        nwrite=write(fd,bufp,nleft);
                        nleft-=nwrite;
                        bufp+=nwrite;
                }
                return count;
        }
 
 
void root_usr(int connectfd)  // root 用户
{
	int ret,len;
	packet pack;
	memset(pack.name,0,16);
	while(1)
	{
	strcpy(pack.content,
	"**********欢迎 Root 管理员**********\n\n1 . 查看聊天记录\n 2 . 删除聊天记录\n 乱按 -》 退出  \n");
	len=strlen(pack.content)+1;
	pack.len=htonl(len);
	writen(connectfd,&pack,len+20);
		ret=read(connectfd,&pack,4);
		if(ret<=0)
		return;
		len=ntohl(pack.len);
		ret=read(connectfd,&pack.name,len+16);
		if(ret<=0)
		return;
        if( strlen(pack.content)!=1 )
		{cout<<pack.content<<" ***"<<strlen(pack.content)<<endl; return ;}
 
		switch(pack.content[0])
		{
			case  '1' :
				   {
					 FILE *fp=fopen(FILENAME,"rb");
					 fseek(fp,0,SEEK_END);
					 int size=ftell(fp);
					 fseek(fp,0,SEEK_SET);
				         char * data=new char[size];
					 int ret=fread(data,size,1,fp);
					 if(ret!=1)
					  cout<<"size  不等于  ret"<<endl;
 
					 strcpy(pack.content,"*********消息纪录******\n\n");
					 len=strlen(pack.content)+1;
					 pack.len=htonl(len);
					 writen(connectfd,&pack,len+20);
					 writen(connectfd,data,size);
				     strcpy(pack.content,"***********完***********\n");
                                         len=strlen(pack.content)+1;
					  pack.len=htonl(len);
					 writen(connectfd,&pack,len+20);
					 delete [] data;
 
				   }break;
 
			case '2':
				   {
				   		FILE *fp=fopen(FILENAME,"w");
						fclose(fp);
				   }
			default :
				   return ;
 
		}
 
	}
 
}

int main()
{
	cout<<"运行程序"<<endl;
	FILE *fp;
	fp=fopen(FILENAME,"rb");
	if(fp==NULL)
		fp=fopen(FILENAME,"wb");
	fclose(fp);
 
	int shmid;
	void * shm;
	struct packet * pack;
	shmid = shmget((key_t)4321, sizeof(struct packet), 0666 | IPC_CREAT);
	if (shmid ==-1)
	{
			cout<<"共享内存创建失败"<<endl;
	        exit(0);
    }
	shm = shmat(shmid,  (void*)0, 0);
	memset(shm,0,sizeof(packet));
	if(shm!=(void *)-1)
		cout<<"共享内存格式化成功"<<endl;
	pack=(struct packet *)shmat(shmid,  (void*)0, 0);
	if(pack==(void *)-1)
	     cout<<"shmat失败"<<endl;
 
	
	char objname[16]={0};
	int num,nlen;
	pid_t pid;
	int listenfd,connectfd;
	struct sockaddr_in servaddr;
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(8888);
	if( (listenfd=socket(AF_INET,SOCK_STREAM,0))==-1 )
		cout<<"socket错误"<<endl;
	if( (bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr)))==-1 )
		cout<<"bind错误"<<endl;
	if( listen(listenfd,10)==-1 )
		cout<<"listen 错误"<<endl;
	while(1)
	{
		if( (connectfd=accept(listenfd,NULL,NULL))==-1)
			cout<<"accept错误"<<endl;
		else		
		cout<<"连接成功"<<endl;
		pid=fork();
		if(pid==0)	//子进程 负责连接
		{		
			close(listenfd);
            // 读取对方名字
			read(connectfd,objname,16);	
            // 判断是不是root用户
			if( !(strcmp(objname,"root")&&strcmp(objname,"Root") ))
			{
				cout<<objname<<" 大佬已连接"<<endl;
			   root_usr(connectfd);
			   cout<<objname<<" 断开连接"<<endl;
			   exit(0);
			}
			cout<<"用户" << objname<<" 已连接"<<endl;
			pid=fork();
			if(pid>0)  //父进程 接受数据
			{
				pack=(struct packet *)shmat(shmid,  (void*)0, 0);
			    if(pack==(void *)-1)
				      cout<<"shmat失败"<<endl;

                printf("pack 二进制:%0x,pack size = %d\n",pack,sizeof(pack));

				while(1)	
				{	
                    // 读取数据
					num=readn(connectfd,pack,4);
                    cout << "pack content:" <<pack ->content << endl;
                    cout << "pack len" << pack->len << endl;
                    cout << "pack name" << endl;
					test(pid,num);
                    cout << "读取第一次test后num" <<num << endl;
					nlen=ntohl(pack->len);
                    cout<< "nlen = " << nlen << endl;
					num=readn(connectfd,pack->name,nlen+16);
                    test(pid,num);
                    cout << "读取第二次test后num" <<num<< endl;

					cout<<pack->name<<" : "<<pack->content<<endl;
					 fp=fopen(FILENAME,"ab");
				     if(fp==NULL)
					 cout<<"历史记录txt打开失败"<<endl;
					 if(fwrite(pack,nlen+20,1,fp)==0)
					   cout<<"fwrite错误"<<endl;
					fclose(fp);
 
				}
			}
			if(pid==0) //子进程 发送数据
			{
				signal(SIGUSR1,func);
				char name_tmp[16]={0};
				char buf_tmp[1024]={0};
				pack=(struct packet *)shmat(shmid,  (void*)0, 0);
				if(pack==(void *)-1)
				        cout<<"shmat失败"<<endl;
				while(1)
				{
					while(1)
					{
					    sleep(0.1);
						//如果pack.name = objname
						if( strcmp(objname,pack->name)!=0 )
						{
							//只要name tmp有一个不相等就break
							 if(  strcmp(name_tmp,pack->name)||strcmp(buf_tmp,pack->content) ) 
							  {
									break;
						          }
						}
					}
					strcpy(name_tmp,pack->name);
					strcpy(buf_tmp,pack->content) ;
					nlen=strlen(pack->content)+1;	//  有'\0'
					int zz=write(connectfd,pack,nlen+20);
					if(zz<=0)
					cout<<"************write 失败 ***"<<endl;
				}
			}		
			else
				cout<<"2pid错误"<<endl;
			exit(0);
		}
		else if(pid>1)	//父进程 继续监听
			close(connectfd);
		else
		{
			cout<<"1pid错误"<<endl;
			exit(0);
		}
	}
	return 0;
}

