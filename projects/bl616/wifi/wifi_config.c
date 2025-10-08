#include "wifi_config.h"
#define DBG_TAG "wifi_confg"
#include "custom/custom.h"
#include "log.h"
#include <string.h>
#include <stdio.h>
#include "wifi_mgmr_ext.h"
#include "wifi_config_server.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "wifi.h"
#include "mem.h"

#include "bflb_timestamp.h"
#include "custom/custom.h"


//easy flash
#include "easyflash.h"
#include "bflb_mtd.h"

extern void wifi_handler_event(uint32_t code);
extern void wifi_config(void* args);
#define WIFI_STORE_LENGTH   16

#ifndef AP_SSID
#define AP_SSID     "config"
#endif

#ifndef AP_PASS
#define AP_PASS     NULL
#endif

#ifndef AP_AKM
#define AP_AKM  NULL
#endif


static char ssid[16];
static char pass[16];


void wifi_event(uint32_t code);
void connect_wifi(char* ssid, char* key);
void start_config_http_server();
void http_response_callback(char* data);
void save_wifi_info(char* ssid, char* key);


config_event event = NULL;


#define NOTIFY_EVENT(msg, ...)   \
    do{                     \
        if(NULL != event) { \
            event(msg, ##__VA_ARGS__);     \
        }                   \
    } while (0)             \


/**
 * @brief 保存wifi信息
 * @param  ssid
 * @param  pass
 */
void save_wifi_info(char* ssid, char* pass)
{
    flash_set_wifi_info(ssid, pass);
    NOTIFY_EVENT("SAVE WIFI INFO");
}

void start_http_server()
{
    if (http_server_is_runing())
    {
        LOG_I("close http server");
        http_server_close();
        bflb_mtimer_delay_ms(1000);
    }

    //启动server
    while (http_server_is_runing())
    {
        LOG_I("waiting close...");
        vTaskDelay(1000);
    }
    LOG_I("-----\r\nstarting\r\n-----\r\n");
    http_server_start(http_response_callback);
}

/**
 * @brief wifi状态回调函数
 * 测试发现没有密码错误和找不到ssid的状态，找不到ssid还可以间接处理，ssid或者是密码错误连接不上
 * 返回的状态也是CODE_WIFI_ON_DISCONNECT，貌似所有的连接失败问题都是返回这个结果，就没有根据状态
 * 继续执行不同的逻辑
 * @param  code
 */
void wifi_event(uint32_t code)
{

    switch (code)
    {

        //没有连接 启动htpp server
        case CODE_WIFI_ON_DISCONNECT:
        {
            LOG_I("CAN NOT CONNECT\r\ntrying");
            if (get_ap_status() != STARTED)
            {
                start_config_http_server();
            }
            else {
                start_http_server();
            }

        }
        break;
        //AP 启动成功，更新二维码以及描述文本
        case CODE_WIFI_ON_AP_STARTED:
        {
            NOTIFY_EVENT(NULL);
            NOTIFY_EVENT("AP START SUCCESSED!\r\nSTART HTTP SERVER");
            start_http_server();
        }
        break;
        //获取到IP，保存ssid和密码
        case CODE_WIFI_ON_GOT_IP:
        {

            char* ip = wifi_get_sta_ip();
            LOG_I("------got ip\r\n%s\r\n------", ip);
            NOTIFY_EVENT("CONNECTED!!!\r\nGET IP:\r\n%s\r\n", ip);
            save_wifi_info(ssid, pass);
            ap_close();
            http_server_close();
            NOTIFY_EVENT("CLOSE HTTP SERVER");

        }
        break;
        default:
            break;
    }
}
/**
 * @brief 注册配置信息的回调函数
 * @param  cev
 */
void register_config_msg(config_event cev)
{
    event = cev;
}

/**
 * @brief 连接WIFI
 * @param  ssid
 * @param  pass
 */
void connect_wifi(char* ssid, char* pass)
{
    wifi_connect(ssid, pass);
}

/**
 * @brief 启动http server及ap
 */
void start_config_http_server()
{
    int ret = -1;
start_ap:
    //启动 ap
    ret = ap_start(AP_SSID, AP_PASS, AP_AKM);
    if (ret < 0)
    {
        LOG_I("ap start error\n");
        goto start_ap;
    }
    NOTIFY_EVENT("AP(%s) STARTED!\n", AP_SSID);
}
/**
 * 拿到的数据格式是key1=v2&key2=v2
 * @brief 根据key值获取字符串里面的value
 * @param  data 原始数据
 * @param  key  key
 * @param  value value
 * @return int -1 找不到key对应的值，其他代表value的长度
 */
int get_responses_data(char* data, char* key, char* value)
{
    int len = -1;
    char* start = strstr(data, key);
    if (start)
    {
        start = strchr(start, '=');
        if (start)
        {
            start++;
            char* end = start;
            while (*end != '&' && *end != '\0') {
                end++;
            }
            len = end - start;
            strncpy(value, start, len);
        }
    }

    return len;
}

/**
 * @brief post数据回调函数
 * @param  data    post 接口获取到的request data
 */
void http_response_callback(char* buf)
{

    LOG_I("response data: (%s)\n", buf);
    NOTIFY_EVENT("READ WIFI INFO\n");
    memset(ssid, 0, sizeof(ssid));
    //获取ssid
    int len = get_responses_data(buf, "ssid", ssid);
    if (len <= 0)
    {
        NOTIFY_EVENT("SSID ERR!\n");
        return;
    }

    LOG_I("ssid(%s)\r\n", ssid);
    memset(pass, 0, sizeof(pass));
    len = get_responses_data(buf, "pass", pass);
    if (len > 0)
    {
        LOG_I("pass(%s)\r\n", pass);
    }
    /**手机连接小安派的热点，小安派再连接手机开的热点会连接不上wifi
     * 一直在重复wifi认证
     */
    NOTIFY_EVENT("AP CLOSE!\n");
    ap_close();
    NOTIFY_EVENT("CONNECTING WIFI...\n");
    // wifi_disconnect();
    wifi_connect(ssid, pass);
}

/**
 * @brief wifi配网
 * 先检查flash中的ssid信息，如果没有就启动配网流程
 * @param  ui
 */
void wifi_config(void* ui)
{

    // flash_get_data(ssid, KEY_SSID, WIFI_STORE_LENGTH);
    // flash_get_data(pass, KEY_PASS, WIFI_STORE_LENGTH);
    // //只需要检查ssid，密码可以是空的
    // if (NULL != ssid && strlen(ssid) > 0)
    // {
    //     NOTIFY_EVENT("CONNECTING(%s)\n", ssid);
    //     LOG_I("find wifi info: [s: %s,k: %s]", ssid, pass);
    //     //找到ssid，尝试连接wifi
    //     connect_wifi(ssid, pass);
    // }
    // else {
    //     LOG_I("ssid not find start...\n");
    //     //没有连接过wifi启动配网流程
    //    // start_config_http_server();
    // }
    // uint32_t old_code = -1;
    // while (1) {
    //     vTaskDelay(1);
    //     uint32_t new_code = get_wifi_event_code();
    //     if (old_code != new_code) {
    //         //状态改变
    //         old_code = new_code;
    //         //处理wifi事件
    //        // wifi_event(new_code);
    //     }
    // }
}


#ifdef CONFIG_SHELL
#include <shell.h>
static    char *argv1[] = {"wifi_ap_start","-s","Cubex DAPLink"};

int cmd_wifi_web_config(int argc, char **argv)
{
    
        wifi_mgmr_ap_start_cmd(3,argv1);
        vTaskDelay(100);
        start_http_server();
        NOTIFY_EVENT("AP(%s) STARTED!\r\n", AP_SSID);
    return 0;
}

SHELL_CMD_EXPORT_ALIAS(cmd_wifi_web_config, wifi_web_config, wifi web config);
#endif