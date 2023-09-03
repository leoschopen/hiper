#include "../hiper/base/hiper.h"
#include "../hiper/base/timer.h"
#include "../hiper/base/iomanager.h"

static hiper::Logger::ptr g_logger = LOG_ROOT();

static int               timeout = 1000;
static hiper::Timer::ptr s_timer;

void timer_callback()
{
    LOG_INFO(g_logger) << "timer callback, timeout = " << timeout;
    timeout += 1000;
    if (timeout < 5000) {
        s_timer->reset(timeout, true);
    }
    else {
        s_timer->cancel();
    }
}

int main(int argc, char* argv[])
{
    hiper::IOManager iom(1, true, "test");
    // recur
    s_timer = iom.addTimer(1000, timer_callback, true);

    // 单次定时器
    // iom.addTimer(500, [] { LOG_INFO(g_logger) << "500ms timeout"; });
    // iom.addTimer(5000, [] { LOG_INFO(g_logger) << "5000ms timeout"; });

    return 0;
}