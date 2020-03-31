/*************************************************************************
	> File Name: HttpHandler.cpp
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年03月30日 星期一 22时26分01秒
 ************************************************************************/
#include "HttpHandler.h"

#include <unistd.h>
using namespace HW_TXL;

HttpHandler::HttpHandler(int fd):
	fd_(fd),
	working_(false),
	timer_(nullptr),
	state_(ExpectRequestLine),
	method_(Invalid),
	version_(Unknown){}

HttpHandler::~HttpHandler(){
	close(fd_);
}

int HttpHandler::read(int &SavedErrno){
	int ret = inBuffer_.readFd(fd_,SavedErrno);
	return ret;
}

int HttpHandler::write(int &SavedErrno){
	int ret = outBuffer_.writeFd(fd_,SavedErrno);
	return ret;
}

bool HttpHandler::parseHttp(){
	bool ok = true,hasmore = ture;
	while(hasmore){
		if(state_ == ExpectRequestLine){
			const char *crlf = inBuffer_.findCRLF();
			if(crlf){
				ok = __parseRequestLine(inBuffer_.beginAt(),crlf);
				if(ok){
					state_ = ExpectHeaders;
					inBuffer_.retrieveUntil(crlf+2);
				} else{
					hasmore = false;
				}
			}else{  //没有回车换行 解析失败
				ok = false;
				hasmore = false;
			}
		}else if(state_ == ExpectHeaders){
			const char *crlf = inBuffer_.findCRLF();
			if(crlf){
				const char *colon = std::find(inBuffer_.beginAt(),crlf,':');
				if(colon != crlf){
					__addHeader(inBuffer_.beginAt(),colon,crlf);
				} else{
					hasmore = false;
					state_ = ExpectBody;
				}
				inBuffer_.retrieveUntil(crlf+2);
			}else{
				ok = false;
				hasmore = false;
			}
		}else if(state_ == ExpectBody){
			// TODO 处理报文题
			inBuffer_.retrieveAll();
			state_ = GotAll;
		}
		
	}
	return ok;
}

bool HttpHandler::__parseRequestLine(const char *begin,const char *end){
	bool ok = false;
	const char *start = begin;
	const char *space = std::find(start,end,' ');
	if(space != end && __setMethod(start,space)){
		start = space+1;
		space = std::find(start,end,' ');
		if(space != end){
			const char *question = std::find(start,space,'?');
			if(question != space){
				__setPath(start,question);
				__setQuery(question+1,space);
			} else{
				__setPath(start,space);
			}
			start = space+1;
			ok = end - start == 8 && std::equal(start,end-1,"HTTP/1.");
		}
		if(ok){
			if(*(end-1)=='1') __setVersion(HTTP11);
			else if(*(end-1)=='0') __setVersion(HTTP10);
			else ok = false;
		}
	}
	return ok;
}

bool HttpHandler::__setMethod(const char *start,const char *end){
	std::string m(start,end);
	if(m == "GET"){
		method_ = GET;
	}else if(m == "POST"){
		method_ = POST;
	}else if(m == "HEAD"){
		method_ = HEAD;
	}else if(m == "PUT"){
		method_ = PUT;
	}else if(m == "DELETE"){
		method_ = DELETE;
	}else 
		method_ = Invalid;

	return method_ !=Invalid;
}

void HttpHandler::__addHeader(const char *begin,const char *colon,const char *end){
	std::string field(begin,colon);
	++colon;
	while(colon < end && *colon==' ')
		++colon;
	
	std::string value(colon,end);
	while(!value.empty() && value.back()==' ')
		value.pop_back();

	headers_[field] = value;
}

std::string HttpHandler::getMethod() const {
	std::string ret;
	if(method_ == GET){
		ret = "GET";
	}else if(method_ == POST){
		ret  = "POST";
	}else if(method_ == HEAD){
		method_ = "HEAD";
	}else if(method_ == PUT){
		method_ = "PUT";
	}else if(method_ == DELETE){
		method_ = "DELETE";
	return method_;
}

std::string HttpHandler::getHeader(const std::string &field){
	std::string ret;
	if(headers_.count(field))ret = headers_[field];
	return ret;
}

bool HttpHandler::keepAlive() const{
	std::string connection = getHeader("Connection");
	bool res = connection == "Keep-Alive" || version_ == HTTP11 && connection != "close";
	return res;
}

void HttpHandler::resetParse(){
	state_ = ExpectRequestLine;
	method_ = Invalid;
	version_ = Unknown;
	path_ = "";
	query_ = "";
	headers_.clear();
}
