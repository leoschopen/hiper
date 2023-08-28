#include "../hiper/base/hiper.h"

hiper::Logger::ptr g_logger = LOG_ROOT();

void run_in_fiber() {
    LOG_INFO(g_logger) << "run_in_fiber begin";
    hiper::Fiber::YieldToHold();
    LOG_INFO(g_logger) << "run_in_fiber end";
    hiper::Fiber::YieldToHold();
}

void test_fiber() {
    LOG_INFO(g_logger) << "main begin -1";
    {
        hiper::Fiber::GetThis(); // create main fiber
        LOG_INFO(g_logger) << "main begin";
        hiper::Fiber::ptr fiber(new hiper::Fiber(run_in_fiber)); // sub fiber
        fiber->resume(); // hold main fiber and exec sub fiber
        LOG_INFO(g_logger) << "main after resume";
        fiber->resume();
        LOG_INFO(g_logger) << "main after end";
        fiber->resume();
    }
    LOG_INFO(g_logger) << "main after end2";
}

int main(int argc, char** argv) {
    hiper::Thread::SetName("main");

    std::vector<hiper::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i) { 
        thrs.push_back(hiper::Thread::ptr(
                    new hiper::Thread(&test_fiber, "name_" + std::to_string(i))));
    }
    for(auto i : thrs) {
        i->join();
    }
    return 0;
}
