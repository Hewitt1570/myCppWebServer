/*************************************************************************
	> File Name: ThreadPool.cpp
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年03月30日 星期一 17时18分59秒
 ************************************************************************/
#include "ThreadPool.h"
#include <mutex>

using namespace HW_TXL;

ThreadPool::ThreadPool(int threadNum):stop_(false){
	threadNum = threadNum<0?1:threadNum;
	for(int i=0;i<threadNum;i++)
		threads_.emplace_back([this](){
			while(true){
				JobFunc work;
				{
					std::unique_lock<std::mutex> lock(lock_);
					while(!stop_ && jobs_.empty())
						cond_.wait(lock);  //因条件变量挂起前会解锁
					if(jobs_.empty() && stop_){
						//线程池已关闭
						return;
					}
					work = jobs_.front();
					jobs_.pop();
				} //离开作用域 自动解锁
				if(work!=nullptr) work();
			}
		});
}

ThreadPool::~ThreadPool(){
	{
		std::unique_lock<std::mutex> lock(lock_);
		stop_ = true;
	}
	cond_.notify_all();
	for(auto &thread : threads_)
		thread.join();
}

//生产者
void ThreadPool::pushJob(const JobFunc &job){
	{
		std::unique_lock<std::mutex> lock(lock_);
		jobs_.push(job);
	}
	cond_.notify_one();
}

