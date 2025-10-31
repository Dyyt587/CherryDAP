/*
 * @Author: Dyyt587 67887002+Dyyt587@users.noreply.github.com
 * @Date: 2025-10-09 04:00:20
 * @LastEditors: Dyyt587 67887002+Dyyt587@users.noreply.github.com
 * @LastEditTime: 2025-11-01 00:39:06
 * @FilePath: \esp32s3\main\main.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "usb_config.h"
#include "driver/usb_serial_jtag.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_check.h"
#include "dap_main.h"
#include "usb2uart.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "elaphureLink/elaphureLink_protocol.h"

void app_main() {
     extern void msc_fat_example(void);
     extern esp_err_t msc_fat_mount(void);
     // msc_fat_mount();
     // msc_fat_example();
     uartx_preinit();
     chry_dap_init(0, ESP_USBD_BASE);
    while (1) {
         chry_dap_handle();
         chry_dap_usb2uart_handle();
    }
}