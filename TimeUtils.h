#pragma once
#include <cstring>

// Get current date/time, format is YYYYMMDD.HH:mm:ss
const std::string currentDateTime( const char* format = "%Y%m%d-%X") {
    struct timeval tmnow;
    struct tm *tm;
    char buf[80] = {0}, usec_buf[32] = {0};
    gettimeofday(&tmnow, NULL);
    tm = localtime(&tmnow.tv_sec);
    strftime(buf, 30, format, tm);
    strcat(buf,".");
    sprintf(usec_buf, "%d" ,(int)tmnow.tv_usec);
    strcat(buf, usec_buf);
    return buf;
}

