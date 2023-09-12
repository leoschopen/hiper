cmake_minimum_required(VERSION 3.25)

project(hiper)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -std=c++17 -g -O0 -ggdb -Wall -Werror")

# -rdynamic: 将所有符号都加入到符号表中，便于使用dlopen或者backtrace追踪到符号
# -fPIC: 生成位置无关的代码，便于动态链接
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -fPIC")


# -Wno-unused-function: 不要警告未使用函数
# -Wno-builtin-macro-redefined: 不要警告内置宏重定义，用于重定义内置的__FILE__宏
# -Wno-deprecated: 不要警告过时的特性
# -Wno-deprecated-declarations: 不要警告使用带deprecated属性的变量，类型，函数
# -Wno-unused-command-line-argument: 不要警告未使用的命令行参数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated -Wno-deprecated-declarations -Wno-unused-command-line-argument")

#add_compile_options(-fstandalone-debug)
#message(STATUS "optional: -fstandalone-debug")


find_package(Boost REQUIRED) 
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

set(LIB_SRC
        hiper/base/address.cc
        hiper/base/bytearray.cc
        hiper/base/config.cc
        hiper/base/endian.hpp
        hiper/base/env.cc
        hiper/base/fdmanager.cc
        hiper/base/fiber.cc
        hiper/base/hook.cc
        hiper/base/iomanager.cc
        hiper/base/log.cc
        hiper/base/mutex.cc
        hiper/base/scheduler.cc
        hiper/base/socket.cc
        hiper/base/thread.cc
        hiper/base/timer.cc
        hiper/base/util.cc
        )

set(LIB_LIST pthread yaml-cpp mimalloc jemalloc)

add_library(hiper SHARED ${LIB_SRC})
target_link_libraries(hiper "${LIB_LIST}")


add_executable(mutex_test "tests/mutex_test.cc")
target_link_libraries(mutex_test hiper "${LIB_LIST}")

add_executable(log_test "tests/log_test.cc")
target_link_libraries(log_test hiper "${LIB_LIST}")

add_executable(config_test "tests/config_test.cc")
target_link_libraries(config_test hiper "${LIB_LIST}")

add_executable(scheduler_test "tests/scheduler_test.cc")
target_link_libraries(scheduler_test hiper "${LIB_LIST}")

add_executable(bytearray_test "tests/bytearray_test.cc")
target_link_libraries(bytearray_test hiper "${LIB_LIST}")

add_executable(timer_test "tests/timer_test.cc")
target_link_libraries(timer_test hiper "${LIB_LIST}")

add_executable(hook_test "tests/hook_test.cc")
target_link_libraries(hook_test hiper "${LIB_LIST}")

add_executable(try "tests/try.cc")
target_link_libraries(try hiper "${LIB_LIST}")


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)