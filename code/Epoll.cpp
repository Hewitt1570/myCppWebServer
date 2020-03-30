/*************************************************************************
	> File Name: Epoll.cpp
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年03月29日 星期日 22时51分37秒
 ************************************************************************/
#include "Epoll.h" 
#include "ThreadPool.h"
#include "HttpHandler.h"

#include <memory>
#include <sys/epoll.h> //epoll  
#include <unistd.h> //close 
#include <cstring> //strerror
#include <cassert> //assert  
using namespace HW_TXL;

Epoll::Epoll():
	epollFd_(::epoll_create1(EPOLL_CLOEXEC)),
	events_(MAXEVENTS)
{
	assert(epollFd_ > 0);
}

Epoll::~Epoll(){
	::close(epollFd_);
}
//向epoll中添加监听事件
int Epoll::Add(int fd,HttpHandler *handler,int events){
	struct epoll_event event;
	event.data.ptr = (void *)hander;
	event.events = events;
	int ret = ::epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event);
	return ret;
}
//修改监听事件类型
int Epoll::Mod(int fd,HttpHandler *handler,int events){
	struct epoll_event event;
	event.data.ptr = (void *)hander;
	event.events = events;
	int ret = ::epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event);
	return ret;
}
//将事件从epoll上摘下
int Epoll::Del(int fd,HttpHandler *handler,int events){
	struct epoll_event event;
	event.data.ptr = (void *)hander;
	event.events = events;
	int ret = ::epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event);
	return ret;
}

//阻塞监听 
int Epoll:wait(int timeout_MS){
	int eventsNum = ::epoll_wait(epollFd_, &*events_.begin(), MAXEVENTS, timeout_MS);
	if(eventsNum < 0){
		printf("Epoll::wait error: %s\n",strerror(errno));
	}	
	return eventsNum;
}

void Epoll::handleEvent(int listenfd,std::shared_ptr<ThreadPool> &threadpool,int eventsNum){
	assert(eventsNum>0);
	for(int i=0;i<eventsNum;i++){
		struct epoll_event &ev = events_[i];
	
		HttpHandler *handler = (HttpHandler *)(ev.data.ptr);
		int fd = handler -> getfd();
		
		if(fd == listenfd){ //由于listenfd只监听了读事件 所以只有可能是产生了新连接
			NewConnection_();
		}else{
			//检查出错事件
			if((ev.events & EPOLLERR) || (ev.events & EPOLLHUP)){
				//错误关闭连接
				handler -> setNoWorking();
				CloseConnection_(handler);
			}else if(ev.events & EPOLLOUT){//先处理写事件
				//处理写事件
				handler -> setWorking();
				threadpool -> pushJob(std::bind(MakeResponse_,handler));				
			}else if(ev.events & EPOLLIN){
				//处理读事件 把任务挂到线程池任务队列中
				handler -> setWorking();
				threadpool -> pushJob(std::bind(HandleRequest_,handler));
			}else{
				printf("Epoll::handleEvent unexpected event\n");
			}
		}
		

	}
}
