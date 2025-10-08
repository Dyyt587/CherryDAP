/*
 * @Author: Dyyt587 67887002+Dyyt587@users.noreply.github.com
 * @Date: 2025-10-05 22:22:50
 * @LastEditors: Dyyt587 67887002+Dyyt587@users.noreply.github.com
 * @LastEditTime: 2025-10-07 23:36:33
 * @FilePath: \CherryDAP\projects\bl616\wifi\wifi_config.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#ifndef __WIFI_CONFIG_H
#define __WIFI_CONFIG_H

#include "easyflash.h"
#include "wifi.h"
#include "wifi_config_server.h"
#include "lwipopts_user.h"

#define AP_SSID     "config"

/**
 * 定义位置在${SDK}/components/net/lwip/lwip_apps/dhcpd/dhcp_server_raw.c中
 * 如果要修改需要在lwipopts_user.h中定义好DHCP地址，lwip/opt.h会自动引用lwipopts_user.h中的代码
 */
#define DHCP_IP DHCPD_SERVER_IP



typedef void (*config_event)(char* msg, ...);
void wifi_config(void* args);
void register_config_msg(config_event cev);
#endif