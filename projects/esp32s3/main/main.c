/*
 * @Author: Dyyt587 67887002+Dyyt587@users.noreply.github.com
 * @Date: 2025-10-09 04:00:20
 * @LastEditors: Dyyt587 67887002+Dyyt587@users.noreply.github.com
 * @LastEditTime: 2025-11-02 03:06:12
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
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "mdns.h"
#include "wifi_handle.h"

#if CONFIG_EXAMPLE_FATFS_MODE_READ_ONLY
#define EXAMPLE_FATFS_MODE_READ_ONLY true
#else
#define EXAMPLE_FATFS_MODE_READ_ONLY false
#endif

#if CONFIG_FATFS_LFN_NONE
#define EXAMPLE_FATFS_LONG_NAMES false
#else
#define EXAMPLE_FATFS_LONG_NAMES true
#endif

static const char *TAG = "example";

// Mount path for the partition
const char *base_path = "/spiflash";

// Handle of the wear levelling library instance
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

#define DEFAULT_SCAN_LIST_SIZE 2

//static const char *TAG = "scan";




TaskHandle_t kWifiTcpServerTaskhandle = NULL;
TaskHandle_t kDAPTaskHandle=NULL;
extern  void tcp_server_task(void *pvParameters);





// WiFi 配置 - 请修改为您的 WiFi 信息
#define WIFI_SSID          "@Dyyt"     // 修改为您的 WiFi 名称
#define WIFI_PASSWORD      "123456789" // 修改为您的 WiFi 密码
#define WIFI_MAXIMUM_RETRY 5

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi connecting...");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP (attempt %d/%d)", s_retry_num, WIFI_MAXIMUM_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "Failed to connect to WiFi");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// CherryDAP 任务
void chry_dap_task(void *pvParameters)
{
    ESP_LOGI(TAG, "CherryDAP task started");

    // 初始化 UART 和 DAP
    uartx_preinit();
    chry_dap_init(0, ESP_USBD_BASE);

    ESP_LOGI(TAG, "CherryDAP initialized, entering main loop");

    // 主循环处理 DAP 和 USB-UART
    while (1) {
        chry_dap_handle();
        chry_dap_usb2uart_handle();

        // 可选：添加小延迟以降低 CPU 占用（如果不需要最高性能）
        //vTaskDelay(pdMS_TO_TICKS(1));
    }

    // 任务不会退出
    vTaskDelete(NULL);
}

/**
 * @brief 打印所有 FreeRTOS 任务的详细信息（使用 vTaskList）
 */
