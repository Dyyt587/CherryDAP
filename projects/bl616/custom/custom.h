/*
 * @Author: Dyyt587 67887002+Dyyt587@users.noreply.github.com
 * @Date: 2025-10-05 22:22:49
 * @LastEditors: Dyyt587 67887002+Dyyt587@users.noreply.github.com
 * @LastEditTime: 2025-10-24 00:33:41
 * @FilePath: \CherryDAPc:\users\80520\documents\github\cherrydap\projects\bl616\custom\custom.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __CUSTOM_H
#define __CUSTOM_H


#define KEY_SSID    "ssid"
#define KEY_PASS    "pass"
#define KEY_RTT_ADDR    "rtt_addr"
#define KEY_CDC_UART_MODE    "cdc_uart_mode"

#ifdef __cplusplus
extern "C" {

#endif

    void custom_init();

    /**
     * @brief 保存数据
     * @param  ssid
     * @param  pass
     */
    void flash_set_wifi_info(char* ssid, char* pass);

    void flash_set_cfg_info(char* rtt_addr, char* cdc_uart_mode);
    /**
     * @brief 获取数据
     * @param  buf
     * @param  key
     * @param  len
     */
    void flash_get_data(char* buf, char* key, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif