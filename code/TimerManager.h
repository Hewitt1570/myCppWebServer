/*************************************************************************
	> File Name: TimerManager.h
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年03月30日 星期一 17时40分49秒
 ************************************************************************/
#ifndef __TIMERMANAGER_H__
#define __TIMERMANAGER_H__

#include <iostream>
#include <queue>
#include <chrono>
#include <vector>
#include <cassert>
#include <functional> 
#include <mutex>

namespace HW_TXL{

using timeOutCallBack = std::function<void()>;
using Clock = std::chrono::high_resolution_clock;
using MS = std::chrono::milliseconds;
using TimeStamp = Clock::time_point;

class HttpHandler;

//内部类 每个handler对应一个timer
class Timer{
pubilc:
	Timer(const TimeStamp &when,const timeOutCallBack &cb)
		:delete_(false),
		 expireTime_(when),
		 callBack_(cb){}

	~TImer(){}

	void del(){delete_ = true;}
	bool isDeleted() {return delete_;}
	void runCallBack(){callBack_();}
	TimeStamp getExpireTime()const{return expireTime_;}

private: 
	TimeStamp expireTime_;
	timeOutCallBack callBack_;
	bool delete_;

};

struct cmp{
	bool operator ()(Timer *a,Timer *b){
		assert(a!=nullptr && b!=nullptr);
		return a->getExpireTime() > b->getNextExprireTime();
	}
};

//HttpServer通过TimerManager管理定时器
class TimerManager{
pubilc:
	TimerManager():now_(Clock::now()){}
	~TimerManager(){}
	
	void updateTime(){now_ = Clock::now();}
	void addTimer(HttpHandler *handler,const int& timeOut,const timeOutCallBack &cb);
	void delTimer(HttpHandler *handler);
	void handlerExpiredTimers();
	int getNextExprireTime(); //返回下次超时时间 in MS

private:
	using TimerQueue = std::priority_queue<Timer*,std::vector<Timer*>,cmp>;
	
	TimerQueue timerQueue_;
	TimeStamp now_;
	std::mutex lock_;

};

}
#endif

