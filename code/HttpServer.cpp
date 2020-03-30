/*************************************************************************
	> File Name: HttpServer.cpp
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年03月29日 星期日 21时36分30秒
 ************************************************************************/
#include "utils.h"
#include "Epoll.h"
#include "TimerManager.h"
#include "HttpServer.h"

#include <iostream>
#include <cassert>
#include <unistd.h>  //close 
#include <sys/socket.h> //accept 
#include <arpa/inet.h> //sockaddr_in

using namespace HW_TXL;

HttpServer::HttpServer(int port,int threadNum):
	port_(port),
	listenFd_(utils::createListenFd(port_)),
	epoll_(new Epoll()),
	listenHandler_(new HttpHandler(listenFd_)),
	threadPool_(new ThreadPool()),
	TimerManager_(new TimerManager);
{
	assert(listenFd_>0);
}

HttpServer::~HttpServer(){
	
}

void HttpServe::run(){
	//监听套接字挂到EPOLL上 
	epoll_ -> Add(listenFd_,listenHandler_,EPOLLET | EPOLLIN);
	//注册新连接回调函数
	epoll_ -> setNewConnection(std::bind(&HttpServer::AcceptNewConnection,this));
	//注册关闭连接回调函数
	epoll_ -> setCloseConnection(std::bind(&HttpServer::CloseConnection,this,std::placeholders _1));
	//注册请求处理回调函数(读事件)
	epoll_ -> setHandleRequest(std::bind(&HttpServer::HandleRequest,this,std::placeholders _1));
	//注册响应请求回调函数(写事件)
	epoll_ -> setMakeResponse(std::bind(&HttpServer::MakeResponse,this,std::placeholders _1));
	
	//事件循环  
	while(true){
		//获取距离下次连接超时还有多久
		int timeoutMS = TimerManager_->getNextExpireTime();
		//把距离超时的时间作为epoll挂起等待的时限
		int eventsNum = epoll_->wait(timeoutMS);

		if(eventsNum > 0){
			epoll_ -> handleEvent(listenFd_,threadPool_,eventsNum);
		}
		TimerManager_->handleExpiredTimer(); //这里需要注意 先处理响应事务，再处理超时连接
	}
}
//监听套接字是以ET模式挂到EPOLL上的  所以需要处理到EAGAIN
void HttpServer::AcceptNewConnection(){
	while(1){
		int acceptFd = ::accept4(listenFd_,nullptr,nullptr,SOCK_NONBLOCK | SOCK_CLOEXEC);
		if(acceptFd == -1){
			if(errno == EAGAIN)  //此时说明已处理完所有连接请求
				break;
			//为其他错误 退出
			printf("HttpServer::AcceptNewConnection error: %s\n",strerror(errno));
			break;
		}
		//为新连接分配一个Httphandler进行一对一服务
		HttpHandler *handler = new HttpHandler(acceptFd);
		//加入到定时器中
		TimerManager_ -> addTimer(handler,CON_TIMEOUT,std::bind(&HttpServer::CloseConnection,this,handler));
		//挂到EPOLL上监听读事件 LT模式 ONESHOT模式(保证任一时刻只被一个线程处理)
		epool_ -> Add(acceptFd,handler,EPOLLIN | EPOLLONESHOT);
	}
}

void HttpServer::CloseConnection(HttpHandler *handler){
	int fd = handler->getfd();
	//正在工作时不能关闭
	if(handler -> isWorking()) return;
	//删除相应定时器
	TimerManager_ -> delTimer(handler);
	//从epoll上摘下 
	epoll_ -> Del(fd,handler,0);

	delete handler;
	handler = nullptr;
}

void HttpServer::HandleRequest(HttpHandler *handler){
	//每处理一次 需要把计时器摘下 
	TimerManager_ -> delTimer(handler);
	int SavedErrno;
	int ret = handler->write(SavedErrno);
	//read返回0表示对端已经关闭连接
	if(ret==0){
		handler->setNoWorking();
		CloseConnection_(handler);
		return;
	}
	//非EAGAIN错误 断开连接
	if(ret==-1 && SavedErrno!=EAGAIN){
		handler->setNoWorking();
		CloseConnection_(handler);
		return;
	}
	//EAGAIN错误 说明数据还没读完 需要继续监听 为了提高性能，不等待对端把数据准备好 而是直接处理其他事务
	if(ret==-1 && SavedErrno==EAGAIN){
		handler->setNoWorking();
		TimerManager_ -> addTimer(handler,CON_TIMEOUT,std::bind(&HttpServer::CloseConnection,this,handler));
		epoll_ -> Mod(handler->getfd(),handler,EPOLLIN|EPOLLONESHOT);
		return;
	}
	//请求报文正常读入缓冲区  开始解析报文
	
	if(!handler -> parseHttp()){
		
	}
	
	if(handler -> parseFinished()){
		
	}

}

void HttpServer::MakeResponse(HttpHandler *handler){
	TimerManager_ -> delTimer(handler);
	int fd = handler->getfd();

	int dataSize = handler->dataNeedToSend();
	//缓冲区没有要发的数据 重新监听读事件
	if(dataSize == 0){
		epool_ -> Mod(fd,handler,EPOLLIN|EPOLLONESHOT);
		handler -> setNoWorking();
		TimerManager_ -> addTimer(handler,CON_TIMEOUT,std::bind(&HttpServer::CloseConnection,this,handler));
		return;
	}

	int SavedErrno;
	int ret = handler->write(SavedErrno);
	//非EAGAIN错误 断开连接 
	if(ret < 0 && SavedErrno!=EAGAIN){
		handler->setNoWorking();
		CloseConnection_(handler);
		return;
	}
	//对方缓冲区满了 让出线程资源等待下次调用
	if(ret<0 && SavedErrno==EAGAIN){
		TimerManager_ -> addTimer(handler,CON_TIMEOUT,std::bind(&HttpServer::CloseConnection,this,handler));
		epool_ -> Mod(fd,handler,EPOLLIN|EPOLLOUT|EPOLLONESHOT);
		return;
	}
	//数据全部发送完毕
	if(ret == dataSize){
		if(handler -> keepAlive()){ //若为长连接 则继续监听读事件
			epool_ -> Mod(fd,handler,EPOLLIN|EPOLLONESHOT);
			handler -> setNoWorking();
			TimerManager_ -> addTimer(handler,CON_TIMEOUT,std::bind(&HttpServer::CloseConnection,this,handler));
		}else{
			handler->setNoWorking();
			CloseConnection_(handler);
		}
		return;
	}
	handler->setNoWorking();
	TimerManager_ -> addTimer(handler,CON_TIMEOUT,std::bind(&HttpServer::CloseConnection,this,handler));
	epool_ -> Mod(fd,handler,EPOLLIN|EPOLLOUT|EPOLLONESHOT);
	return;
}
