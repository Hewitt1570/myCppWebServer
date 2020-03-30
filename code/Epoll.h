/*************************************************************************
	> File Name: Epoll.h
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年03月29日 星期日 22时24分30秒
 ************************************************************************/
#ifndef __EPOLL_H__
#define __EPOLL_H__ 

#include <functinal>  //function
#include <memory>  //shared_ptr
#include <vector>  //用vector保存返回的epoll_events
#include <sys/epoll.h> //epoll epoll_event

#define MAXEVENTS 2048

namespace HW_TXL{

class HttpHandler;
class ThreadPool;

class Epoll{
public:
	//定义四种函数指针类型名
	using NewConnectionCb = function<void()>;
	using CloseConnectionCb = function<void(HttpHandler*)>;
	using HandleRequestCb = function<void(HttpHandler*)>;
	using MakeResponseCb = function<void(HttpHandler*)>;


	Epoll();
	~Epoll();
	
	//对应epoll_ctl的EPOLL_ADD EPOLL_DEL EPOLL_MOD
	int Add(int fd,HttpHandler *handler,int events); 
	int Del(int fd,HttpHandler *handler,int events);
	int Mod(int fd,HttpHandler *handler,int events);
	
	//阻塞监听 并设置超时时间 返回活跃文件描述符
	int wait(int timeout_MS);
	
	//设置四种回调函数
	void setNewConnection(const NewConnectionCb &cb){NewConnection_ = cb;}
	void setCloseConnection(const CloseConnectionCb &cb){CloseConnection_ = cb;}
	void setHandleRequest(const HandleRequestCb &cb){HandleRequest_ = cb;}
	void setMakeResponse(const MakeResponseCb &cb){MakeResponse_ = cb;}
	
	//对不同的事件类型将相应的处理方法加入到线程池的事件队列中
	void handleEvent(int listenfd,std::shared_ptr<ThreadPool> &threadpool);

private:
	using EventList = std::vector<struct epoll_event>;

	NewConnectionCb NewConnection_;
	CloseConnectionCb CloseConnection_;
	HandleRequestCb HandleRequest_;
	MakeResponseCb MakeResponse_;
	int epollFd_;
	EventList events_;

}


}
#endif
