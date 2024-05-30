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
## 模块介绍与实现
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
#define check(A, M, ...) if(!(A)) { /* log_err(M, ##__VA_ARGS__); */ errno=0; goto error; }

```


## 参考
视频地址：[C++高级教程（从零开始开发服务器框架）](https://www.bilibili.com/video/av53602631/?from=www.sylar.top&vd_source=675503aef6b8806b189e38ef9f181737)

GitHub：https://github.com/sylar-yin/sylar
