#include "log.h"

#include <iostream>
#include <map>
#include <functional>
#include <stdarg.h>
#include <stdio.h>

#include "config.h"

namespace orange {

const char* LogLevel::ToString(LogLevel::Level level) {
    switch (level)
    {
#define XX(name) \
    case LogLevel::name: \
        return #name; \
        break;

    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    default:
        return "UNKNOW";
    }
    return "UNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string& str) {
#define XX(level, val) \
    if(str == #val) { \
        return LogLevel::Level::level; \
    }

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);

    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);
#undef XX
    return LogLevel::Level::UNKNOW;
}

class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string& str = "") {}
    virtual void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string& str = "") {}
    virtual void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << LogLevel::ToString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string& str = "") {}
    virtual void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(const std::string& str = "") {}
    virtual void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLogger()->getName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string& str = "") {}
    virtual void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string& str = "") {}
    virtual void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S") 
        : m_format(format) {
        if(m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }
    virtual void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }

private:
    std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
    FilenameFormatItem(const std::string& str = "") {}
    virtual void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string& str = "") {}
    virtual void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string& str = "") {}
    virtual void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str = "") 
        : m_string(str) {

    }
    virtual void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << m_string;
    }
private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string& str = "") {}
    virtual void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << "\t";
    }
};

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t line, uint32_t elapse 
        , uint32_t thread_id, uint32_t fiber_id, uint64_t time) 
    :m_logger(logger)
    ,m_level(level)
    ,m_file(file)
    ,m_line(line)
    ,m_elapse(elapse)
    ,m_threadId(thread_id)
    ,m_fiberId(fiber_id)
    ,m_time(time)      {}

LogEventWrap::LogEventWrap(LogEvent::ptr e) 
    : m_event(e) {}

LogEventWrap::~LogEventWrap() {
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}

void LogEvent::format(const char* fmt, ...) {
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}

void LogEvent::format(const char* fmt, va_list al) {
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    if(len != -1) {
        m_ss << std::string(buf, len);
        free(buf);
    }
}

