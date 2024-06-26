# Orange
## C++高性能服务器框架

学习项目

## 相关依赖库安装
```shell
# yaml-cpp
sudo apt-get install libyaml-cpp-dev

# boost
sudo apt-get install libboost-all-dev

```
## 模块介绍与实现(基础篇)
### 日志模块
用于格式化输出程序日志，方便定位程序运行时的问题。
日志级别
```C++
enum Level{
    //  未知 级别
    UNKNOW = 0,
    //  DEBUG 级别
    DEBUG = 1,
    //  INFO 级别
    INFO = 2,
    //  WARN 级别
    WARN = 3,
    //  ERROR 级别
    ERROR = 4,
    //  FATAL 级别
    FATAL = 5
};
```
日志格式化
```c++
%m 消息
%p 日志级别
%r 累计毫秒数
%c 日志名称
%t 线程id
%n 换行
%d 时间
%f 文件名
%l 行号
%T 制表符
%F 协程id
%N 线程名称
默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"

```
### 配置模块

### 线程模块

### 协程模块

### 协程调度模块

### IO协程调度模块

### 定时器模块

### HOOK模块

### Address模块

### Socket模块

### 序列化模块

### HTTP模块
**http/1.1解析**
```shell
# 下载地址 https://github.com/mongrel2/mongrel2/tree/master/src/http11
# http11_common.h,http11_parser.h,httpclient_parser.h,http11_parser.rl,httpclient_parser.rl
# 生成cc文件
ragel -G2 -C http11_parser.rl -o http11_parser.rl.cc
ragel -G2 -C httpclient_parser.rl -o httpclient_parser.rl.cc

```

编译不通过
```C
//  编译不通过，去掉 #include "dbg.h" 头文件，添加宏定义
#include <errno.h>
#define check(A, M, ...) if(!(A)) { /* log_err(M, ##__VA_ARGS__); */ errno=0; goto error; }

// 使用的解析器默认是解析完整的请求，而我们的代码里支持分段解析
// http_parser_execute执行时需要重置部分变量，在http11_parser.rl中的http_parser_execute函数添加
parser->nread = 0;
parser->mark = 0;
parser->field_len = 0;
parser->field_start = 0;

```

### Socket流模块

### HttpServer模块

### Servlet模块

### HttpConnection模块
http测试工具PostMan

## 系统篇
### 性能测试
安装性能测试工具
```shell
sudo apt install httpie
```
压测服务器
my_http_server
```shell
# 测试命令
 ab -n 1000000 -c 200 "http://10.1.20.3:8020/orange"
# 测试结果（1线程）
Server Software:        orange/1.0.0
Server Hostname:        10.1.20.3
Server Port:            8020

Document Path:          /orange
Document Length:        139 bytes

Concurrency Level:      200
Time taken for tests:   91.650 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Total transferred:      245000000 bytes
HTML transferred:       139000000 bytes
Requests per second:    10911.09 [#/sec] (mean)
Time per request:       18.330 [ms] (mean)
Time per request:       0.092 [ms] (mean, across all concurrent requests)
Transfer rate:          2610.56 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   1.1      1      21
Processing:     4   17   2.3     17      51
Waiting:        1   17   2.3     17      51
Total:          8   18   2.3     18      52

Percentage of the requests served within a certain time (ms)
  50%     18
  66%     18
  75%     18
  80%     18
  90%     19
  95%     20
  98%     25
  99%     30
 100%     52 (longest request)
```
```shell
# 测试命令
 ab -n 1000000 -c 200 "http://10.1.20.3:8020/orange"
# 测试结果（2线程）
Server Software:        orange/1.0.0
Server Hostname:        10.1.20.3
Server Port:            8020

Document Path:          /orange
Document Length:        139 bytes

Concurrency Level:      200
Time taken for tests:   91.036 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Total transferred:      252000000 bytes
HTML transferred:       139000000 bytes
Requests per second:    10984.63 [#/sec] (mean)
Time per request:       18.207 [ms] (mean)
Time per request:       0.091 [ms] (mean, across all concurrent requests)
Transfer rate:          2703.25 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   0.8      1      20
Processing:     2   17   1.4     17      38
Waiting:        1   17   1.4     17      38
Total:          8   18   1.3     18      41

Percentage of the requests served within a certain time (ms)
  50%     18
  66%     18
  75%     18
  80%     18
  90%     19
  95%     19
  98%     23
  99%     25
 100%     41 (longest request)
```
nginx压测
```shell
# 测试命令
 ab -n 1000000 -c 200 "http://10.1.20.3:80/orange"
# 测试结果（2线程）
Server Software:        nginx/1.18.0
Server Hostname:        10.1.20.3
Server Port:            80

Document Path:          /orange
Document Length:        162 bytes

Concurrency Level:      200
Time taken for tests:   86.409 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Total transferred:      321000000 bytes
HTML transferred:       162000000 bytes
Requests per second:    11572.91 [#/sec] (mean)
Time per request:       17.282 [ms] (mean)
Time per request:       0.086 [ms] (mean, across all concurrent requests)
Transfer rate:          3627.84 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    7   2.1      7      37
Processing:     3   10   2.3     10      45
Waiting:        0    8   2.3      8      35
Total:          9   17   3.6     17      67

Percentage of the requests served within a certain time (ms)
  50%     17
  66%     17
  75%     17
  80%     17
  90%     18
  95%     21
  98%     32
  99%     34
 100%     67 (longest request)
```

