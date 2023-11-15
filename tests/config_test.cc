/*
 * @Author: Leo
 * @Date: 2023-08-02 10:43:44
 * @Description: 
 */
#include "../hiper/base/hiper.h"

#include <iostream>
#include <string>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>
using namespace std;

hiper::Logger::ptr g_logger = LOG_ROOT();

void print_yaml(const YAML::Node& node, int level)
{
    if (node.IsNull()) {
        LOG_INFO(g_logger) << std::string(level * 4, ' ') << "NULL - " << node.Type();
    }
    else if (node.IsScalar()) {
        LOG_INFO(g_logger) << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type() << "标量";
    }
    else if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            LOG_INFO(g_logger) << std::string(level * 4, ' ') << it->first << " - "
                               << it->second.Type() << "映射";
            print_yaml(it->second, level + 1);
        }
    }
    else if (node.IsSequence()) {
        for (size_t i = 0; i < node.size(); ++i) {
            LOG_INFO(g_logger) << std::string(level * 4, ' ') << i << " - " << node[i].Type() << "序列";
            print_yaml(node[i], level + 1);
        }
    }
    else {
        LOG_INFO(g_logger) << std::string(level * 4, ' ') << "ERROR - " << node.Type() << "错误";
    }
}



void test_config(int argc, char* argv[])
{
    hiper::ConfigVar<int>::ptr   g_int   = hiper::Config::Lookup("test.hi", (int)8080, "hi int");
    hiper::ConfigVar<float>::ptr g_float = hiper::Config::Lookup("test.hello", (float)15.4, "hi");

    g_int->addListener([](const int& old_value, const int& new_value) {
        LOG_INFO(g_logger) << "old_value = " << old_value << " new_value = " << new_value;
    });

    LOG_INFO(g_logger) << "before " << g_int->toString();
    LOG_INFO(g_logger) << "before " << g_float->toString();

    YAML::Node config = YAML::LoadFile("/home/leo/webserver/hiper/conf/test.yml");
    // 判断yaml节点类型并打印
    print_yaml(config, 0);
    // hiper::EnvMgr::GetInstance()->init(argc, argv);   // 解析命令行参数
    // // hiper::Config::LoadFromConfDir(hiper::EnvMgr::GetInstance()->getConfigPath());
    // // hiper::Config::LoadFromConfDir("/home/leo/webserver/hiper/hiper/conf/");
    // hiper::Config::LoadFromYaml(config);

    // LOG_INFO(g_logger) << "after " << g_int->toString();
    // LOG_INFO(g_logger) << "after " << g_float->toString();
}

int main(int argc, char* argv[])
{
    // // 读取 YAML 文件
    // YAML::Node config = YAML::LoadFile("/home/leo/webserver/hiper/bin/conf/log.yml");
    // // LOG_INFO(g_logger) << config;
    // print_yaml(config, 0);

    test_config(argc, argv);

    return 0;
}