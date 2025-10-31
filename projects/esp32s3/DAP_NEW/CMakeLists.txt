set(COMPONENT_ADD_INCLUDEDIRS "include/" "config/" "cmsis-core")
set(COMPONENT_SRCS 
			"source/DAP.c "
			"source/DAP_vendor.c"
			"source/JTAG_DP.c"
			"source/SW_DP.c"
			"source/swd_host.c"
			"source/error.c"
			"source/SWO.c"
			"source/UART.c"
			"source/spi_op.c"
			"source/spi_switch.c"
			"source/dap_utility.c"
			)
set(COMPONENT_REQUIRES driver main)
register_component()