# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
# file(GLOB_RECURSE SOURCES
#     "usb2uart.c"
#     "main.c"
#     "dap_main.c"
#     "../dap_main.c"
#     "../DAP/Source/*.c"
#     "../CherryRB/*.c"
#     )

# set(include_dirs 
#     "."
#     "../"
#     "../DAP/Include"
#     "../CherryRB"
#     )

file(GLOB_RECURSE SOURCES
    "usb2uart.c"
    "tcp_server.c"
    "usbip_server.c"
    "wifi_handle.c"
    "DAP_handle.c"
    "main.c"
    "dap_main.c"
    "../CherryRB/*.c"
    "../DAP_NEW/source/*.c"
    "../elaphureLink/*.c"
    "../USBIP/*.c"
    )

set(include_dirs 
    "."
    "../"
    "../DAP_NEW/include"
    "../DAP_NEW/config"
    "../CherryRB"
    "../elaphureLink"
    "../USBIP"
    )

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES esp_wifi esp_http_server vfs esp_http_client nvs_flash driver usb spi_flash esp_partition fatfs wear_levelling
                       WHOLE_ARCHIVE)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable -Wno-format -Wno-format-truncation -Wno-incompatible-pointer-types)

# Create a FATFS image from the contents of the 'fatfs_image' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash'.
# If read-only mode is set (CONFIG_EXAMPLE_FATFS_MODE_READ_ONLY)
# the generated image will be raw without wear levelling support.
# Otherwise it will support wear levelling and thus enable read-write mounting of the image in the device.

# if(CONFIG_FATFS_LFN_NONE)
#     set(image ../fatfs_image)
# else()
#     set(image ../fatfs_long_name_image)
# endif()

# if(CONFIG_EXAMPLE_FATFS_MODE_READ_ONLY)
#     if(CONFIG_EXAMPLE_FATFS_DEFAULT_DATETIME)
#         fatfs_create_rawflash_image(storage ${image} FLASH_IN_PROJECT)
#     else()
#         fatfs_create_rawflash_image(storage ${image} FLASH_IN_PROJECT PRESERVE_TIME)
#     endif()
# else()
#     if(CONFIG_EXAMPLE_FATFS_DEFAULT_DATETIME)
#         fatfs_create_spiflash_image(storage ${image} FLASH_IN_PROJECT)
#     else()
#         fatfs_create_spiflash_image(storage ${image} FLASH_IN_PROJECT PRESERVE_TIME)
#     endif()
# endif()
