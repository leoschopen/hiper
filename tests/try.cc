#include "../hiper/base/hiper.h"

#include <cassert>
#include <iostream>
#include <string>
#include <yaml-cpp/yaml.h>

hiper::Logger::ptr logger = LOG_ROOT();

void test_assert()
{
    LOG_INFO(logger) << hiper::BacktraceToString(20, 2, "    ");
}

int main(int argc, char** argv)
{
    test_assert();
    return 0;
}
