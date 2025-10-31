# MSC FAT 文件系统使用说明

## 概述

本项目已实现 USB MSC（大容量存储设备）功能，并自动在 Flash 分区上挂载 FAT 文件系统。

## 功能特性

✅ **自动初始化**：首次调用 MSC 功能时自动挂载 FAT 文件系统  
✅ **磨损均衡**：使用 ESP-IDF 的 wear leveling 延长 Flash 寿命  
✅ **自动格式化**：如果挂载失败，自动格式化为 FAT 文件系统  
✅ **USB 存储**：通过 USB 将 ESP32-S3 识别为 U 盘  
✅ **本地访问**：ESP32 内部也可以通过 VFS 访问文件  

## 分区配置

在 `partitions.csv` 中配置了 9MB 存储分区：

```csv
storage,  data, fat,     ,        9M,
```

## 文件系统挂载点

- **挂载点**：`/fat`
- **分区名**：`storage`
- **容量**：9 MB
- **扇区大小**：512 字节
- **最大同时打开文件数**：4

## 使用方法

### 1. USB 模式（作为 U 盘使用）

将 ESP32-S3 连接到电脑：
- Windows：自动识别为可移动磁盘
- Linux/Mac：挂载为 USB 存储设备
- 可以直接拖拽文件进行读写

### 2. 内部代码访问

在 ESP32 代码中访问文件系统：

```c
#include <stdio.h>

void example_usage(void)
{
    // 写文件
    FILE* f = fopen("/fat/hello.txt", "wb");
    if (f != NULL) {
        int data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        fwrite(data, sizeof(int), 10, f);
        fclose(f);
    }

    // 读文件
    f = fopen("/fat/hello.txt", "rb");
    if (f != NULL) {
        int buffer[10];
        fread(buffer, sizeof(int), 10, f);
        fclose(f);
        
        // 使用读取的数据
        for (int i = 0; i < 10; i++) {
            printf("%d\n", buffer[i]);
        }
    }
}
```

### 3. 运行测试示例

在你的代码中调用测试函数：

```c
extern void msc_fat_example(void);

void app_main(void)
{
    // ... 其他初始化代码 ...
    
    // 运行 FAT 文件系统测试
    msc_fat_example();
}
```

## 代码架构

### 关键函数

| 函数 | 说明 |
|------|------|
| `msc_partition_init()` | 初始化分区并自动挂载 FAT |
| `msc_fat_mount()` | 挂载 FAT 文件系统（带磨损均衡） |
| `msc_fat_unmount()` | 卸载文件系统 |
| `usbd_msc_get_cap()` | USB MSC：获取容量 |
| `usbd_msc_sector_read()` | USB MSC：读取扇区 |
| `usbd_msc_sector_write()` | USB MSC：写入扇区 |
| `msc_fat_example()` | 演示文件读写功能 |

### 工作流程

```
USB 主机 <--> USB MSC 接口 <--> Flash 分区
                                     ↓
                              FAT 文件系统
                                     ↓
                              ESP32 应用程序
```

## 编译和烧录

### 1. 配置项目

确保 `sdkconfig` 中启用了 MSC 功能：
```
CONFIG_CHERRYDAP_USE_MSC=y
```

### 2. 编译

```bash
cd c:\Users\80520\Documents\GitHub\CherryDAP\projects\esp32s3
idf.py build
```

### 3. 烧录

```bash
idf.py flash monitor
```

### 4. 查看日志

启动后应该看到：
```
I (xxx) MSC: MSC using storage partition at 0x..., size: 9437184 bytes
I (xxx) MSC: FAT filesystem mounted successfully at /fat
I (xxx) MSC: Test file created: /fat/msc_ready.txt
I (xxx) MSC: FAT filesystem ready for use
```

## 注意事项

### ⚠️ 重要提示

1. **同步问题**：不要在 USB 连接时从 ESP32 内部写文件，可能导致数据不一致
2. **Flash 寿命**：频繁写入会缩短 Flash 寿命，已启用磨损均衡
3. **文件数量**：最多同时打开 4 个文件（可在代码中修改 `max_files`）
4. **格式化**：首次使用或挂载失败时会自动格式化为 FAT

### 🔧 故障排查

| 问题 | 可能原因 | 解决方法 |
|------|---------|---------|
| 挂载失败 | 分区未定义 | 检查 `partitions.csv` |
| 文件打不开 | 同时打开文件过多 | 关闭不用的文件 |
| USB 识别失败 | MSC 未启用 | 检查 `CONFIG_CHERRYDAP_USE_MSC` |
| 数据丢失 | 同步问题 | 避免 USB 和内部同时访问 |

## 高级配置

### 修改挂载点

在 `msc_fat_mount()` 中修改：
```c
esp_vfs_fat_spiflash_mount("/mydata", "storage", &mount_config, &s_wl_handle);
```

### 修改最大文件数

```c
mount_config.max_files = 8;  // 改为 8 个
```

### 禁用自动格式化

```c
mount_config.format_if_mount_failed = false;
```

## 性能优化建议

1. **批量操作**：一次性写入大量数据，减少擦除次数
2. **缓存**：使用 RAM 缓存，定期同步到 Flash
3. **日志级别**：生产环境关闭 DEBUG 日志以提高性能

## 示例应用场景

- 📝 **数据记录器**：保存传感器数据到文件
- 🔧 **配置存储**：存储用户配置文件
- 📦 **固件更新**：通过 USB 拷贝固件文件
- 🗂️ **文件传输**：ESP32 与 PC 之间传输文件

## 相关文件

- `main/dap_main.c` - MSC 和 FAT 实现
- `main/CMakeLists.txt` - 组件依赖配置
- `partitions.csv` - 分区表定义
- `main/dap_main.h` - MSC 相关定义

## API 参考

详细 API 文档请参考：
- [ESP-IDF FAT 文件系统文档](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/api-reference/storage/fatfs.html)
- [ESP-IDF 分区 API](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/api-reference/storage/partition.html)