Logger::Logger(const std::string& name)
    : m_name(name), m_level(LogLevel::Level::DEBUG) {
    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::addAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex);
    if(!appender->getFormatter()) {
        MutexType::Lock lock(appender->m_mutex);
        appender->m_formatter = m_formatter;
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex);
    for(auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
        if(*it == appender) {
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppender() {
    MutexType::Lock lock(m_mutex);
    m_appenders.clear();
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
    MutexType::Lock lock(m_mutex);
    if(level >= m_level) {
        auto self = shared_from_this();
        if(!m_appenders.empty()) {
            for(auto& i : m_appenders) {
                i->log(self, level, event);
            }
        } else if(m_root) {
            m_root->log(level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event) {
    log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event) {
    log(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::ptr event) {
    log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event) {
    log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event) {
    log(LogLevel::FATAL, event);
}

LogFormatter::ptr Logger::getFormatter() {
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}

void Logger::setFormatter(LogFormatter::ptr formatter) {
    MutexType::Lock lock(m_mutex);
    m_formatter = formatter;
    for(auto i : m_appenders) {
        if(!i->m_has_formatter) {
            MutexType::Lock lock(i->m_mutex);
            i->m_formatter = formatter;
        }
    }
}
void Logger::setFormatter(const std::string& fmt){
    orange::LogFormatter::ptr new_fmt(new orange::LogFormatter(fmt));
    if(new_fmt->isError()) {
        std::cout << "Logger setFormatter name = " << m_name
                  << " value = " << fmt << " invalid formatter"
                  << std::endl;
        return;
    }
    setFormatter(new_fmt);
}

std::string Logger::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["name"] = m_name;
    node["level"] = LogLevel::ToString(m_level);
    if(m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    for(auto& i : m_appenders) {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void LogAppender::setFormatter(LogFormatter::ptr formatter) {
    MutexType::Lock lock(m_mutex);
    m_formatter = formatter;
    if(m_formatter) {
        m_has_formatter = true;
    } else {
        m_has_formatter = false;
    }
}

LogFormatter::ptr LogAppender::getFormatter() const {
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}

void StdoutAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
    if(level >= m_level) {
        m_formatter->format(std::cout, logger, level, event);
    }
}

std::string StdoutAppender::toYamlString() {
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    node["level"] = LogLevel::ToString(m_level);
    {
        MutexType::Lock lock(m_mutex);
        if(m_has_formatter && m_formatter) {
            node["formatter"] = m_formatter->getPattern();
        }
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

FileLogAppender::FileLogAppender(const std::string& filename) 
    : m_filename(filename) {
    reopen();
}

void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
    MutexType::Lock lock(m_mutex);
    uint64_t now = time(0);
    if(m_lastTime != now) {
        reopen();
        m_lastTime = now;
    }
    if(m_level <= level) {
        m_filestream << m_formatter->format(logger, level, event);
    }
}

bool FileLogAppender::reopen() {
    MutexType::Lock lock(m_mutex);
    if(m_filestream) {
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream;
}

std::string FileLogAppender::toYamlString() {
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["level"] = LogLevel::ToString(m_level);
    {
        MutexType::Lock lock(m_mutex);
        if(m_has_formatter && m_formatter) {
            node["formatter"] = m_formatter->getPattern();
        }
    }
    node["file"] = m_filename;
    std::stringstream ss;
    ss << node;
    return ss.str();
}

LogFormatter::LogFormatter(const std::string& pattern) 
    : m_pattern(pattern) {
    init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    std::stringstream ss;
    for(auto& i : m_items) {
        i->format(ss, logger, level, event);
    }
    return ss.str();
}

std::ostream& LogFormatter::format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    for(auto& i : m_items) {
        i->format(ofs, logger, level, event);
    }
    return ofs;
}

// %xxx %xxx{xxx} %%
// %d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n
void LogFormatter::init() {
    // str, format, type
    std::vector<std::tuple<std::string, std::string, int>> vec;
    std::string nstr;
    for(size_t i = 0; i < m_pattern.size(); ++i) {
        if(m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }
        if((i + 1) < m_pattern.size()) {
            if(m_pattern[i + 1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }

        size_t n = i + 1;
        size_t fmt_begin = 0;
        int fmt_status = 0;

        std::string str;
        std::string fmt;
        while(n < m_pattern.size()) {
            if(!fmt_status && (!isalpha(m_pattern[n])) && m_pattern[n] != '{'
                    && m_pattern[n] != '}') {
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }

            if(fmt_status == 0) {
                if(m_pattern[n] == '{') {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    fmt_status = 1;
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            } else if(fmt_status == 1) {
                if(m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if(n == m_pattern.size()) {
                if(str.empty()) {
                    str = m_pattern.substr(i + 1);
                }
            }
        }

        if(fmt_status == 0) {
            if(!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, "", 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        } else if(fmt_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }
    if(!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }

    static std::map<std::string, std::function<FormatItem::ptr(const std::string& std)> > s_format_items = {
#define XX(str, C) \
        {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt)); }}

        XX(m, MessageFormatItem),
        XX(p, LevelFormatItem),
        XX(r, ElapseFormatItem),
        XX(c, NameFormatItem),
        XX(t, ThreadIdFormatItem),
        XX(n, NewLineFormatItem),
        XX(d, DateTimeFormatItem),
        XX(f, FilenameFormatItem),
        XX(l, LineFormatItem),
        XX(T, TabFormatItem),
        XX(F, FiberIdFormatItem),
        // "%d [%p] %f %l %m %n"
#undef XX
    };

    for(auto& i : vec) {
        if(std::get<2>(i) == 0) {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        }
        if(std::get<2>(i) == 1) {
            auto it = s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_pattern %" + std::get<0>(i) + ">>")));
                m_error = true;
            } else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
    }
}

LoggerManager::LoggerManager() {
    m_root.reset(new Logger);
    m_root->addAppender(std::shared_ptr<LogAppender>(new StdoutAppender));
    m_loggers[m_root->m_name] = m_root;

    init();
}

Logger::ptr LoggerManager::getLogger(const std::string& name) {
    MutexType::Lock lock(m_mutex);
    auto it = m_loggers.find(name);
    if(it != m_loggers.end()) {
        return it->second;
    }

    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}

std::string LoggerManager::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    for(auto& i : m_loggers) {
        node["logs"].push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

struct LogAppenderDefine {
    int type = 0; // 1 file, 2 stdout
    LogLevel::Level level = LogLevel::Level::UNKNOW;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine& oth) const {
        return type == oth.type
            && level == oth.level
            && formatter == oth.formatter
            && file == oth.file;
    }
};

struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::Level::UNKNOW;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const {
        return name == oth.name
            && level == oth.level
            && formatter == oth.formatter
            && appenders == oth.appenders;
    }

    bool operator<(const LogDefine& oth) const {
        return name < oth.name;
    }
};

template<>
class LexicalCast<std::string, LogDefine> {
public:
    orange::LogDefine operator()(const std::string& val) {
        orange::LogDefine ld;
        YAML::Node node = YAML::Load(val);
        if(!node["name"].IsDefined()) {
            std::cout << "log config error: name is null, n = " << node 
                      << std::endl;
            throw std::logic_error("log config error, name is null");
        }
        ld.name = node["name"].as<std::string>();
        ld.level = LogLevel::FromString(node["level"].IsDefined() ? node["level"].as<std::string>() : "");
        if(node["formatter"].IsDefined()) {
            ld.formatter = node["formatter"].as<std::string>();
        }
        if(node["appenders"].IsDefined()) {
            for(size_t i = 0; i < node["appenders"].size(); ++i) {
                auto a = node["appenders"][i];
                if(!a["type"].IsDefined()) {
                    std::cout << "log config error: type is null, a = " << a
                              << std::endl;
                    continue;
                }
                std::string type = a["type"].as<std::string>();
                LogAppenderDefine lad;
                if(type == "FileLogAppender") {
                    lad.type = 1;
                    if(!a["file"].IsDefined()) {
                        std::cout << "log config error: appender file is null, a = " << a
                                  << std::endl;
                        continue;
                    }
                    lad.file = a["file"].as<std::string>();
                } else if(type == "StdoutLogAppender") {
                    lad.type = 2;
                } else {
                    std::cout << "log config error: appender is invalid, a = " << a
                              << std::endl;
                    continue;
                }
                if(a["formatter"].IsDefined()) {
                    lad.formatter = a["formatter"].as<std::string>();
                }
                lad.level = LogLevel::FromString(a["level"].IsDefined() ? a["level"].as<std::string>() : "");
                ld.appenders.push_back(lad);
            }
        }
        return ld;
    }
};

template<>
class LexicalCast<LogDefine, std::string> {
public:
    std::string operator()(const LogDefine& val) {
        YAML::Node node;
        node["name"] = val.name;
        if(val.level != LogLevel::Level::UNKNOW) {
            node["level"] = LogLevel::ToString(val.level);
        }
        if(!val.formatter.empty()) {
            node["formatter"] = val.formatter;
        }
        for(auto& a : val.appenders) {
            YAML::Node na;
            if(a.type == 1) {
                na["type"] = "FileLogAppender";
                na["file"] = a.file;
            } else if(a.type == 2) {
                na["type"] = "StdoutAppender";
            }
            if(a.level != LogLevel::Level::UNKNOW) {
                na["level"] = LogLevel::ToString(a.level);
            }
            if(!a.formatter.empty()) {
                na["formatter"] = a.formatter;
            }
            node.push_back(na);
        }
        std::stringstream ss;
        ss << node;
        return ss.str(); 
    }
};

orange::ConfigVar<std::set<LogDefine> >::ptr g_log_defines = orange::Config::Lookup(
        "logs", std::set<LogDefine>(), "logs define");

struct LogIniter {
    LogIniter() {
        g_log_defines->addListener(0xF1E231, [](const std::set<LogDefine>& old_val,
                const std::set<LogDefine>& new_val) {
            ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << "on_logger_conf_change";
            for(auto& i : new_val) {
                orange::Logger::ptr logger;
                auto it = old_val.find(i);
                if(it == old_val.end()) {
                    logger = ORANGE_LOG_NAME(i.name);
                } else {
                    if(!(i == *it)) {
                        logger = ORANGE_LOG_NAME(i.name);
                    } else {
                        continue;
                    }
                }
                logger->setLevel(i.level);
                if(!i.formatter.empty()) {
                    orange::LogFormatter::ptr formatter(new orange::LogFormatter(i.formatter));
                    logger->setFormatter(formatter);
                    
                }
                logger->clearAppender();
                for(auto& a : i.appenders) {
                    orange::LogAppender::ptr ap;
                    if(a.type == 1) {
                        ap.reset(new orange::FileLogAppender(a.file));
                    } else if(a.type == 2) {
                        ap.reset(new orange::StdoutAppender());
                    }
                    ap->setLevel(a.level);

                    // orange::LogFormatter::ptr formatter(new orange::LogFormatter(a.formatter));
                    // if(!formatter->isError()) {
                    //     ap->setFormatter(formatter);
                    // }
                    if(!a.formatter.empty()) {
                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                        if(!fmt->isError()) {
                            ap->setFormatter(fmt);
                        } else {
                            std::cout << "log.name=" << i.name << " appender type=" << a.type
                                        << " formatter=" << a.formatter << " is invalid" << std::endl;
                        }
                    }

                    logger->addAppender(ap);
                }
            }

            for(auto& i : old_val) {
                auto it = new_val.find(i);
                if(it == new_val.end()) {
                    auto logger = ORANGE_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level)100);
                    logger->clearAppender();
                }
            }
        });
    }
};

static LogIniter __log_init;

void LoggerManager::init() {

}

}
