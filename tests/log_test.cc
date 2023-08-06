/*
 * @Author: Leo
 * @Date: 2023-07-30 00:51:07
 * @LastEditTime: 2023-08-06 09:57:34
 * @Description: 日志类测试
 */

#include "../hiper/base/log.h"

#include "../hiper/base/config.h"
#include "../hiper/base/env.h"
#include "../hiper/base/util.h"

#include <ctime>
#include <unistd.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

hiper::Logger::ptr g_logger = LOG_ROOT();   // 默认INFO级别，获取root日志器



void print_yaml(const YAML::Node& node, int level)
{
    if (node.IsScalar()) {
        LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type()
                             << " - " << level;
    }
    else if (node.IsNull()) {
        LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << "NULL - " << node.Type() << " - "
                             << level;
    }
    else if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << it->first << " - "
                                 << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    }
    else if (node.IsSequence()) {
        for (size_t i = 0; i < node.size(); ++i) {
            LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << i << " - " << node[i].Type()
                                 << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml()
{
    YAML::Node root = YAML::LoadFile("/home/leo/webserver/hiper/bin/conf/log.yml");
    // print_yaml(root, 0);
    // LOG_INFO(LOG_ROOT()) << root.Scalar();

    LOG_INFO(LOG_ROOT()) << root["test"].IsDefined();
    LOG_INFO(LOG_ROOT()) << root["logs"].IsDefined();
    LOG_INFO(LOG_ROOT()) << root;
}



void test_log()
{
    hiper::Config::printdata();
    static hiper::Logger::ptr system_log = LOG_NAME("system");
    system_log->addAppender(hiper::LogAppender::ptr(new hiper::StdoutLogAppender()));
    // system_log->addAppender(
    //     hiper::LogAppender::ptr(new hiper::FileLogAppender("../logs/system_log.txt")));
    LOG_INFO(system_log) << "hello system" << std::endl;


    std::cout << hiper::LoggerMgr::GetInstance()->toYamlString() << std::endl;


    YAML::Node root = YAML::LoadFile("/home/leo/webserver/hiper/bin/conf/log.yml");
    hiper::Config::LoadFromYaml(root);

    std::cout << "=============hi" << std::endl;
    hiper::Config::printdata();
    std::cout << "=============" << std::endl;
    std::cout << hiper::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    std::cout << "=============" << std::endl;
    std::cout << root << std::endl;
    LOG_INFO(system_log) << "hello system" << std::endl;

    // system_log->setFormatter("%d - %m%n");
    LOG_INFO(system_log) << "hello system" << std::endl;
}

void test_loadconf()
{
    hiper::Config::LoadFromConfDir("conf");
}



int main(int argc, char* argv[])
{
    hiper::EnvMgr::GetInstance()->init(argc, argv);   // 解析命令行参数
    hiper::Config::LoadFromConfDir(
        hiper::EnvMgr::GetInstance()->getConfigPath());   // 加载yml配置文件

    // root logger 打印到控制台
    LOG_FATAL(g_logger) << "fatal msg";
    LOG_ERROR(g_logger) << "err msg";
    LOG_INFO(g_logger) << "info msg";
    LOG_DEBUG(g_logger) << "debug msg";   // 不打印

    LOG_FMT_FATAL(g_logger, "fatal %s:%d", __FILE__, __LINE__);
    LOG_FMT_ERROR(g_logger, "err %s:%d", __FILE__, __LINE__);
    LOG_FMT_INFO(g_logger, "info %s:%d", __FILE__, __LINE__);
    LOG_FMT_DEBUG(g_logger, "debug %s:%d", __FILE__, __LINE__);

    sleep(1);
    hiper::SetThreadName("brand_new_thread");

    g_logger->setLevel(hiper::LogLevel::WARN);
    LOG_FATAL(g_logger) << "fatal msg";
    LOG_ERROR(g_logger) << "err msg";
    LOG_INFO(g_logger) << "info msg";     // 不打印
    LOG_DEBUG(g_logger) << "debug msg";   // 不打印


    hiper::FileLogAppender::ptr fileAppender(new hiper::FileLogAppender("./log.txt"));
    g_logger->addAppender(fileAppender);
    LOG_FATAL(g_logger) << "fatal msg";
    LOG_ERROR(g_logger) << "err msg";
    LOG_INFO(g_logger) << "info msg";     // 不打印
    LOG_DEBUG(g_logger) << "debug msg";   // 不打印

    hiper::Logger::ptr test_logger = LOG_NAME("test_logger");

    hiper::StdoutLogAppender::ptr appender(new hiper::StdoutLogAppender);
    hiper::LogFormatter::ptr      formatter(new hiper::LogFormatter(
        "%d:%rms%T%p%T%c%T%f:%l %m%n"));   // 时间：启动毫秒数 级别 日志名称 文件名：行号 消息 换行
    appender->setFormatter(formatter);

    test_logger->addAppender(appender);
    test_logger->setLevel(hiper::LogLevel::WARN);

    LOG_ERROR(test_logger) << "err msg";
    LOG_INFO(test_logger) << "info msg";   // 不打印

    // 输出全部日志器的配置
    g_logger->setLevel(hiper::LogLevel::INFO);
    LOG_INFO(g_logger) << "logger config:" << hiper::LoggerMgr::GetInstance()->toYamlString();


    // =========================测试日志管理器==============================
    auto                        l = hiper::LoggerMgr::GetInstance()->getLogger("xx");
    hiper::FileLogAppender::ptr file_appender(new hiper::FileLogAppender("./xx_log.txt"));
    l->addAppender(file_appender);

    LOG_ERROR(l) << "l err msg";

    LOG_INFO(l) << "xx logger";

    return 0;
}