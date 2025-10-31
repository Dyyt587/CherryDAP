/*
 * MSC FAT 文件系统集成示例
 * 
 * 本文件展示如何在 app_main 中使用 FAT 文件系统功能
 */

#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "APP_MAIN";

// 声明 MSC FAT 示例函数（定义在 dap_main.c 中）
extern void msc_fat_example(void);

void app_main(void)
{
    ESP_LOGI(TAG, "=== CherryDAP with MSC FAT Example ===");

    // 等待系统稳定
    vTaskDelay(pdMS_TO_TICKS(1000));

#if CONFIG_CHERRYDAP_USE_MSC
    ESP_LOGI(TAG, "Running MSC FAT filesystem example...");
    
    // 运行 FAT 文件系统测试示例
    // 这会执行以下操作：
    // 1. 写入测试数据到 /fat/test_data.bin
    // 2. 读取文件并验证数据
    // 3. 打印验证结果
    msc_fat_example();
    
    ESP_LOGI(TAG, "MSC FAT example completed");
    
    // 演示标准文件操作
    ESP_LOGI(TAG, "Creating custom file...");
    FILE* f = fopen("/fat/custom.txt", "w");
    if (f != NULL) {
        fprintf(f, "Hello from ESP32-S3!\n");
        fprintf(f, "This is a custom file created by app_main.\n");
        fclose(f);
        ESP_LOGI(TAG, "Created /fat/custom.txt");
    } else {
        ESP_LOGE(TAG, "Failed to create custom file");
    }
    
    // 读取并显示文件内容
    ESP_LOGI(TAG, "Reading custom file...");
    f = fopen("/fat/custom.txt", "r");
    if (f != NULL) {
        char line[128];
        while (fgets(line, sizeof(line), f) != NULL) {
            // 移除换行符
            line[strcspn(line, "\n")] = 0;
            ESP_LOGI(TAG, "  > %s", line);
        }
        fclose(f);
    }
    
#else
    ESP_LOGW(TAG, "MSC is not enabled in configuration");
    ESP_LOGW(TAG, "Enable CONFIG_CHERRYDAP_USE_MSC in menuconfig to use FAT filesystem");
#endif

    ESP_LOGI(TAG, "=== Application Running ===");
    ESP_LOGI(TAG, "FAT filesystem is mounted at /fat");
    ESP_LOGI(TAG, "You can now:");
    ESP_LOGI(TAG, "  1. Connect USB to access as USB drive");
    ESP_LOGI(TAG, "  2. Use standard C file I/O (fopen/fwrite/fread)");
    ESP_LOGI(TAG, "  3. Access files from both ESP32 and PC");
    
    // 主循环
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "System running... FAT filesystem available at /fat");
    }
}
