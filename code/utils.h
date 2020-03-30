/*************************************************************************
	> File Name: utils.h
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年03月29日 星期日 21时41分34秒
 ************************************************************************/
#ifndef __UTILS_H__
#define __UTILS_H__   

#define MAX_LISTEN_SIZ 1024 //listen队列上限

namespace HW_TXL{
	
namespace utils{
	int createListenFd(int port);  //创建监听套接字
	int setNonBlocking(int fd);   //设置非阻塞属性
}

}

#endif
