/*************************************************************************
	> File Name: HttpServer.cpp
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年03月29日 星期日 21时36分30秒
 ************************************************************************/
#include "utils.h"
#include "Epoll.h"
#include "HttpServer.h"

#include <iostream>
#include <cassert>

using namespace HW_TXL;

HttpServer::HttpServer(int port,int threadNum):
	port_(port),
	listenFd_(utils::createListenFd(port_)),
	epoll_(new Epoll()),
	listenHandler_(),
	threadPool_(),
	TimerManager()
{
	
}

HttpServer::~HttpServer(){
	
}

void HttpServe::run(){

}

