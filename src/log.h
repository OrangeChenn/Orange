#pragma once

#include <stdint.h>

#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

#include "thread.h"
#include "singleton.h"
#include "util.h"

#define ORANGE_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        orange::LogEventWrap(std::shared_ptr<orange::LogEvent>(new orange::LogEvent(logger, level, \
                 __FILE__, __LINE__, 0, orange::GetThreadId(), \
                orange::GetFiberId(), time(0), orange::Thread::GetName()))).getSS()

#define ORANGE_LOG_DEBUG(logger) ORANGE_LOG_LEVEL(logger, orange::LogLevel::DEBUG)
#define ORANGE_LOG_INFO(logger) ORANGE_LOG_LEVEL(logger, orange::LogLevel::INFO)
#define ORANGE_LOG_WARN(logger) ORANGE_LOG_LEVEL(logger, orange::LogLevel::WARN)
#define ORANGE_LOG_ERROR(logger) ORANGE_LOG_LEVEL(logger, orange::LogLevel::ERROR)
#define ORANGE_LOG_FATAL(logger) ORANGE_LOG_LEVEL(logger, orange::LogLevel::FATAL)

#define ORANGE_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        orange::LogEventWrap(std::shared_ptr<orange::LogEvent>(new orange::LogEvent(logger, level, \
                __FILE__, __LINE__, 0, orange::GetThreadId(), \
                orange::GetFiberId(), time(0), orange::Thread::GetName()))).getEvent()->format(fmt, __VA_ARGS__)

#define ORANGE_LOG_FMT_DEBUG(logger, fmt, ...) ORANGE_LOG_FMT_LEVEL(logger, orange::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define ORANGE_LOG_FMT_INFO(logger, fmt, ...) ORANGE_LOG_FMT_LEVEL(logger, orange::LogLevel::INFO, fmt, __VA_ARGS__)
#define ORANGE_LOG_FMT_WARN(logger, fmt, ...) ORANGE_LOG_FMT_LEVEL(logger, orange::LogLevel::WARN, fmt, __VA_ARGS__)
#define ORANGE_LOG_FMT_ERROR(logger, fmt, ...) ORANGE_LOG_FMT_LEVEL(logger, orange::LogLevel::ERROR, fmt, __VA_ARGS__)
#define ORANGE_LOG_FMT_FATAL(logger, fmt, ...) ORANGE_LOG_FMT_LEVEL(logger, orange::LogLevel::FATAL, fmt, __VA_ARGS__)

#define ORANGE_LOG_ROOT() orange::LoggerMgr::GetInstance()->getRoot()
#define ORANGE_LOG_NAME(name) orange::LoggerMgr::GetInstance()->getLogger(name)

namespace orange {

class Logger;

// 日志级别
class LogLevel {
public:
    enum Level {
        UNKNOW = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5,
    };

    static const char* ToString(LogLevel::Level level);
    static LogLevel::Level FromString(const std::string& level);
};

// 日志事件
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t line, uint32_t elapse 
        , uint32_t thread_id, uint32_t fiber_id, uint64_t time, const std::string& threadName);

    std::shared_ptr<Logger> getLogger() const { return m_logger; }
    LogLevel::Level getLevel() const { return m_level; }
    const char* getFile() const { return m_file; }
    int32_t getLine() const { return m_line; }
    uint32_t getElapse() const { return m_elapse; }
    uint32_t getThreadId() const { return m_threadId; }
    const std::string& getThreadName() const { return m_threadName; }
    uint32_t getFiberId() const { return m_fiberId; }
    uint64_t getTime() const { return m_time; }
    std::string getContent() const { return m_ss.str(); }
    std::stringstream& getSS() { return m_ss; }

    void format(const char* fmt, ...);
    void format(const char* fmt, va_list al);
private:
    std::shared_ptr<Logger> m_logger;
    LogLevel::Level m_level;
    const char* m_file = nullptr;   // 文件名
    int32_t m_line = 0;             // 行号
    uint32_t m_elapse = 0;          // 程序启动开始到现在的毫秒数
    uint32_t m_threadId = 0;        // 线程id
    std::string m_threadName;       // 线程名称
    uint32_t m_fiberId = 0;         // 协程id
    uint64_t m_time = 0;            // 时间戳
    std::stringstream m_ss;
};

// 日志事件包装器
class LogEventWrap {
public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();
    LogEvent::ptr getEvent() { return m_event; }
    std::stringstream& getSS() { return m_event->getSS(); }
private:
    LogEvent::ptr m_event;
};

// 日志格式器
class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string& pattern);
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
    std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
    bool isError() const { return m_error; }
    const std::string getPattern() { return m_pattern; }

public:
    class FormatItem {
        public:
        typedef std::shared_ptr<FormatItem> ptr;
        // virtual ~FormatItem() {}
        virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };

    void init();
private:
    bool m_error = false;
    std::string m_pattern;
    std::vector<FormatItem::ptr> m_items;
};

// 日志输出地
class LogAppender {
friend class Logger;
public:
    typedef std::shared_ptr<LogAppender> ptr;
    typedef orange::SpinLock MutexType;
    // virtual ~LogAppender();

    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    virtual std::string toYamlString() = 0;

    void setFormatter(LogFormatter::ptr formatter);
    LogFormatter::ptr getFormatter() const;
    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level level) { m_level = level;}
    bool hasFormatter() const { return m_has_formatter; }

protected:
    LogLevel::Level m_level = LogLevel::Level::DEBUG;
    LogFormatter::ptr m_formatter;
    mutable MutexType m_mutex;
    bool m_has_formatter = false;
};

// 日志器
class Logger : public std::enable_shared_from_this<Logger> {
friend class LoggerManager;
public:
    typedef std::shared_ptr<Logger> ptr;
    typedef orange::SpinLock MutexType;

    Logger(const std::string& name = "root");
    void log(LogLevel::Level level, LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    void clearAppender();
    LogFormatter::ptr getFormatter();
    void setFormatter(LogFormatter::ptr formatter);
    void setFormatter(const std::string& fmt);
    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level level) { m_level = level; }
    std::string getName() const { return m_name; }
    void setName(const std::string name) { m_name = name; }

    std::string toYamlString();
private:
    std::string m_name;                     // 日志名称
    LogLevel::Level m_level;                // 日志级别
    MutexType m_mutex;
    std::list<LogAppender::ptr> m_appenders;// Appender集合
    LogFormatter::ptr m_formatter;
    Logger::ptr m_root;
};

// 输出到控制台的Appender
class StdoutAppender : public LogAppender {
friend class Logger;
public:
    typedef std::shared_ptr<StdoutAppender> ptr;
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;
};

// 输出到文件的Appender
class FileLogAppender : public LogAppender {
friend class Logger;
public:
    FileLogAppender(const std::string& filename);
    typedef std::shared_ptr<FileLogAppender> ptr;
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;

    bool reopen();

private:
    std::string m_filename;
    std::ofstream m_filestream;
    uint64_t m_lastTime = 0;
};

class LoggerManager {
public:
    typedef orange::SpinLock MutexType;
    LoggerManager();

    Logger::ptr getLogger(const std::string& name);
    Logger::ptr getRoot() { return m_root; }

    std::string toYamlString();

    void init();
private:
    std::map<std::string, Logger::ptr> m_loggers;
    MutexType m_mutex;
    Logger::ptr m_root;
};

typedef orange::Singleton<LoggerManager> LoggerMgr;

}
