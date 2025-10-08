#ifndef __WIFI_H
#define __WIFI_H

#if USE_AP == 0
#define USE_AP 1
#endif

#define WIFI_STACK_SIZE     (1536)
#define TASK_PRIORITY_FW    (16)

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    NOT_CONNECT = 0,
    CONNECT = 1,
    RESTART, SCAN,
    CLOSED,
    SCAN_DOWN,
    GOT_IP,
}wifi_status;

typedef enum
{
    S_FAILED = -1,
    S_SUCCESSED = 0,
    S_SSID_EMPTY,
    S_SSID_KEY_ERR,
}wifi_connect_code;


#if USE_AP == 1

typedef enum
{
    CLOSE = 0, STARTED = 1,
}ap_status;

#define AP_OPEN     "OPEN"
#define AP_WPA      "WPA"
#define AP_WPA2     "WPA2"

#endif

typedef struct wifi_scan_ap_item {
    uint32_t mode;
    uint32_t timestamp_lastseen;
    int ssid_len;
    uint8_t channel;
    int8_t rssi;
    char ssid[32];
    char ssid_tail[1];//always put ssid_tail after ssid
    uint8_t bssid[6];
    int8_t ppm_abs;
    int8_t ppm_rel;
    uint8_t auth;
    uint8_t cipher;
    uint8_t is_used;
    uint8_t wps;
} wifi_scan_ap_item_t;

typedef void (*ap_scan_item_cb_t)(void* env, void* arg, wifi_scan_ap_item_t* item);


#ifdef __cplusplus
extern "C" {
#endif

    uint32_t get_wifi_event_code();

    /**
     * @brief 连接wifi
     * @param  ssid
     * @param  key
     * @return uint8_t
     */
    wifi_connect_code wifi_connect(char* ssid, char* key);
    /**
     * @brief 断开wifi连接
     */
    void wifi_disconnect();
    /**
     * @brief 关闭wifi
     * @return int
     */
    int wifi_close();
    /**
     * @brief 打开wifi
     * @return int
     */
    int wifi_open();
    /**
     * @brief 重启wifi
     * @return int
     */
    int wifi_restart();
    /**
     * @brief 扫描wifi
     * @param  env
     * @param  arg
     * @param  cb
     */
    void wifi_scan(void* env, void* arg, ap_scan_item_cb_t cb);

    /**
     * @brief 获取wifi状态
     * @return wifi_status
     */
    wifi_status get_wifi_status();
    /**
     * @brief 设置wifi自动连接
     * @param  autoconnect
     */
    void wifi_autoconnect(bool autoconnect);
    /**
    * @brief 初始化wifi
    */
    void wifi_init();
    /**
     * @brief wifi event回调
     * @param  code
     */
    void wifi_event_handler(uint32_t code);

    /**
     * @brief 获取sta ip
     * @return char*
     */
    char* wifi_get_sta_ip();

#if USE_AP == 1
    /**
     * @brief 打开ap
     * @param  ssid
     * @param  key
     * @param  akm
     * @return int
     */
    int ap_start(char* ssid, char* key, char* akm);
    /**
     * @brief 重启ap
     * @param  ssid
     * @param  key
     * @param  akm
     */
    void ap_restart(char* ssid, char* key, char* akm);
    /**
     * @brief 关闭ap
     */
    void ap_close();
    /**
     * @brief 获取ap状态
     * @return ap_status
     */
    ap_status get_ap_status();
#endif


#ifdef __cplusplus
}
#endif
#endif