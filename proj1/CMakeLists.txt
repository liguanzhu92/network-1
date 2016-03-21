cmake_minimum_required(VERSION 3.3)
project(proj1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(FTPC_FILES
        client.c tcp_daemon/tcpdio.c tcp_daemon/headers/tcpd.h)

set(FTPS_FILES
        server.c tcp_daemon/tcpdio.c tcp_daemon/headers/tcpd.h)

set(TCPD_FILES
        tcp_daemon/tcpd.c tcp_daemon/timer_helper.c tcp_daemon/headers/tcpd.h tcp_daemon/headers/troll.h tcp_daemon/headers/check_sum.h)

set(TIMER_FILES
        tcp_daemon/timer.c tcp_daemon/delta_list.c tcp_daemon/headers/delta_list.h tcp_daemon/headers/tcpd.h)

add_executable(ftpc ${FTPC_FILES})
add_executable(ftps ${FTPS_FILES})
add_executable(tcpd ${TCPD_FILES})
add_executable(timer ${TIMER_FILES})