#include <iostream>
#include "../src/log.h"
#include "../src/util.h"

int main(int argc, char** argv) {
    orange::Logger::ptr logger(new orange::Logger);
    logger->addAppender(orange::LogAppender::ptr(new orange::StdoutAppender));
    orange::LogAppender::ptr file_appender(new orange::FileLogAppender("./log.txt"));
    orange::LogFormatter::ptr fmt(new orange::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    logger->addAppender(file_appender);

    file_appender->setLevel(orange::LogLevel::ERROR);

    ORANGE_LOG_LEVEL(logger, orange::LogLevel::DEBUG);

    ORANGE_LOG_DEBUG(logger);
    ORANGE_LOG_INFO(logger);

    ORANGE_LOG_FMT_INFO(logger, "CHEN %s ORANGE", "aa");

    ORANGE_LOG_ERROR(logger);

    ORANGE_LOG_FMT_ERROR(logger, "CHEN %s ORANGE", "aa");

    // orange::LogEvent::ptr event(new orange::LogEvent(__FILE__, __LINE__, 0, orange::GetThreadId(), orange::GetFiberId(), time(0)));
    // event->getSS() << "hello orange log";
    // logger->log(orange::LogLevel::Level::DEBUG, event);
    // std::cout << "hello orange" << std::endl;

    auto l = orange::LoggerMgr::GetInstance()->getLogger("cc");
    ORANGE_LOG_INFO(l) << "chen cheng";

    return 0;
}
