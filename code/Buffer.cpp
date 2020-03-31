/*************************************************************************
	> File Name: Buffer.cpp
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年03月31日 星期二 20时09分51秒
 ************************************************************************/
#include "Buffer.h"

#include <cstring> //perror
#include <iostream>

#include <unistd.h> //read write 
#include <sys/uio.h>  //readv 
using namespace HW_TXL;

//分散读
ssize_t Buffer::readFd(int fd,int &SavedErrno){
	char extraBuf[65536];  //开个大型临时缓冲区接受
	struct iovec vec[2];
	const size_t writable = writableBytes();
	vec[0].iov_base = beginWrite();
	vec[0].iov_len = writable;
	vec[1].iov_base = extraBuf;
	vec[1].iov_len = sizeof(extraBuf);

	const ssize_t n = ::readv(fd,vec,2);

	if(n<0){
		printf("Buffer::readFd error : %s\n",strerror(errno));
		SavedErrno = errno;
	}
	else if(static<int>(n) < writable){
		hasWritten(n);
	}
	else{
		writeIndex_ = buffer_.size();
		append(extraBuf,n - writable);
	}
	return n;
}

ssize_t Buffer::writeFd(int fd, int &SavedErrno){
	size_t readable = readableBytes();
	char *bufPtr = __begin()+readIndex_;
	ssize_t n;
	if((n = write(fd,bufPtr,readble))<=0){
		if( n<0 && errno == EINTR)
			return 0;
		else{
			printf("Buffer:writeFd error %s\n",strerror(errno));
			SavedErrno = errno;
			return -1;
		}
	}
	readIndex_ += n;
	return n;
}
