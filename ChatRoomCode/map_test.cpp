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
#include <unordered_map>
#include <vector>

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

int main()
{
    unordered_map<int, vector<int > > heartMap;
    vector<int> userinfo(3,0);
    userinfo[0] = 3;
    userinfo[1] = 1;
    heartMap.insert(make_pair(4,userinfo));
    unordered_map<int, vector<int > >::iterator it = heartMap.begin();
    for(;it!=heartMap.end();)
    {
        cout << "当前connectfd" << it->first << ",UID" << it -> second[1] << ",timer:" << it -> second[2] << endl;
        it++;
    }
}