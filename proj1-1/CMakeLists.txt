cmake_minimum_required(VERSION 3.3)
project(proj1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(TIMER_FILES
        timer.c delta_list.c delta_list.h)

set(TIMER_TEST_FILES
        timer_test.c delta_list.h)

set(CHECK_SUM_FILES
        check_sum.c)

add_executable(timer ${TIMER_FILES})
add_executable(timer_test ${TIMER_TEST_FILES})
add_executable(check_sum ${CHECK_SUM_FILES})