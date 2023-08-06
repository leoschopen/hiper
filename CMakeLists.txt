cmake_minimum_required(VERSION 3.25)

project(hiper)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -std=c++17 -rdynamic -g -O0 -ggdb")

#add_compile_options(-fstandalone-debug)
#message(STATUS "optional: -fstandalone-debug")

find_package(Boost REQUIRED) 
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

set(LIB_SRC
        hiper/base/thread.cc
        hiper/base/util.cc
        hiper/base/mutex.cc
        hiper/base/log.cc
        hiper/base/config.cc
        hiper/base/env.cc
        hiper/base/noncopyable.h
        hiper/base/singleton.h
        )

set(LIB_LIST pthread yaml-cpp)

add_library(hiper SHARED ${LIB_SRC})
target_link_libraries(hiper "${LIB_LIST}")


add_executable(mutex_test "tests/mutex_test.cc")
target_link_libraries(mutex_test hiper "${LIB_LIST}")

add_executable(log_test "tests/log_test.cc")
target_link_libraries(log_test hiper "${LIB_LIST}")

add_executable(config_test "tests/config_test.cc")
target_link_libraries(config_test hiper "${LIB_LIST}")

add_executable(try "tests/try.cc")
target_link_libraries(try hiper "${LIB_LIST}")


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)