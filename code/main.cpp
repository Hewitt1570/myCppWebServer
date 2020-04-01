/*************************************************************************
	> File Name: main.cpp
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年04月01日 星期三 09时50分26秒
 ************************************************************************/
#include "HttpServer.h"
#include <stdlib.h>

using namespace HW_TXL;

int main(int argc, char **argv){
	int port = 6666;
	if(argc>=2){
		port = atoi(argv[1]);
	}
	
	int threadNum = 4;
	if(argc>=3){
		threadNum = atoi(argv[2]);
	}
	printf("%d %d\n",port,threadNum);
	HttpServer server(port,threadNum);
	server.run();
	return 0;
}


