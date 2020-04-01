/*************************************************************************
	> File Name: Buffer.h
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年03月31日 星期二 18时23分26秒
 ************************************************************************/
#ifndef __BUFFER_H__
#define __BUFFER_H_ 

#include <vector> 
#include <string>
#include <algorithm> //copy search函数 
#include <cassert>
#include <iostream>

#define INIT_SIZE 1024  //缓冲区初始大小

namespace HW_TXL{

class Buffer{
public:
	Buffer():
		buffer_(INIT_SIZE),
		readIndex_(0),
		writeIndex_(0)
	{
		assert(readableBytes() == 0);
		assert(writableBytes() == INIT_SIZE);
	}
	~Buffer(){}

	size_t readableBytes()const{  //可读字节数
		return writeIndex_ - readIndex_;
	}

	size_t writableBytes()const{  //可写字节数
		return buffer_.size() - writeIndex_;
	}

	size_t preFreeSpace()const{   //readIndex_ 前有多少空闲空间
		return readIndex_;
	}

	const char *beginAt()const{   //返回读指针
		return __begin() + readIndex_;
	}

	void retrieve(const size_t &len){  //丢掉len个字节
		assert(len <= readableBytes());
		readIndex_ += len;
	}

	void retrieveAll(){   //丢弃所有数据
		readIndex_ = 0;
		writeIndex_ = 0;
	}

	void retrieveUntil(const char *end){ //丢前end前的所有数据
		assert(beginAt()<=end);
		assert(end <= beginWrite());
		retrieve(end-beginAt());
	}
	
	std::string retrieveAsString(){  //以string的形式取出所有数据
		std::string ret(beginAt(),readableBytes());
		retrieveAll();
		return ret;
	}
	
	void append(const std::string &str){ //添加数据至缓冲区
		append(str.c_str(),str.length());
	}

	void append(const char *data,size_t len){
		ensureFreeSpace(len);
		std::copy(data,data+len,beginWrite());
		hasWritten(len);
	}

	void append(const void *data,size_t len){
		append(static_cast<const char*>(data),len);
	}

	void append(const Buffer &buf){
		append(buf.beginAt(),buf.readableBytes());
	}

	void ensureFreeSpace(size_t len){  //确保可写len个字节
		if(writableBytes()<len){  //不足则调用扩容函数
			__makeSpace(len);
		}
		assert(writableBytes()>=len);
	}
	char *beginWrite(){  //返回写指针
		return __begin()+writeIndex_; 
	}
	
	const char *beginWrite()const {
		return __begin()+writeIndex_;
	}
	
	void hasWritten(size_t len){
		writeIndex_ += len;
	}

	ssize_t readFd(int fd,int &SavedErrno); //从socket读入缓冲区
	ssize_t writeFd(int fd,int &SavedErrno); //从缓冲区写入socket

	const char *findCRLF()const {  //从头开始找第一个回车换行符
		const char CRLF[] = "\r\n";
		const char *crlf = std::search(beginAt(),beginWrite(),CRLF,CRLF+2);
		return crlf == beginWrite()?nullptr:crlf;
	}

	const char *findCRLF(const char *start)const{ //从start开始找回车换行符
		assert(start>=beginAt());
		assert(start<=beginWrite());
 
		const char CRLF[] = "\r\n";
		const char *crlf = std::search(start,beginWrite(),CRLF,CRLF+2);
		return crlf == beginWrite()?nullptr:crlf;
	}

private:
	char *__begin(){   //返回缓冲区头指针
		return &*buffer_.begin();
	}
	
	const char *__begin()const{
		return &*buffer_.begin();
	}

	void __makeSpace(size_t len){
		if(writableBytes() + preFreeSpace() >=len){
			size_t readalbe = readableBytes();
			std::copy(__begin()+readIndex_,__begin()+writeIndex_,__begin());
			readIndex_ = 0;
			writeIndex_ = readalbe;
			assert(readalbe == readableBytes());
		}else{
			buffer_.resize(writeIndex_+len);
		}
	}

private:
	std::vector<char> buffer_;
	size_t readIndex_;
	size_t writeIndex_;
};

}
	
#endif
