#include <stdio.h>
#include <stdlib.h>
#include "easyflash.h"
#include "log.h"
#include "custom.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

/**
 * @brief 保存数据
 * @param  key
 * @param  value
 */
void flash_set_wifi_info(char* ssid, char* pass)
{

    char buf[1024] = { '\0' };
    vTaskList(buf);
    printf("-----b------\n%s\n----------", buf);
    int code = ef_set_env(KEY_SSID, ssid);
    vTaskList(buf);
    printf("-----e------\n%s\n----------", buf);
    if (code != EF_NO_ERR) {
        goto back;
    }
    code = ef_set_env(KEY_PASS, pass);
    ef_save_env();
back:
    LOG_I("flash save finish:%d\n", code);
}


//mode 0 串口 1 shell 2 rtt
void flash_set_cfg_info(char* rtt_addr, char* cdc_uart_mode)
{
 
    int code = ef_set_env(KEY_RTT_ADDR, rtt_addr);
    if (code != EF_NO_ERR) {
        goto back;
    }
    code = ef_set_env(KEY_CDC_UART_MODE, cdc_uart_mode);
    ef_save_env();
back:
    LOG_I("flash save finish:%d\n", code);
}

/**
 * @brief 获取数据
 * @paran  buf
 * @param  key
 * @param  len
 */
void flash_get_data(char* buf, char* key, uint32_t len)
{
    ef_get_env_blob(key, buf, (size_t)len, (size_t*)&len);
}
/**
 * @brief 设置数据
 * @paran  buf
 * @param  key
 * @param  len
 */
void flash_set_data(char* buf, char* key, uint32_t len)
{
    ef_set_env_blob(key, buf, len);
    ef_save_env();
}
void custom_init()
{

}


#ifdef CONFIG_SHELL
#include <shell.h>

int cmd_cfg_set(int argc, char **argv)
{
    if (argc < 3) {
        printf("Usage: cfg set <key> <value>\n");
        return -1;
    }

    const char *key = argv[1];
    const char *value = argv[2];

    flash_set_data((char *)value, (char *)key, strlen(value));
    return 0;
}

SHELL_CMD_EXPORT_ALIAS(cmd_cfg_set, cfg_set, cfg set);

int cmd_cfg_get(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: cfg get <key>\n");
        return -1;
    }

    const char *key = argv[1];
    char value[128] = {0};
    flash_get_data(value, (char *)key, sizeof(value));
    printf("cfg get: [%s] = [%s]\n", key, value);
    return 0;
}
SHELL_CMD_EXPORT_ALIAS(cmd_cfg_get, cfg_get, cfg get);
#endif
