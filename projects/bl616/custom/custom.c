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

void custom_init()
{

}