#include "../hiper/base/hiper.h"
#include <unistd.h>

static hiper::Logger::ptr g_logger = LOG_ROOT();

void test_fiber() {
    static int s_count = 5;
    LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

    sleep(1);
    if(--s_count >= 0) {
        hiper::Scheduler::GetThis()->schedule(&test_fiber, hiper::GetThreadId());
    }
}

int main(int argc, char** argv) {
    LOG_INFO(g_logger) << "main";
    hiper::Scheduler sc(1, false, "test");
    sc.start();
    sleep(2);
    LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    LOG_INFO(g_logger) << "over";
    return 0;
}