void print_task_info(void)
{
    // 分配缓冲区用于 vTaskList
    char *task_list_buffer = pvPortMalloc(2048);
    char *runtime_stats_buffer = pvPortMalloc(2048);

    if (task_list_buffer != NULL && runtime_stats_buffer != NULL) {
        UBaseType_t task_count = uxTaskGetNumberOfTasks();

        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "FreeRTOS Task List with Core Affinity");
        ESP_LOGI(TAG, "Total tasks: %d", task_count);
        ESP_LOGI(TAG, "========================================");

        // 获取任务详细信息以显示 CPU 核心
        TaskStatus_t *task_status_array = pvPortMalloc(task_count * sizeof(TaskStatus_t));

        if (task_status_array != NULL) {
            uint32_t total_runtime;
            UBaseType_t actual_count = uxTaskGetSystemState(task_status_array, task_count, &total_runtime);

            ESP_LOGI(TAG, "%-16s %-6s %-5s %-8s %-5s %-8s",
                     "Task Name", "State", "Prio", "Stack", "Num", "Core");
            ESP_LOGI(TAG, "----------------------------------------------------------------");

            for (UBaseType_t i = 0; i < actual_count; i++) {
                TaskStatus_t *task = &task_status_array[i];

                // 获取任务状态字符
                const char state_char =
                    (task->eCurrentState == eRunning)   ? 'X' :
                    (task->eCurrentState == eReady)     ? 'R' :
                    (task->eCurrentState == eBlocked)   ? 'B' :
                    (task->eCurrentState == eSuspended) ? 'S' :
                                                          'D';

// 获取任务运行的核心
#if CONFIG_FREERTOS_UNICORE
                const char *core_str = "0";
#else
                BaseType_t affinity = xTaskGetAffinity(task->xHandle);
                const char *core_str;
                if (affinity == 0) {
                    core_str = "Core 0";
                } else if (affinity == 1) {
                    core_str = "Core 1";
                } else if (affinity == tskNO_AFFINITY) {
                    core_str = "Any";
                } else {
                    core_str = "Unknown";
                }
#endif

                ESP_LOGI(TAG, "%-16s %-6c %-5d %-8d %-5d %-8s",
                         task->pcTaskName,
                         state_char,
                         task->uxCurrentPriority,
                         task->usStackHighWaterMark,
                         task->xTaskNumber,
                         core_str);
            }

            vPortFree(task_status_array);
        }

        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "CPU Usage Statistics");
        ESP_LOGI(TAG, "========================================");

        // 使用 vTaskGetRunTimeStats 获取运行时统计
        vTaskGetRunTimeStats(runtime_stats_buffer);
        ESP_LOGI(TAG, "Task Name       Runtime         %%");
        ESP_LOGI(TAG, "----------------------------------------");
        printf("%s\n", runtime_stats_buffer);

        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "Memory Information");
        ESP_LOGI(TAG, "----------------------------------------");
        ESP_LOGI(TAG, "Free heap:     %6d bytes", esp_get_free_heap_size());
        ESP_LOGI(TAG, "Min free heap: %6d bytes", esp_get_minimum_free_heap_size());
        ESP_LOGI(TAG, "Largest block: %6d bytes", heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
        ESP_LOGI(TAG, "========================================\n");

    } else {
        ESP_LOGE(TAG, "Failed to allocate memory for task info");
    }

    // 释放缓冲区
    if (task_list_buffer != NULL) {
        vPortFree(task_list_buffer);
    }
    if (runtime_stats_buffer != NULL) {
        vPortFree(runtime_stats_buffer);
    }
}

 
void app_main()
{
    DAP_SETUP();
    LED_RUNNING_OUT(1);
    // Initialize NVS
    // esp_err_t ret = nvs_flash_init();
    // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // 	ESP_ERROR_CHECK(nvs_flash_erase());
    // 	ret = nvs_flash_init();
    // }
    // ESP_ERROR_CHECK( ret );

    // app_main1();
        wifi_init();

    // 创建 WiFi 扫描任务，固定到 CPU 核心 1
    // 参数：任务函数, 任务名称, 堆栈大小, 参数, 优先级, 任务句柄, CPU 核心
    xTaskCreatePinnedToCore(
        tcp_server_task,   // 任务函数
        "tcp_server_task", // 任务名称
        4096,             // 堆栈大小（字节）
        NULL,             // 传递给任务的参数
        5,                // 任务优先级（0-24，数字越大优先级越高）
        &kWifiTcpServerTaskhandle,             // 任务句柄（如果不需要可以为 NULL）
        0                 // CPU 核心：0 或 1 (ESP32-S3 有两个核心)
    );
    //Initialize NVS
    // esp_err_t ret = nvs_flash_init();
    // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    //     ESP_ERROR_CHECK(nvs_flash_erase());
    //     ret = nvs_flash_init();
    // }
    // ESP_ERROR_CHECK(ret);

    // ESP_LOGI(TAG, "Starting WiFi connection...");
    // wifi_init_sta();
    // ESP_LOGI(TAG, "WiFi task created on core 1");

    // 创建 CherryDAP 任务，固定到 CPU 核心 0
    xTaskCreatePinnedToCore(
        chry_dap_task,   // 任务函数
        "chry_dap_task", // 任务名称
        8192,            // 堆栈大小（字节）- DAP 需要较大堆栈
        NULL,            // 传递给任务的参数
        20,               // 任务优先级（高优先级，确保实时响应）
        &kDAPTaskHandle,            // 任务句柄
        1                // CPU 核心 0 (PRO_CPU)
    );

    ESP_LOGI(TAG, "CherryDAP task created on core 0");
    LED_RUNNING_OUT(0);
    // 主任务可以空闲或执行其他工作
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000)); // 每 5 秒打印一次
       // ESP_LOGI(TAG, "Main task running...");

        // 打印所有任务信息
        //print_task_info();
    }
}