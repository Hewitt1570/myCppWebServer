/*************************************************************************
	> File Name: HttpResponse.h
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年03月31日 星期二 15时34分23秒
 ************************************************************************/
#ifndef __HTTPRESPONSE_H__
#define __HTTPRESPONSE_H__

#include <map>
#include <string>

namespace HW_TXL{

class Buffer;

class HttpResponse{
public:
	static const std::map<int,std::string>stateCode_Message;
	static const std::map<std::string,std::string>suffix_Type;
	
	HttpResponse(int state,std::string path,bool keepAlive):
		stateCode_(state),
		path_(path),
		keepAlive_(keepAlive)
	{}

	~HttpResponse(){}

	Buffer makeResponse();
	void errorResponse(Buffer &outBuffer,std::string &message);
	void staticRequest(Buffer &outBuffer,long filesize);

private:
	std::string __getFileType();

private:
	std::map<std::string,std::string> headers_;  //响应头部
	int stateCode_;  //状态码
	std::string path_; //资源路径
	bool keepAlive_;  //是否为长连接  
};

}

#endif
