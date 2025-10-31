/*
 * @Author: Dyyt587 67887002+Dyyt587@users.noreply.github.com
 * @Date: 2025-10-09 04:00:20
 * @LastEditors: Dyyt587 67887002+Dyyt587@users.noreply.github.com
 * @LastEditTime: 2025-10-28 20:38:35
 * @FilePath: \esp32s3\main\port_common.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once
#include <stdint.h>
#include "gpio.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/uart_select.h"

#ifndef __STATIC_INLINE
#define __STATIC_INLINE static inline
#endif

#ifndef __WEAK
#define __WEAK __attribute__((weak))
#endif




#define GPIO_DAP_SWD_MOSI       (GPIO_NUM_11)
#define GPIO_DAP_SWD_CLK        (GPIO_NUM_12)
#define GPIO_DAP_TDO            (GPIO_NUM_9)
#define GPIO_DAP_TDI            (GPIO_NUM_10)
#define GPIO_DAP_JTAG_nTRST     (GPIO_NUM_14)
#define GPIO_DAP_JTAG_nRESET    (GPIO_NUM_13)

// #define PIN_SWCLK_TCK               GPIO_NUM_17
// #define PIN_SWDIO_TMS               GPIO_NUM_16
// #define PIN_TDI                     GPIO_NUM_15
// #define PIN_TDO                     GPIO_NUM_7
// #define PIN_nRESET                  GPIO_NUM_6

#define PIN_SWCLK_TCK               GPIO_DAP_SWD_CLK
#define PIN_SWDIO_TMS               GPIO_DAP_SWD_MOSI
#define PIN_TDI                     GPIO_DAP_TDI
#define PIN_TDO                     GPIO_DAP_TDO
#define PIN_nRESET                  GPIO_DAP_JTAG_nRESET



// #define LED_CONNECTED               GPIO_NUM_41
// #define LED_RUNNING                 GPIO_NUM_42

#define DAP_UART_TX                 GPIO_NUM_4
#define DAP_UART_RX                 GPIO_NUM_5

#define USART_UX                    UART_NUM_0
#define DEV_UART0_TX                GPIO_NUM_16
#define DEV_UART0_RX                GPIO_NUM_3
#define RX_BUF_SIZE                 1024

#define DAP_CPU_CLOCK 0U

void dap_platform_init(void);
void dap_gpio_init(void);

void set_led_connect(uint32_t bit);
void set_led_running(uint32_t bit);
uint32_t get_led_connect(void);
uint32_t get_led_running(void);


static inline uint32_t dap_get_time_stamp(void)
{
    return (uint32_t)xTaskGetTickCount();
}
