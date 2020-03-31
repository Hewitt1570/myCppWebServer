/*************************************************************************
	> File Name: HttpHandler.h
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年03月30日 星期一 10时57分28秒
 ************************************************************************/
#ifndef __HTTPHANDLER_H__
#define __HTTPHANDLER_H__ 

#define STATIC_ROOT ../www
#include "Buffer.h"

#include <iostream>
#include <string>
#include <unordered_map> 

namespace HW_TXL{

class Timer;

class HttpHandler{
public:
	enum HttpParseState {//报文解析状态 
		ExpectRequestLine,
		ExpectHeaders,
		ExpectBody,
		GotAll
	};
	
	enum RequstMethod {//请求方法  
		Invalid,GET,POST,HEAD,PUT,DELETE
	};

	enum Version {//HTTP版本 
		Unknown,HTTP10,HTTP11
	};

	HttpHandler(int fd);
	~HttpHandler();
	
	//外部接口
	int getfd(){return fd_;} //返回文件描述符号
	Timer *getTimer(){return timer_;} //返回定时器指针
	void setTimer(Timer *timer){timer_ = timer;} //设置定时器指针
	
	int read(int &SavedErrno);  //将数据从socket读到应用层缓冲区
	int write(int &SavedErrno); //将数据从应用层缓冲区写到socket

	void appendToOutBuffer(const Buffer &buf){outBuffer_.append(buf);} //将response返回的数据添加到写缓冲区
	int dataNeedToSend(){return outBuffer_.readableBytes();}//返回写缓冲区有多少字节数据需要发送

	void setNoWorking(){working_ = false;}
	void setWorking(){working_ = true;}
	bool isWorking(){return working_;}

	bool parseHttp(); //解析Http报文
	bool parseFinished(){return state_ == GotAll;}//是否解析完
	
	void resetParse();
	std::string getPath()const { return path_; }
	std::string getQuery()const { return query_; }
	std::string getHeader(const std::string &field) const;
	std::string getMethod() const;
	bool keepAlive() const;

private:
	//解析请求行
	bool __parseRequestLine(const char *begin,const char *end); //左闭右开
	//设置请求方法
	bool __setMethod(const char *begin,const char *end);
	//设置URL路径
	void __setPath(const char *begin,const char *end){
		std::string subPath(begin,end);
		if(subPath == '/')
			subPath = "/pipioj.html";
		path_ = STATIC_ROOT + subPath;
	}
	//设置URL询问
	void __setQuery(const char *begin,const char *end){
		query_.assign(begin,end);
	}
	void __setVersion(Version version){
		version_ = version;
	}
	//添加报文头 field:value 用unordered_map保存
	void __addHeader(const char *begin,const char *colon,const char *end);

private:
	//网络通信相关
	int fd_;  //套接字文件描述符
	Buffer inBuffer_;   //应用层读缓冲
	Buffer outBuffer_;  //应用层写缓冲
	bool working_;  //是否正在工作 

	//定时器相关
	Timer *timer_;
	
	//HTTP报文解析相关
	HttpParseState state_;	//报文解析状态
	RequstMethod method_;	//HTTP请求方法
	Version version_;		//HTTP版本
	std::string path_;	    //URL路径
	std::string query_;		//URL询问
	std::unordered_map<std::string,std::string> headers_;//报文头部
};

}
#endif
