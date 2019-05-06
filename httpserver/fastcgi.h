//
// Created by zuhxs on 2019/5/3.
//

#ifndef HTTPSERVER_FASTCGI_H
#define HTTPSERVER_FASTCGI_H

#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include "http.h"

#define FCGI_HOST "127.0.0.1"
#define FCGI_PORT 9000

#define FCGI_HEADER_LEN     8       // 协议包头长度
/*
 * 可用于FCGI_Header的type组件的值
 */
#define FCGI_BEGIN_REQUEST  1   // 请求开始记录类型
#define FCGI_ABORT_REQUEST  2
#define FCGI_END_REQUEST    3   // 响应结束记录类型
#define FCGI_PARAMS         4   // 传输名值对数据
#define FCGI_STDIN          5   // 传输输入数据，例如post数据
#define FCGI_STDOUT         6   // php-fpm响应数据输出
#define FCGI_STDERR         7   // php-fpm错误输出
#define FCGI_DATA           8

#define FCGI_VERSION_1  1               // fastcgi协议版本
#define FCGI_MAX_LENGTH 0xFFFF          // 允许传输的最大数据长度65536

/*
 * 期望php-fpm扮演的角色值
 */
#define FCGI_RESPONDER  1
#define FCGI_AUTHORIZER 2
#define FCGI_FILTER     3

// 为1，表示php-fpm响应结束不会关闭该请求连接
#define FCGI_KEEP_CONN  1


typedef struct {
    unsigned char version;          // 版本
    unsigned char type;             // 协议记录类型
    unsigned char requestIdB1;      // 请求ID
    unsigned char requestIdB0;
    unsigned char contentLengthB1;  // 内容长度
    unsigned char contentLengthB0;
    unsigned char paddingLength;    // 填充字节长度
    unsigned char reserved;         // 保留字节
} FCGI_Header;


typedef struct {
    unsigned char roleB1;   // web服务器期望php-fpm扮演的角色
    unsigned char roleB0;
    unsigned char flags;    // 控制连接响应后是否立即关闭
    unsigned char reserved[5];
} FCGI_BeginRequestBody;

typedef struct {
    FCGI_Header header;
    FCGI_BeginRequestBody body;
} FCGI_BeginRequestRecord;

typedef struct {
    unsigned char appStatusB3;
    unsigned char appStatusB2;
    unsigned char appStatusB1;
    unsigned char appStatusB0;
    unsigned char protocolStatus;   // 协议级别的状态码
    unsigned char reserved[3];
} FCGI_EndRequestBody;


typedef struct {
    FCGI_Header header;
    FCGI_EndRequestBody body;
} FCGI_EndRequestRecord;

typedef struct {
    FCGI_Header header;
    unsigned char nameLength;
    unsigned char valueLength;
    unsigned char data[0];
} FCGI_ParamsRecord;

class FASTCGI {
private:
    int CGI_sock;
    static int request_id;
    int current_request_id;

public:
    FASTCGI();
    void fatal_error(const char *message) const;
    void request_cgi(HTTP *hp);
    void start_comm();
    FCGI_Header *make_header(unsigned char type, int contentLength, unsigned char paddingLength);
    FCGI_BeginRequestBody *make_BeginRequestBody(int role, int keepCoon);
    void sendParamsRecord(char *name, int nlen, char *value, int vlen);
    void sendEmptyParamsRecord();
    char *recvRecord();
};


#endif //HTTPSERVER_FASTCGI_H
