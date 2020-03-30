/*************************************************************************
	> File Name: TimerManager.cpp
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年03月30日 星期一 18时09分40秒
 ************************************************************************/
#include "TimerManager.h"

using namespace HW_TXL;

void TimerManager::addTimer(HttpHandler *handler,const int& timeOut,const timeOutCallBack &cb){
	Timer *timer;
	{
		std::unique_lock<mutex> lock(lock_);
		updateTime();
		timer = new Timer(now_+MS(timeOut),cb);
		timerQueue_.push(timer);
	}
	if(handler->getTimer()!=nullptr)
		delTimer(handler);
	
	handler->setTimer(timer);
}

//惰性删除——使用到时才从堆上摘下
void TimerManager::delTimer(HttpHandler *handler){
	assert(handler!=nullptr);

	Timer *timer = handler->getTimer();
	if(timer==nullptr)return;

	//不能直接delete timer 
	timer -> del();
	//防止handler->getTimer访问野指针
	handler->setTimer(nullptr);
}
//处理超时定时器
void TimerManager::handlerExpiredTimers(){
	//将超时定时器放到vec中 避免持有锁的时候执行回调函数 
	std::vector<Timer*>vec;
	{
		std::unique_lock<mutex> lock(lock_);
		updateTime();
		while(!timerQueue_.empty()){
			Timer *timer = timerQueue_.top();
			//定时器已经被删除 无需调用回调函数
			if(timer->isDeleted()){
				timerQueue_.pop();
				delete timer;
				continue;
			}
			//判断是否超时
			if(std::chrono::duration_cast<MS>(timer->getExpireTime()-now_).count()>0){
				//当前节点未超时  后面的节点必然未超时
				return;
			}
			//进入此处说明超时
			vec.push_back(timer);
			timerQueue_.pop();
		}
	}
	for(int i=0;i<vec.size();i++){ //注意调用顺序
		vec[i]->runCallBack();
		delete vec[i];
	}
}
int TimerManager::getNextExprireTime(){
	std::unique_lock<mutex> lock(lock_);
	updateTime();
	while(!timerQueue_.empty()){
		Timer *timer = timerQueue_.top();
		if(timer->isDeleted()){
			timerQueue_.pop();
			delete timer;
			continue;
		}
		int res = std::chrono::duration_cast<MS>(timer->getExpireTime()-now_).count();
		res = res<0?0:res;
		break;
	}
	return res;
} //返回下次超时时间 in MS


