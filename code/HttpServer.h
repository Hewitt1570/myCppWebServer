/*************************************************************************
	> File Name: HttpServer.h
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年03月29日 星期日 21时05分11秒
 ************************************************************************/
#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__ 

#include <memory>  // unique_ptr shared_ptr

#define TIMEOUT -1 //epoll_wait 的超时事件  -1表示阻塞等待
#define CON_TIMEOUT 500  //长连接保持时间  500ms 
#define NUM_WORKERS 4  //默认线程池大小

namespace HW_TXL{

//前置声明 避免头文件相互包含
class HttpHandler;
class TimerManager;
class ThreadPool;
class Epoll;

class HttpServer{
public:
	HttpServer(int port,int ThreadNum);
	~HttpServer();

	void run();  //开始运行

private:  //为四类主要事件编写处理方法
	void AcceptNewConnection();  //接受新连接
	void CloseConnection(HttpHandler *);	//关闭连接
	void HandleRequest(HttpHandler *); //处理请求事务
	void MakeResponse(HttpHandler *);  //对请求作出响应

private:  //私有成员变量
	using EpollPtr = std::unique_ptr<Epoll>;
	using ThreadPoolPtr = std::shared_ptr<ThreadPool>;
	using TimerManagerPtr = std::unique_ptr<TimerManager>;
	using ListenHandler = std::unique_ptr<HttpHandler>;
	
	int port_;  //端口号
	int listenFd_;  //监听套接字
	EpollPtr epoll_;  //封装好的Epoll对象指针
	ThreadPoolPtr threadPool_;  //线程池
	TimerManagerPtr timerManager_; //定时器管理器:
	ListenHandler listenHandler_; //监听套件字指针
};

}

#endif

