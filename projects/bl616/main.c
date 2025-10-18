/*
 * @Author: Dyyt587 67887002+Dyyt587@users.noreply.github.com
 * @Date: 2024-03-30 11:14:00
 * @LastEditors: Dyyt587 67887002+Dyyt587@users.noreply.github.com
 * @LastEditTime: 2025-10-19 01:56:00
 * @FilePath: \CherryDAP\projects\bl616\main.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "board.h"
#include "dap_main.h"
#include "ebtn.h"
#include "bflb_gpio.h"
#include "usb2uart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "mem.h"

#include <lwip/tcpip.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>

#include "bl_fw_api.h"
#include "wifi_mgmr_ext.h"
#include "wifi_mgmr.h"

#include "bflb_irq.h"
#include "bflb_uart.h"

#include "bl616_glb.h"
#include "rfparam_adapter.h"

#include "board.h"
#include "shell.h"

#include "wifi_config.h"

#include "bflb_timestamp.h"
#include "custom/custom.h"

#include "wifi_mgmr_cli.h"
#define DBG_TAG "MAIN"
#include "log.h"

#define WIFI_STACK_SIZE   (1536)
#define TASK_PRIORITY_FW  (16)
#define TASK_PRIORITY_DAP (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct bflb_device_s *uart0;

static TaskHandle_t wifi_fw_task;
static TaskHandle_t dap_fw_task;
static TaskHandle_t button_task;

static wifi_conf_t conf = {
    .country_code = "CN",
};

int flag_cdc_shell=0;
extern void shell_init_with_task(struct bflb_device_s *shell);

// int wifi_start_firmware_task(void)
// {
//     LOG_I("Starting wifi ...\r\n");

//     /* enable wifi clock */

//     GLB_PER_Clock_UnGate(GLB_AHB_CLOCK_IP_WIFI_PHY | GLB_AHB_CLOCK_IP_WIFI_MAC_PHY | GLB_AHB_CLOCK_IP_WIFI_PLATFORM);
//     GLB_AHB_MCU_Software_Reset(GLB_AHB_MCU_SW_WIFI);

//     /* Enable wifi irq */

//     extern void interrupt0_handler(void);
//     bflb_irq_attach(WIFI_IRQn, (irq_callback)interrupt0_handler, NULL);
//     bflb_irq_enable(WIFI_IRQn);

//     xTaskCreate(wifi_main, (char *)"fw", WIFI_STACK_SIZE, NULL, TASK_PRIORITY_FW, &wifi_fw_task);

//     return 0;
// }

void dap_main(void *param)
{
    chry_dap_init(0, 0x20072000);
    while (1) {
        chry_dap_handle();
        chry_dap_usb2uart_handle();
        vTaskDelay(1);
    }
}


typedef enum
{
    USER_BUTTON_0 = 0,
    USER_BUTTON_MAX,

} user_button_t;

/* User defined settings */
static const ebtn_btn_param_t defaul_ebtn_param = EBTN_PARAMS_INIT(20, 0, 20, 300, 200, 500, 10);

static ebtn_btn_t btns[] = {
        EBTN_BUTTON_INIT(USER_BUTTON_0, &defaul_ebtn_param),

};


/**
 * \brief           Get input state callback
 * \param           btn: Button instance
 * \return          `1` if button active, `0` otherwise
 */
uint8_t prv_btn_get_state(struct ebtn_btn *btn)
{
    /*
     * Function will return negative number if button is pressed,
     * or zero if button is releases
     */
 
    return  bflb_gpio_read(g_gpio, GPIO_PIN_2);
}

/**
 * \brief           Button event
 *
 * \param           btn: Button instance
 * \param           evt: Button event
 */
void prv_btn_event(struct ebtn_btn *btn, ebtn_evt_t evt)
{
    if(btn->key_id==USER_BUTTON_0 && evt==EBTN_EVT_ONCLICK){
        if(flag_cdc_shell==1)
        {
            flag_cdc_shell=0;
            shell_set_print((void (*)(char *fmt, ...))printf);

        }else{
            shell_set_print((void (*)(char *fmt, ...))tfp_printf);
            flag_cdc_shell=1;
        }
        LOG_I("flag_cdc_shell  = %d\r\n", flag_cdc_shell);
        tfp_printf("flag_cdc_shell  = %d\r\n", flag_cdc_shell);
       // NVIC_SystemReset();
    }

}

