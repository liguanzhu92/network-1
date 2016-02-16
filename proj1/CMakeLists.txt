cmake_minimum_required(VERSION 3.3)
project(proj1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(FTPC_FILES
        client.c tcpdio.c ftp.h tcpd.h)

set(FTPS_FILES
        server.c tcpdio.c ftp.h tcpd.h)

set(TCPD_FILES
        tcpd.c tcpd.h troll.h)

set(TIMER_FILES
        timer.c delta_list.c delta_list.h)

add_executable(ftpc ${FTPC_FILES})
add_executable(ftps ${FTPS_FILES})
add_executable(tcpd ${TCPD_FILES})
add_executable(timer ${TIMER_FILES})