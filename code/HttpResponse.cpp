/*************************************************************************
	> File Name: HttpResponse.cpp
	> Author: Hewitt
	> Mail: 309488437@qq.com 
	> Created Time: 2020年03月31日 星期二 15时50分04秒
 ************************************************************************/
#include "HttpResponse.h"
#include "Buffer.h"

#include <math.h>
#include <string>
#include <sys/stat.h> //stat 
#include <unistd.h>  //close 
#include <fcntl.h>  //open 
#include <sys/mman.h> //mmap  munmap 

using namespace HW_TXL;

const std::unordered_map<int,std::string> HttpResponse::stateCode_Message = {
	{200,"OK"},
	{400,"Bad Request"},
	{403,"Forbidden"},
	{404,"Not Found"}
};

const std::unordered_map<std::string,std::string> HttpResponse::suffix_Type = {
	{".html","text/html"},
	{".xml","text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
};

Buffer HttpResponse::makeResponse(){
	Buffer output;

	if(stateCode_ == 400){
		errorResponse(output,"HW_TXL can't parse the message~");
		return output;
	}

	//文件相关
	struct stat sourceStat;
	//文件未找到
	if(::stat(path_.data(), &sourceStat) < 0){
		stateCode_ = 404;
		errorResponse(output,"HW_TXL can't find the file~");
		return output;
	}

	//文件权限错误
	if(!(S_IRUSR & sourceStat.st_mode)){
		stateCode_ = 403;
		errorResponse(output,"HW_TXL can't read the file~");
		return output;
	}

	//开始处理静态请求
	staticRequest(output,sourceStat.st_size);
	return output;
}

void HttpResponse::staticRequest(Buffer &output,long filesize){
	assert(filesize>=0);

	auto iter = stateCode_Message.find(stateCode_);
	if(iter == stateCode_Message.end()){
		stateCode_ = 400;
		errorResponse(output,"Unknown status code~");
		return;
	}

	//响应行  http版本 + 状态码 + 状态码描述 + 回车换行
	output.append("HTTP/1.1 " + std::to_string(stateCode_) + iter->second + "\r\n");

	//响应头部 
	if(keepAlive_){
		output.append("Connection: Keep-Alive\r\n");
		output.append("Keep-Alive: timeout=500\r\n");
	}else{
		output.append("Connection: close\r\n");
	}
	output.append("Content-type: " + __getFileType() + "\r\n");
	output.append("Content-length: " + std::to_string(filesize) + "\r\n");
	
	output.append("Server: HW_TXL\r\n");
	output.append("\r\n");

	//响应正文
	int srcFd = ::open(path_.data(),O_RDONLY);
	void *mmapPtr = ::mmap(NULL,filesize,PROT_READ,MAP_PRIVATE,srcFd,0);
	::close(srcFd);
	if(mmapPtr == (void *) -1){
		munmap(mmapPtr,filesize);
		output.retrieveAll();
		stateCode_ = 404;
		errorResponse(output,"HW_TXL can't find the file~");
		return;
	}
	char *srcAddr = static_cast<char *>(mmapPtr);
	output.append(srcAddr,filesize);

	munmap(mmapPtr,filesize);
}

void HttpResponse::errorResponse(Buffer &output,const std::string &message){
	std::string body;

	auto iter = stateCode_Message.find(stateCode_);
	if(iter == stateCode_Message.end()) 
		return ;
	
	body += "<html><title>HW_TXL Find AN Error</title>";
	body += "<body bgcolor=\"ffffff\">";
	body += std::to_string(stateCode_) + " : " +iter->second + "\n";
	body += "<p>" + message + "</p>";
	body += "<hr><em>HW_TXL web server</em></body></html>";
	
	//响应行 
	output.append("HTTP/1.1" + std::to_string(stateCode_) + " " + iter->second + "\r\n");

	//响应头 
	output.append("Server: HW_TXL\r\n");
	output.append("Content-type: text/html\r\n");
	output.append("Content-length:" + std::to_string(body.size()) + "\r\n");
	output.append("Connection: close\r\n");

	//响应正文 
	output.append(body);
}

std::string HttpResponse::__getFileType(){
	int idx = path_.find_last_of('.');
	//没有后缀 默认纯文本 
	if(idx == -1)
		return "text/plain";
	
	std::string suffix(path_.substr(idx));
	auto iter = suffix_Type.find(suffix);
	if(iter == suffix_Type.end())
		return "text/plain";
	
	return iter->second;
}
