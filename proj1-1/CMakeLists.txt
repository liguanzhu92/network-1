cmake_minimum_required(VERSION 3.3)
project(proj1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(TIMER_FILES
        timer.c delta_list.c delta_list.h)

set(TIMER_TEST_FILES
        timer_test.c delta_list.h)

add_executable(timer ${TIMER_FILES})
add_executable(timer_test ${TIMER_TEST_FILES})