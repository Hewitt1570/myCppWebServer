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
#include "HttpHandler.h"
#include "HttpResponse.h"
#include "ThreadPool.h"

#include <iostream>
#include <cassert>
#include <cstring>
#include <functional>
#include <unistd.h>  //close 
#include <sys/socket.h> //accept 
#include <arpa/inet.h> //sockaddr_in

using namespace HW_TXL;

HttpServer::HttpServer(int port,int threadNum):  //一定要注意：使用初始化列表时 初始化顺序与定义顺序相同
	port_(port),								//特别是当后面的成员依赖前面的成员初始化时  一定要注意顺序
	listenFd_(utils::createListenFd(port)),
	epoll_(new Epoll()),
	listenHandler_(new HttpHandler(listenFd_)),
	threadPool_(new ThreadPool(threadNum)),
	timerManager_(new TimerManager())
{
//	printf("server ok! port:%d listenFd_:%d\n",port_,listenFd_);
	assert(listenFd_>0);
}

HttpServer::~HttpServer(){
	
}

void HttpServer::run(){
	//监听套接字挂到EPOLL上 
	epoll_ -> Add(listenFd_,listenHandler_.get(), EPOLLIN|EPOLLET);
	//注册新连接回调函数
	epoll_ -> setNewConnection(std::bind(&HttpServer::AcceptNewConnection,this));
	//注册关闭连接回调函数
	epoll_ -> setCloseConnection(std::bind(&HttpServer::CloseConnection,this,std::placeholders:: _1));
	//注册请求处理回调函数(读事件)
	epoll_ -> setHandleRequest(std::bind(&HttpServer::HandleRequest,this,std::placeholders:: _1));
	//注册响应请求回调函数(写事件)
	epoll_ -> setMakeResponse(std::bind(&HttpServer::MakeResponse,this,std::placeholders:: _1));
	
	//事件循环  
	while(true){
		//获取距离下次连接超时还有多久
		int timeoutMS = timerManager_->getNextExpireTime();
		
		//把距离超时的时间作为epoll挂起等待的时限
		int eventsNum = epoll_->wait(timeoutMS);
		
	//	printf("events happen %d\n",eventsNum);
		if(eventsNum > 0){
			epoll_ -> handleEvent(listenFd_,threadPool_,eventsNum);
		}
		timerManager_->handleExpiredTimers(); //这里需要注意 先处理响应事务，再处理超时连接
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
		timerManager_ -> addTimer(handler,CON_TIMEOUT,std::bind(&HttpServer::CloseConnection,this,handler));
		//挂到EPOLL上监听读事件 LT模式 ONESHOT模式(保证任一时刻只被一个线程处理)
		epoll_ -> Add(acceptFd,handler,EPOLLIN | EPOLLONESHOT | EPOLLET);
	}
}

void HttpServer::CloseConnection(HttpHandler *handler){
	int fd = handler->getfd();
	//正在工作时不能关闭
	if(handler -> isWorking()) return;
	//删除相应定时器
	timerManager_ -> delTimer(handler);
	//从epoll上摘下 
	epoll_ -> Del(fd,handler,0);

	delete handler;
	handler = nullptr;
}

void HttpServer::HandleRequest(HttpHandler *handler){
	//每处理一次 需要把计时器摘下 
	timerManager_ -> delTimer(handler);
	int SavedErrno;
	int ret = handler->read(SavedErrno);
//	printf("接受了多少数据： %d\n",ret);
	//read返回0表示对端已经关闭连接
	if(ret==0){
		handler->setNoWorking();
		CloseConnection(handler);
		return;
	}
	//非EAGAIN错误 断开连接
	if(ret==-1 && SavedErrno!=EAGAIN){
		handler->setNoWorking();
		CloseConnection(handler);
		return;
	}
	//EAGAIN错误 说明数据还没读完 需要继续监听 为了提高性能，不等待对端把数据准备好 而是直接处理其他事务
	if(ret==-1 && SavedErrno==EAGAIN){
		handler->setNoWorking();
		timerManager_ -> addTimer(handler,CON_TIMEOUT,std::bind(&HttpServer::CloseConnection,this,handler));
		epoll_ -> Mod(handler->getfd(),handler,EPOLLIN|EPOLLONESHOT|EPOLLET);
		return;
	}
	//请求报文正常读入缓冲区  开始解析报文
//	printf("开始解析报文\n");
//	//解析失败 发送错误报文 断开连接 
	if(!handler -> parseHttp()){
//		printf("报文解析失败\n");
		HttpResponse response(400,"",false);
		handler -> appendToOutBuffer(response.makeResponse());
		
		//XXX 关闭前只写了一次 可能会有问题
		int writeErrno;
		handler -> write(writeErrno);
		handler -> setNoWorking();
		CloseConnection(handler);
		return;
	}	
	
	//解析完成 
	if(handler -> parseFinished()){
//		printf("解析完成\n");
		HttpResponse response(200,handler->getPath(),handler->keepAlive());
		handler -> appendToOutBuffer(response.makeResponse());
//		timerManager_->addTimer(handler,CON_TIMEOUT,std::bind(&HttpServer::CloseConnection,this,handler));
		epoll_ -> Mod(handler->getfd(),handler,EPOLLIN|EPOLLOUT|EPOLLONESHOT|EPOLLET);
	}
}

void HttpServer::MakeResponse(HttpHandler *handler){
	timerManager_ -> delTimer(handler);
	int fd = handler->getfd();

	int dataSize = handler->dataNeedToSend();
	//缓冲区没有要发的数据 重新监听读事件
	if(dataSize == 0){
		epoll_ -> Mod(fd,handler,EPOLLIN|EPOLLONESHOT|EPOLLET);
		handler -> setNoWorking();
		timerManager_ -> addTimer(handler,CON_TIMEOUT,std::bind(&HttpServer::CloseConnection,this,handler));
		return;
	}

	int SavedErrno;
	int ret = handler->write(SavedErrno);
	//非EAGAIN错误 断开连接 
	if(ret < 0 && SavedErrno!=EAGAIN){
		handler->setNoWorking();
		CloseConnection(handler);
		return;
	}
	//对方缓冲区满了 让出线程资源等待下次调用
	if(ret<0 && SavedErrno==EAGAIN){
		timerManager_ -> addTimer(handler,CON_TIMEOUT,std::bind(&HttpServer::CloseConnection,this,handler));
		epoll_ -> Mod(fd,handler,EPOLLIN|EPOLLOUT|EPOLLONESHOT|EPOLLET);
		return;
	}
	//数据全部发送完毕
	if(ret == dataSize){
		if(handler -> keepAlive()){ //若为长连接 则继续监听读事件 
			handler->resetParse();
			epoll_ -> Mod(fd,handler,EPOLLIN|EPOLLONESHOT|EPOLLET);
			handler -> setNoWorking();
			timerManager_ -> addTimer(handler,CON_TIMEOUT,std::bind(&HttpServer::CloseConnection,this,handler));
		}else{
			handler->setNoWorking();
			CloseConnection(handler);
		}
		return;
	}
	handler->setNoWorking();
	timerManager_ -> addTimer(handler,CON_TIMEOUT,std::bind(&HttpServer::CloseConnection,this,handler));
	epoll_ -> Mod(fd,handler,EPOLLIN|EPOLLOUT|EPOLLONESHOT|EPOLLET);
	return;
}