### 守护进程
创建两个进程，子进程执行任务，主进程监控子进程状态，子进程异常退出时，主进程重新启动子进程执行任务。
```c++
int daemon(int nochdir, int noclose);
```
第一个参数：如果nochdir参数为1，则表示在调用daemon()函数之后不改变当前工作目录（working directory）为根目录（/），否则会将当前工作目录改变为根目录。将工作目录切换到根目录有助于确保不占用任何挂载的文件系统。
第二个参数：如果noclose参数为1，则表示在调用daemon()函数之后不关闭标准输入、标准输出和标准错误流（stdin、stdout、stderr），否则会关闭这些文件描述符。关闭文件描述符可以防止守护进程意外地从控制台输出或输入数据。

daemon(1, 0)的作用是将当前进程转变为守护进程，守护进程通常运行在后台，独立于终端会话，没有用户交互。它们通常用于在后台执行一些服务或任务。在转变为守护进程之后，进程会执行以下操作：
- 脱离控制终端，使得守护进程不会受到终端信号的影响。
- 在后台运行，不再与终端关联。
- 改变当前工作目录为根目录。
- 关闭标准输入、标准输出和标准错误流
### 启动参数解析

### 环境变量
1.读写环境变量
2.获取程序的绝对路径，基于绝对路径设置cwd
3.可以通过cmdline，在main函数之前解析好参数

getenv
setenv
```shell
/proc/pid

cwd # 进程启动路径
exe # 二进制程序真正执行路径
cmdline #命令行参数，和全局构造函数，在main函数之前就可以解析出参数
```
```c++
// 把软连接转成真正的路径
ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz);
```
### 配置加载

### Server主体框架
1.防止重复启动多次（pid）
2.初始化日志文件路径（/path/to/log）
3.工作目录的路径。（/path/to/work）
4.解析httpserver配置，通过配置，启动httpserver

## 聊天室
### httpserver
支持文件访问，流量访问，index.html

### websocket server
1.普通的http实现，需要周期性轮询
2.websocket 长连接，服务器可以主动推送数据，效率高

### 协议设计
协议格式：{type:"协议类型"， data: {}}
```
1.login协议
- Client: {type: "login_request", data: {"name": "your name"}}
- Server: {type: "login_response", result: 200, msg: "ok"}

2.send协议
- Client: {type: "send_request", data: {"msg" : "massage"}}
- Server: {type: "send_response", result: 200, msg: "ok"}

3.user_enter 通知
- {type: "user_enter", msg: "xxx加入聊天室"}

4.user_leave 通知
- {type: "user_leave", msg: "xxx离开聊天室"}

5.msg 通知
- {type: "msg", msg: "具体聊天信息", user: "xxx", time: xxxx}
```
## 参考
视频地址：[C++高级教程（从零开始开发服务器框架）](https://www.bilibili.com/video/av53602631/?from=www.sylar.top&vd_source=675503aef6b8806b189e38ef9f181737)

GitHub：https://github.com/sylar-yin/sylar
