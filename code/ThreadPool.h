/*************************************************************************
	> File Name: ThreadPool.h
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年03月30日 星期一 17时06分00秒
 ************************************************************************/
#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__ 


#include <vector>
#include <functional>
#include <thread>
#include <queue>
#include <condition_variable>
#include <mutex>

namespace HW_TXL{

class ThreadPool{
public: 
	using JobFunc = std::function<void()>;

	ThreadPool(int threadNum);
	~ThreadPool();
	void pushJob(const JobFunc &job);

private:
	std::vector<std::thread> threads_;  //线程池
	std::queue<JobFunc> jobs_;		//任务队列
	std::mutex lock_;			//互斥量 保护任务队列
	std::condition_variable cond_; //条件变量  构成生产者消费者模型
	bool stop_;
};

}

#endif
