cmake_minimum_required(VERSION 3.13)

# Clear default cmake flags. Cmake overrides -03 on Release, and -g on Debug
set(CMAKE_C_FLAGS_DEBUG     "" CACHE STRING "Release C flags" FORCE)
set(CMAKE_C_FLAGS_RELEASE   "" CACHE STRING "Release C flags" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG   "" CACHE STRING "Release CXX flags" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "" CACHE STRING "Release CXX flags" FORCE)
set(CMAKE_ASM_FLAGS_DEBUG   "" CACHE STRING "Release ASM flags" FORCE)
set(CMAKE_ASM_FLAGS_RELEASE "" CACHE STRING "Release ASM flags" FORCE)


list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/hwlt-framework/cmake")

# Import project config
include(hwlt-config.cmake)

# disable lfs to whole project
set(CONFIG_ENABLE_LFS   OFF CACHE BOOL "Override CONFIG_ENABLE_LFS to OFF" FORCE)
# disable lvgl to whole project
set(CONFIG_ENABLE_LVGL  OFF CACHE BOOL "Override CONFIG_ENABLE_LVGL to OFF" FORCE)
include(hwlt-framework)

project(era-bootloader)

# General reduce BL firmware size
add_compile_definitions(CONFIG_DISABLE_NFC_CARDS)

option(CONFIG_BOOT_USE_LIBSE "Use specific library for verification signature" ON)

if(CMAKE_BUILD_TYPE MATCHES "Release")
    add_compile_definitions(REDUCE_LOGGER_SIZE)
    if(CONFIG_SHELL_ENABLED)
        add_compile_definitions(SPECIFIED_LOG_LEVEL=ELOG_LVL_ASSERT)
        add_compile_definitions(REDUCE_DEBUG_FIRMWARE_SIZE)
    elseif(CONFIG_DEBUG_UART_ENABLED)
        add_compile_definitions(SPECIFIED_LOG_LEVEL=ELOG_LVL_ERROR)
        add_compile_definitions(REDUCE_DEBUG_FIRMWARE_SIZE)
    else()
        add_compile_definitions(CONFIG_FORCE_DISABLE_LOGGER)
    endif()

    set (HWLT_MCUBOOT_LDFLAGS
        -T${CMAKE_SOURCE_DIR}/scripts/ld/STM32H743AIIX_FLASH_hwlt_mcuboot.ld
    )

    add_compile_definitions(
        USER_VECT_TAB_OFFSET=0x020400
        CONFIG_WDT_REFRESH_REQUIRED
    )
    
else()

    add_compile_definitions(
        DEBUG
    )
    if (CMAKE_DEBUG_USE_WDG)
        add_compile_definitions(
            WDT_ENABLED
        )
    endif()

endif()

if(CONFIG_WALLET_USE_MBEDTLS)
    add_compile_definitions(CONFIG_WALLET_USE_MBEDTLS)
endif()

if(CONFIG_BOOT_USE_LIBSE)
    add_compile_definitions(MCUBOOT_USE_LIBSE)
else()
    add_subdirectory(${CMAKE_SOURCE_DIR}/mbedTLS/include)
    include_directories(mbedTLS/include)
endif()

include_directories(${HWLT_BOARD_INCDIR}
                    ${HWLT_FREERTOS_INCDIRS}
                    mcuboot/boot/bootutil/include/
                    src/mcuboot/Inc
                    )

link_directories("${CMAKE_SOURCE_DIR}/mcuboot/boot/bootutil")

add_subdirectory(${CMAKE_SOURCE_DIR}/mcuboot/boot/bootutil)

add_subdirectory(src)
