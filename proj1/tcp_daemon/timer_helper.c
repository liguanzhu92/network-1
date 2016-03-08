#include <stdlib.h>

//
// Created by xu on 3/1/16.
//
float cal_RTO(float M, int packet_ctrl) {
    float A = 0;
    float D = 3000;
    float Err;
    float g = 0.125;
    float h = 0.25;
    int RTO = 0;
    if(packet_ctrl == 0) {
        RTO = 60000;
        return RTO;
    } else if(packet_ctrl == 1) {
        A = 50;
        if(M == 0)
        {
            RTO = 60000;
            return RTO;
        }
        else
        {
            RTO = M;
            return RTO;
        }
    } else {
        Err = M - A;
        A = A + g * Err;
        D = D + h * (abs(Err)-D);
        RTO = A + 4 * D;
        return RTO;
    }
}
float cal_RTT(struct timeval *start, struct timeval *end) {
    float RTT =  end->tv_sec * 1e6 + end->tv_usec - start->tv_sec * 1e6 - start->tv_usec;
    return RTT;
}
