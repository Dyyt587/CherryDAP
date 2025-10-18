/*
 * @Author: Dyyt587 67887002+Dyyt587@users.noreply.github.com
 * @Date: 2025-10-19 02:38:00
 * @LastEditors: Dyyt587 67887002+Dyyt587@users.noreply.github.com
 * @LastEditTime: 2025-10-19 02:39:57
 * @FilePath: \CherryDAP\projects\bl616\SystemView\SEGGER_SystemView.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef SEGGER_SYSTEMVIEW_H
#define SEGGER_SYSTEMVIEW_H
#include "board.h"
// #include "microboot.h"
#include "SEGGER_SYSVIEW_REC.h"

void read_system_and_send_usb(void);
uint32_t write_system_and_receive_usb(uint8_t inputChar);
#endif


