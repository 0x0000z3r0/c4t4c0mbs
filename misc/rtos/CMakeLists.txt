cmake_minimum_required(VERSION 3.16)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv64)
set(CMAKE_C_COMPILER riscv64-unknown-elf-gcc)
set(CMAKE_ASM_COMPILER riscv64-unknown-elf-gcc)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(rtos-samples LANGUAGES C ASM)

set(FREERTOS_KERNEL ${CMAKE_SOURCE_DIR}/FreeRTOS-Kernel)
set(FREERTOS_TCP ${CMAKE_SOURCE_DIR}/FreeRTOS-Plus-TCP)

set(RTOS_CFLAGS
	-march=rv64imac
	-mabi=lp64
	-mcmodel=medany
	-Wall
	-Wextra
	-g3
	-O0
	--specs=picolibc.specs
)

set(RTOS_INCLUDES
	${CMAKE_SOURCE_DIR}/common
	${FREERTOS_KERNEL}/include
	${FREERTOS_KERNEL}/portable/GCC/RISC-V
	${FREERTOS_TCP}/source/include
	${FREERTOS_TCP}/source/portable/Compiler/GCC
)

set(PLATFORM_SOURCES
	common/uart.c
	common/NetworkInterface.c
	common/static_tasks.c
	${FREERTOS_KERNEL}/tasks.c
	${FREERTOS_KERNEL}/queue.c
	${FREERTOS_KERNEL}/list.c
	${FREERTOS_KERNEL}/timers.c
	${FREERTOS_KERNEL}/event_groups.c
	${FREERTOS_KERNEL}/portable/GCC/RISC-V/port.c
	${FREERTOS_KERNEL}/portable/GCC/RISC-V/portASM.S
	${FREERTOS_KERNEL}/portable/MemMang/heap_4.c
	${FREERTOS_TCP}/source/FreeRTOS_ARP.c
	${FREERTOS_TCP}/source/FreeRTOS_BitConfig.c
	${FREERTOS_TCP}/source/FreeRTOS_DHCP.c
	${FREERTOS_TCP}/source/FreeRTOS_DHCPv6.c
	${FREERTOS_TCP}/source/FreeRTOS_DNS.c
	${FREERTOS_TCP}/source/FreeRTOS_DNS_Cache.c
	${FREERTOS_TCP}/source/FreeRTOS_DNS_Callback.c
	${FREERTOS_TCP}/source/FreeRTOS_DNS_Networking.c
	${FREERTOS_TCP}/source/FreeRTOS_DNS_Parser.c
	${FREERTOS_TCP}/source/FreeRTOS_ICMP.c
	${FREERTOS_TCP}/source/FreeRTOS_IP.c
	${FREERTOS_TCP}/source/FreeRTOS_IP_Timers.c
	${FREERTOS_TCP}/source/FreeRTOS_IP_Utils.c
	${FREERTOS_TCP}/source/FreeRTOS_IPv4.c
	${FREERTOS_TCP}/source/FreeRTOS_IPv4_Sockets.c
	${FREERTOS_TCP}/source/FreeRTOS_IPv4_Utils.c
	${FREERTOS_TCP}/source/FreeRTOS_IPv6.c
	${FREERTOS_TCP}/source/FreeRTOS_IPv6_Sockets.c
	${FREERTOS_TCP}/source/FreeRTOS_IPv6_Utils.c
	${FREERTOS_TCP}/source/FreeRTOS_ND.c
	${FREERTOS_TCP}/source/FreeRTOS_RA.c
	${FREERTOS_TCP}/source/FreeRTOS_Routing.c
	${FREERTOS_TCP}/source/FreeRTOS_Sockets.c
	${FREERTOS_TCP}/source/FreeRTOS_Stream_Buffer.c
	${FREERTOS_TCP}/source/FreeRTOS_TCP_IP.c
	${FREERTOS_TCP}/source/FreeRTOS_TCP_IP_IPv4.c
	${FREERTOS_TCP}/source/FreeRTOS_TCP_IP_IPv6.c
	${FREERTOS_TCP}/source/FreeRTOS_TCP_Reception.c
	${FREERTOS_TCP}/source/FreeRTOS_TCP_State_Handling.c
	${FREERTOS_TCP}/source/FreeRTOS_TCP_State_Handling_IPv4.c
	${FREERTOS_TCP}/source/FreeRTOS_TCP_State_Handling_IPv6.c
	${FREERTOS_TCP}/source/FreeRTOS_TCP_Transmission.c
	${FREERTOS_TCP}/source/FreeRTOS_TCP_Transmission_IPv4.c
	${FREERTOS_TCP}/source/FreeRTOS_TCP_Transmission_IPv6.c
	${FREERTOS_TCP}/source/FreeRTOS_TCP_Utils.c
	${FREERTOS_TCP}/source/FreeRTOS_TCP_Utils_IPv4.c
	${FREERTOS_TCP}/source/FreeRTOS_TCP_Utils_IPv6.c
	${FREERTOS_TCP}/source/FreeRTOS_TCP_WIN.c
	${FREERTOS_TCP}/source/FreeRTOS_Tiny_TCP.c
	${FREERTOS_TCP}/source/FreeRTOS_UDP_IP.c
	${FREERTOS_TCP}/source/FreeRTOS_UDP_IPv4.c
	${FREERTOS_TCP}/source/FreeRTOS_UDP_IPv6.c
	${FREERTOS_TCP}/source/portable/BufferManagement/BufferAllocation_2.c
)

add_library(rtos_platform STATIC ${PLATFORM_SOURCES})
target_include_directories(rtos_platform PUBLIC ${RTOS_INCLUDES})
target_compile_options(rtos_platform PRIVATE ${RTOS_CFLAGS})

function(add_rtos_sample sample_dir)
	add_executable(${sample_dir}
		${sample_dir}/main.c
		common/start.S
	)
	target_include_directories(${sample_dir} PRIVATE ${RTOS_INCLUDES})
	target_compile_options(${sample_dir} PRIVATE ${RTOS_CFLAGS})
	target_link_libraries(${sample_dir} PRIVATE rtos_platform)
	target_link_options(${sample_dir} PRIVATE
		${RTOS_CFLAGS}
		-nostartfiles
		-Wl,--gc-sections
		-T ${CMAKE_SOURCE_DIR}/common/fake_rom.ld
	)
	set_target_properties(${sample_dir} PROPERTIES
		OUTPUT_NAME rtos_proxy
		SUFFIX .elf
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${sample_dir}
	)

	add_custom_target(run-${sample_dir}
		COMMAND qemu-system-riscv64 -machine virt -bios none -display none
			-serial stdio
			-chardev socket,id=netuart,host=127.0.0.1,port=8888,server=on,wait=off
			-device pci-serial,chardev=netuart
			-kernel $<TARGET_FILE:${sample_dir}>
		DEPENDS ${sample_dir}
		USES_TERMINAL
	)
endfunction()

add_rtos_sample(01-freertos-overflow)
add_rtos_sample(02-executable-stack)
add_rtos_sample(03-toctou)
