#ifndef __WIFI_CONFIG_SERVER_H
#define __WIFI_CONFIG_SERVER_H

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include <string.h>
#include "semphr.h"

#include <sys/socket.h>
#include <stdint.h>

#define u32 uint32_t
#define u16 uint16_t
#define u8  uint8_t

//数据回调
typedef void (*response_callback)(char* data);

typedef struct myparm
{
    int sc;
    u8* buf;
}MYPARM;

#ifdef __cplusplus
extern "C"
{
#endif

    //启动http server
    int http_server_start(response_callback cb);
    void http_server_close();
    bool http_server_is_runing();

#ifdef __cplusplus
}
#endif
#endif