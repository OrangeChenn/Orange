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

## 参考
视频地址：[C++高级教程（从零开始开发服务器框架）](https://www.bilibili.com/video/av53602631/?from=www.sylar.top&vd_source=675503aef6b8806b189e38ef9f181737)

GitHub：https://github.com/sylar-yin/sylar