void button_main(void *param)
{
    //extern struct bflb_device_s *g_gpio;
    LOG_I("button_main ...\r\n");

    bflb_gpio_init(g_gpio, GPIO_PIN_2, GPIO_INPUT | GPIO_PULLDOWN | GPIO_SMT_EN | GPIO_DRV_0);  
    ebtn_init(btns, EBTN_ARRAY_SIZE(btns), 0, 0,
              prv_btn_get_state, prv_btn_event);
              static int tick=0;
    while (1) {

        ebtn_process(tick);
        tick+=5;
        vTaskDelay(5);
 
    }
}

int dap_start_firmware_task(void)
{
    LOG_I("Starting dap ...\r\n");

    xTaskCreate(dap_main, (char *)"dap", WIFI_STACK_SIZE, NULL, TASK_PRIORITY_DAP, &dap_fw_task);

    return 0;
}
static config_event event = NULL;

#define NOTIFY_EVENT(msg, ...)         \
    do {                               \
        if (NULL != event) {           \
            event(msg, ##__VA_ARGS__); \
        }                              \
    } while (0)

void wifi_config1(void *param)
{
    static char ssid[] = "@Dyyt";
    static char pass[] = "123456789";
    flash_set_wifi_info(ssid, pass);
    flash_get_data(ssid, KEY_SSID, 16);
    flash_get_data(pass, KEY_PASS, 16);
    LOG_I("flash read wifi info: [s: %s,k: %s]\r\n", ssid, pass);

    vTaskDelay(10);

    //只需要检查ssid，密码可以是空的
    if (NULL != ssid && strlen(ssid) > 0 && 1) {
        NOTIFY_EVENT("CONNECTING(%s)\n", ssid);
        LOG_I("find wifi info: [s: %s,k: %s]\r\n", ssid, pass);
        //找到ssid，尝试连接wifi
        connect_wifi(ssid, pass);
    } else {
        LOG_I("ssid not find start...\r\n");
        //没有连接过wifi启动配网流程
        vTaskDelay(100);
        char *argv[] = {"wifi_ap_start","-s","Cubex DAPLink"};
        wifi_mgmr_ap_start_cmd(3,argv);
        vTaskDelay(100);
        start_http_server();

    }
    uint32_t old_code = -1;
    while (1) {
        //printf("wifi_config\r\n");
        vTaskDelay(1);
        uint32_t new_code = get_wifi_event_code();
        if (old_code != new_code) {
            //状态改变
            old_code = new_code;
            //处理wifi事件
           // wifi_event(new_code);
        }
    }
}

void stdout_putf ( void* p, char c)
{
    extern chry_ringbuffer_t g_usbshell;
    chry_ringbuffer_write(&g_usbshell, &c, 1);

}
int main(void)
{
    board_init();

      init_printf(NULL, stdout_putf);

    uartx_preinit();

    uart0 = bflb_device_get_by_name("uart0");
    shell_init_with_task(uart0);

    if (0 != rfparam_init(0, NULL, 0)) {
        LOG_I("PHY RF init failed!\r\n");
        return 0;
    }

    LOG_I("PHY RF init success!\r\n");

    // tcpip_init(NULL, NULL);
    // wifi_start_firmware_task();

    tcpip_init(NULL, NULL);
    //初始化wifi
    wifi_init();

    //初始化flash
    bflb_mtd_init();
    easyflash_init();

    printf("wifi_config\r\n");
    xTaskCreate(button_main, (char *)"button", 512, NULL, 15, &button_task);

    xTaskCreate(wifi_config1, "wifi_config", 512, NULL, 8, NULL);

    dap_start_firmware_task();
    vTaskStartScheduler();
    while (1) {
        // chry_dap_handle();
        // chry_dap_usb2uart_handle();
    }
}
