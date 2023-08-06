#include "../hiper/base/hiper.h"
#include <yaml-cpp/node/node.h>

using namespace std;

hiper::Logger::ptr logger = LOG_ROOT();

hiper::Mutex mutex;

void func1(){
    while (1) {
        // hiper::Mutex::Lock lock(mutex);
        LOG_INFO(logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void func2(){
    while (1) {
        // hiper::Mutex::Lock lock(mutex);
        LOG_INFO(logger) << "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy";
    }
}

int main(){
    LOG_INFO(logger) << "main start";
    YAML::Node root = YAML::LoadFile("/home/leo/webserver/hiper/hiper/conf/log.yml");
    hiper::Config::LoadFromYaml(root);

    std::vector<hiper::Thread::ptr> thrs;

    for (int i = 0; i < 2; ++i) {
        hiper::Thread::ptr thr(new hiper::Thread(&func1, "name_" + std::to_string(i * 2)));
        hiper::Thread::ptr thr2(new hiper::Thread(&func2, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        thrs.push_back(thr2);
    }

    for (auto& i : thrs) {
        i->join();
    }

    LOG_INFO(logger) << "main end";
}