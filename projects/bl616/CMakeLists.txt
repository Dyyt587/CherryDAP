cmake_minimum_required(VERSION 3.15)

include(proj.conf)

find_package(bouffalo_sdk REQUIRED HINTS $ENV{BL_SDK_BASE})

# User
sdk_add_compile_definitions(-DCONFIG_CLI_CMD_ENABLE)
#sdk_add_compile_definitions(-DBL616_DHCP_DEBUG)

if(CONFIG_WIFI_GATEWAY)
sdk_add_compile_definitions(-DFHOST_BUFFER_COPY=1)
sdk_add_compile_definitions(-DFHOST_RX_BUF_CNT=48)
sdk_add_compile_definitions(-DFHOST_RX_BUF_SECTION=\".bss\")
sdk_add_compile_definitions(-DCONFIG_WIFI_GATEWAY)
sdk_add_compile_definitions(-DCFG_ENABLE_AP_PS)
#sdk_add_compile_definitions(-DLWIP_HEAP_SIZE=\(32*1024\))
sdk_add_compile_definitions(-DCFG_IP_NAPT_TIMEOUT_MS_TCP=\(30*60*1000\))
sdk_add_compile_definitions(-DCFG_IP_NAPT_TIMEOUT_MS_TCP_DISCON=\(2*20*1000\))
sdk_add_compile_definitions(-DCFG_IP_NAPT_TIMEOUT_MS_UDP=\(620*1000\))
sdk_add_compile_definitions(-DCFG_IP_NAPT_TIMEOUT_MS_ICMP=\(5*2*1000\))
endif()

file(GLOB_RECURSE addsource "${CMAKE_CURRENT_SOURCE_DIR}/wifi/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/custom/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/web/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/http/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/swd_host/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/vfs/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/vfs/target/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/SystemView/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/RTTView/*.c"
)


target_sources(app PRIVATE
wifi_tcp_client.c
wifi_tcp_server.c
usb2uart.c
tinyprintf.c
${CMAKE_CURRENT_SOURCE_DIR}/../../dap_main.c
${CMAKE_CURRENT_SOURCE_DIR}/../../DAP/Source/DAP_vendor.c
${CMAKE_CURRENT_SOURCE_DIR}/../../DAP/Source/DAP.c
${CMAKE_CURRENT_SOURCE_DIR}/../../DAP/Source/JTAG_DP.c
${CMAKE_CURRENT_SOURCE_DIR}/../../DAP/Source/SW_DP.c
${CMAKE_CURRENT_SOURCE_DIR}/../../CherryRB/chry_ringbuffer.c
${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/core/usbd_core.c
${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/port/bouffalolab/usb_dc_bl.c
${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/class/cdc/usbd_cdc_acm.c
${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/class/hid/usbd_hid.c
${CMAKE_CURRENT_SOURCE_DIR}/../../easy_button/ebtn/ebtn.c
${addsource}
)


sdk_add_include_directories(.)
sdk_add_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../CherryRB)
sdk_add_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../easy_button/ebtn)
sdk_add_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)
sdk_add_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../DAP/Include)
sdk_add_include_directories(http)
sdk_add_include_directories(config wifi web)
sdk_add_include_directories(custom)
sdk_add_include_directories(swd_host)
sdk_add_include_directories(vfs)
sdk_add_include_directories(vfs/target)
sdk_add_include_directories(RTTView)
sdk_add_include_directories(SystemView)



sdk_add_compile_definitions(-DCONFIG_USB_HS)
sdk_add_include_directories(../../CherryUSB/common)
sdk_add_include_directories(../../CherryUSB/core)
sdk_add_include_directories(../../CherryUSB/class/cdc)
sdk_add_include_directories(../../CherryUSB/class/msc)
sdk_add_include_directories(../../CherryUSB/class/hid)

#sdk_add_compile_definitions(-DCONFIG_CHERRYDAP_USE_MSC)

sdk_set_main_file(main.c)

project(cherrydap)
