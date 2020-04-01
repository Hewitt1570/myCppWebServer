/*************************************************************************
	> File Name: utils.cpp
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年03月29日 星期日 21时47分03秒
 ************************************************************************/
#include "utils.h"
#include <cstring>
#include <iostream>
#include <sys/socket.h>  //socket setsocketopt bind listen
#include <sys/types.h> 
#include <unistd.h>  //fcntl close
#include <fcntl.h>  //fcntl
#include <stdio.h> //perror
#include <arpa/inet.h>  //htonl htons

using namespace HW_TXL;
int utils::createListenFd(int port){
	//保证端口号合法
	port = ((port<=1024) || (port>=65535))? 8888 : port;
    
	//创建监听套接字
	int listenfd = -1;
	if((listenfd = ::socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK, 0))==-1)
		printf("Utils::createListenFd error: %s\n",strerror(errno));
    
	//设置端口复用
	int opt = 1;
    if(::setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,(const void *)&opt,sizeof(opt))==-1){
		printf("Utils::setsockopt error: %s\n",strerror(errno));
		return -1;
	} 
	// 绑定地址和端口
	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = ::htons((unsigned short)port);
	serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY);
	if(::bind(listenfd,(struct sockaddr *)&serverAddr,sizeof(serverAddr))==-1){
		printf("Utils::bind error: %s\n",strerror(errno));
		return -1;
	}
    //设置监听队列大小
	if(::listen(listenfd,MAX_LISTEN_SIZE)==-1){
		printf("Utils::listen error: %s\n",strerror(errno));
		return -1;
	}

	return listenfd;
}


int utils::setNonBlocking(int fd){
	
	int flag = ::fcntl(fd,F_GETFL,0);
	if(flag == -1){
		printf("utils::serNonBlocking error: %s\n",strerror(errno));
		return -1;
	}

	flag |= O_NONBLOCK;
	if(::fcntl(fd,F_SETFL,flag)==-1){
		printf("utils::serNonBlocking error: %s\n",strerror(errno));
		return -1;	
	}

	return 0;
}



