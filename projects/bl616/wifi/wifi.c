#include <stdint.h>
#include "bl_fw_api.h"
#define DBG_TAG "wifi"
#include "log.h"

#include "bl616_glb.h"
#include "rfparam_adapter.h"

#include "bflb_irq.h"
#include "bflb_uart.h"
#include "bflb_l1c.h"
#include "bflb_mtimer.h"

#include "lwip/netif.h"
#include "dhcp_server.h"

#include "wifi.h"
#include "wifi_mgmr.h"

#include "task.h"

static wifi_status status = CLOSED;
bool wifi_mgmr_init_down = false;

static wifi_conf_t conf =
{
    .country_code = "CN",
};
static TaskHandle_t wifi_fw_task;
uint32_t wifi_event_code = -1;

#define WIFI_INTI() wifi_mgmr_init(&conf)

#define AP_STOP()                   \
    do                              \
    {                               \
        if(wifi_mgmr_ap_state_get()){\
            wifi_mgmr_ap_stop();        \
        }                               \
    } while (0);                    \

#define WIIF_DISCONNECT()           \
    do                              \
    {                               \
        if(wifi_mgmr_sta_state_get())   \
        {                               \
            wifi_sta_disconnect();       \
        }                         \
    } while (0);                    \


#define WIFI_OPEN()     wifi_mgmr_wifi_pwr_on()
#define WIFI_CLOSE()    wifi_mgmr_wifi_pwr_off()


#if USE_AP == 1
ap_status _ap_status = CLOSE;
static wifi_mgmr_ap_params_t ap_paramts = {
    0
};

/**
 * @brief 开启ap
 * @param  ssid
 * @param  key
 * @param  akm
 * @return int
 */
int ap_start(char* ssid, char* key, char* akm)
{
    wifi_mgmr_ap_params_t* dsc = &ap_paramts;

    dsc->ssid = ssid;
    dsc->key = key;
    dsc->akm = akm;
    dsc->channel = 3;
    dsc->use_dhcpd = true;

    // AP_STOP();
    int status = wifi_mgmr_ap_state_get();
    if (status == 0)
    {
        WIFI_INTI();
        LOG_I("start ap info:[ssid: %s,key: %s,akm: %s]", ap_paramts.ssid, ap_paramts.key, ap_paramts.akm);
        wifi_mgmr_conf_max_sta(4);
        char* buf = (char*)pvPortMalloc(1024);
        vTaskList(buf);
        printf("-----task----------\r\n%s\r\n-----------------\r\n", buf);
        vPortFree(buf);
        return wifi_mgmr_ap_start(&ap_paramts);
    }
    LOG_I("ap started!\r\n");
    return status;
}
/**
 * @brief 重启ap
 * @param  ssid
 * @param  key
 * @param  akm
 */
void ap_restart(char* ssid, char* key, char* akm)
{
    AP_STOP();
    ap_start(ssid, key, akm);
}
/**
 * @brief 获取ap信息
 * @return wifi_mgmr_ap_params_t*
 */
wifi_mgmr_ap_params_t* get_ap_info()
{
    if (wifi_mgmr_ap_state_get())
    {
        return &ap_paramts;
    }
    return NULL;
}
/**
 * @brief 关闭ap
 */
void ap_close()
{
    AP_STOP();
}
ap_status get_ap_status()
{
    return _ap_status;
}
#endif
/**
 * @brief 关闭wifi
 * @return int
 */
int wifi_close()
{
    return WIFI_CLOSE();
}
/**
 * @brief 打开wifi
 * @return int
 */
int wifi_open()
{
    return WIFI_OPEN();
}
/**
 * @brief 重启wifi
 * @return int
 */
int wifi_restart()
{
    if (wifi_mgmr_wifi_pwr_off() == 0)
    {
        WIFI_OPEN();
    }
    else {
        if (WIFI_CLOSE() == 0)
        {
            return WIFI_OPEN();
        }
    }
    return -1;
}
/**
 * @brief 扫描wifi
 * @param  evn
 * @param  arg
 * @param  cb
 */
void wifi_scan(void* evn, void* arg, ap_scan_item_cb_t cb)
{
    wifi_mgmr_scan_ap_all(evn, arg, (scan_item_cb_t)cb);
}
/**
 * @brief 获取wifi状态
 * @return wifi_status
 */
wifi_status get_wifi_status()
{
    return status;
}
/**
 * @brief 获取sta ip
 * @return char*
 */
char* wifi_get_sta_ip()
{
    uint32_t ip = 0, mask = 0, gaw = 0, dns = 0;
    wifi_sta_ip4_addr_get(&ip, &mask, &gaw, &dns);
    LOG_I("u32_t ip: %u\r\nmask:%u\r\ngaw:%u\r\ndns:%u", ip, mask, gaw, dns);
    //把int ip转成char类型
    static char cip[IP4ADDR_STRLEN_MAX];
    //高位字节在低地址中，大端模式，获取ip要从高地址到低地址开始读
    sprintf(cip, "%d.%d.%d.%d", ip & 0xFF, ip >> 8 & 0xFF, ip >> 16 & 0xFF, ip >> 24 & 0xFF);
    return cip;
}
/**
 * @brief 连接wifi
 * @param  ssid
 * @param  key
 * @return uint8_t
 */
