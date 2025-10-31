/*
 * @Author: Dyyt587 67887002+Dyyt587@users.noreply.github.com
 * @Date: 2025-10-31 14:44:08
 * @LastEditors: Dyyt587 67887002+Dyyt587@users.noreply.github.com
 * @LastEditTime: 2025-10-31 21:04:06
 * @FilePath: \esp32s3\main\hal_gpio.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef HAL_GPIO_H

// #define GPIO_POWER_PIN          (GPIO_NUM_2)
#define GPIO_LED_RUNNING_STATUS (GPIO_NUM_10)
#define GPIO_LED_WIFI_STATUS    (GPIO_NUM_6)

#define GPIO_DAP_SWD_MOSI       (GPIO_NUM_11)
#define GPIO_DAP_SWD_CLK        (GPIO_NUM_12)
#define GPIO_DAP_TDO            (GPIO_NUM_9)
#define GPIO_DAP_TDI            (GPIO_NUM_10)
#define GPIO_DAP_JTAG_nTRST     (GPIO_NUM_14)
#define GPIO_DAP_JTAG_nRESET    (GPIO_NUM_13)

#define UART_PORT               (UART_NUM_1)            
#define GPIO_UART_TX            (GPIO_NUM_18)
#define GPIO_UART_RX            (GPIO_NUM_17)

#define MODE_SWITCH_GPIO        (GPIO_NUM_15)
#endif // !HAL_GPIO_H