#include "../hiper/base/hiper.h"

#include <iostream>
#include <string>
#include <yaml-cpp/yaml.h>



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

int main(int argc, char** argv)
{
    // test_yaml();
    // test_config();
    // test_class();
    test_log();
    // hiper::EnvMgr::GetInstance()->init(argc, argv);
    // test_loadconf();
    // std::cout << " ==== " << std::endl;
    // test_loadconf();
    // return 0;
    // hiper::Config::Visit([](hiper::ConfigVarBase::ptr var) {
    //     LOG_INFO(LOG_ROOT()) << "name=" << var->getName()
    //                 << " description=" << var->getDescription()
    //                 << " typename=" << var->getTypeName()
    //                 << " value=" << var->toString();
    // });

    return 0;
}
