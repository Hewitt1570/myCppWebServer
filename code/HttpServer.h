/*************************************************************************
	> File Name: HttpServer.h
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年03月29日 星期日 21时05分11秒
 ************************************************************************/
#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__ 

#include <memory>  // unique_ptr shared_ptr



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
	void CloseConnection();	//关闭连接
	void HandleRequest(); //处理请求事务
	void MakeResponse();  //对请求作出响应

private:  //私有成员变量
	using EpollPtr = unique_ptr<Epoll>;
	using ThreadPoolPtr = shared_ptr<ThreadPool>;
	using TimerManagerPtr = unique_ptr<TimerManager>;
	using ListenHandler = unique_ptr<HttpHandler>;

	int listenFd_;  //监听套接字
	int port_;  //端口号
	EpollPtr epoll_;  //封装好的Epoll对象指针
	ThreadPoolPtr threadPool_;  //线程池
	TimerManagerPtr timerManager_; //定时器管理器:
	ListenHandler listenHandler_; //监听套件字指针
}

}

#endif