wifi_connect_code wifi_connect(char* ssid, char* key)
{
    if (NULL == ssid || strlen(ssid) == 0)
    {
        return S_SSID_EMPTY;
    }

    WIIF_DISCONNECT();

    // wifi_sta_connect(ssid, key, NULL, NULL, 0, 0, 0, 1)
    return wifi_sta_connect(ssid, key, NULL, NULL, 0, 0, 0, 1);
}
/**
 * @brief 连接wifi
 * @param  ssid
 * @param  key
 * @return uint8_t
 */
void wifi_disconnect(char* ssid, char* key)
{

    WIIF_DISCONNECT();
 
}
/**
 * @brief 设置wifi自动连接
 * @param  autoconnect
 */
void wifi_autoconnect(bool autoconnect)
{
    if (autoconnect) {
        wifi_mgmr_sta_autoconnect_enable();
    }
    else {
        wifi_mgmr_sta_autoconnect_disable();
    }
}

/**
 * @brief 初始化wifi
 */
void wifi_init()
{
    LOG_I("Starting wifi ...\r\n");
    /* enable wifi clock */

    GLB_PER_Clock_UnGate(GLB_AHB_CLOCK_IP_WIFI_PHY | GLB_AHB_CLOCK_IP_WIFI_MAC_PHY | GLB_AHB_CLOCK_IP_WIFI_PLATFORM);
    GLB_AHB_MCU_Software_Reset(GLB_AHB_MCU_SW_WIFI);

    /* set ble controller EM Size */

    GLB_Set_EM_Sel(GLB_WRAM160KB_EM0KB);

    if (0 != rfparam_init(0, NULL, 0))
    {
        LOG_I("PHY RF init failed!");
        return;
    }

    LOG_I("PHY RF init success!");

    extern void interrupt0_handler(void);
    bflb_irq_attach(WIFI_IRQn, (irq_callback)interrupt0_handler, NULL);
    bflb_irq_enable(WIFI_IRQn);
    wifi_autoconnect(true);
    wifi_event_code = -1;
    xTaskCreate(wifi_main, (char*)"fw", WIFI_STACK_SIZE, NULL, TASK_PRIORITY_FW, &wifi_fw_task);
}

/**
 * @brief wifi event回调
 * @param  code
 */
void wifi_event_handler(uint32_t code)
{
    wifi_event_code = code;
    LOG_I("EVENT CODE[%d]\r\n", code);
    switch (code) {
        case CODE_WIFI_ON_INIT_DONE:
        {
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_INIT_DOWE\r\n", __func__);
            wifi_mgmr_init(&conf);
            status = NOT_CONNECT;
        }
        break;
        case CODE_WIFI_ON_MGMR_DONE:
        {
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_MGMR_DONE\r\n", __func__);
        }
        break;
        case CODE_WIFI_ON_SCAN_DONE:
        {
            status = SCAN_DOWN;
            wifi_mgmr_sta_scanlist();
        }
        break;
        case CODE_WIFI_ON_CONNECTED:
        {
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_CONNECTED\r\n", __func__);
            status = CONNECT;
            void mm_sec_keydump();
            mm_sec_keydump();
        }
        break;
        case CODE_WIFI_ON_GOT_IP:
        {
            status = GOT_IP;
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_GOT_IP\r\n", __func__);
        }
        break;
        case CODE_WIFI_ON_DISCONNECT:
        {
            status = NOT_CONNECT;
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_DISCONNECT\r\n", __func__);
        }
        break;
        case CODE_WIFI_ON_AP_STARTED:
        {
        #if USE_AP == 1
            _ap_status = STARTED;
        #endif
            LOG_I("[AP] [EVT] %s, CODE_WIFI_ON_AP_STARTED\r\n", __func__);
        }
        break;
        case CODE_WIFI_ON_AP_STOPPED:
        {
        #if USE_AP == 1
            _ap_status = CLOSE;
        #endif
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_AP_STOPPED\r\n", __func__);
        }
        break;
        case CODE_WIFI_ON_AP_STA_ADD:
        {
            LOG_I("[APP] [EVT] [AP] [ADD] %lld\r\n", xTaskGetTickCount());
        }
        break;
        case CODE_WIFI_ON_AP_STA_DEL:
        {
            LOG_I("[APP] [EVT] [AP] [DEL] %lld\r\n", xTaskGetTickCount());
        }
        break;
        default:
        {
            LOG_I("[APP] [EVT] Unknown code %u \r\n", code);
        }
        break;
    }
}
uint32_t get_wifi_event_code()
{
    return wifi_event_code;
}
