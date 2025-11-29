

# Add hwlt-framework firmware target
hwlt_add_target(NAME era-bootloader SRCS boot_app.c LDFLAGS ${HWLT_MCUBOOT_LDFLAGS})

file(GLOB_RECURSE IMG_ARRAYS "gui/img/*.c")
file(GLOB_RECURSE FONT_ARRAYS "gui/fonts/*.c")
file(GLOB_RECURSE GUI_SCREENS "gui/screens*.c")
file(GLOB_RECURSE GUI_SOURCE "gui/*.c")
file(GLOB_RECURSE BOOT_SOURCE "mcuboot/Src/*.c")

# Reduce BL size when building with shell + uart (test firmware)
if((CMAKE_BUILD_TYPE MATCHES "Release") AND CONFIG_DEBUG_UART_ENABLED)
    add_compile_definitions(REDUCE_FLASH_USAGE_BY_REMOVING_ANIMATIONS)
    add_compile_definitions(REDUCE_FLASH_USAGE_BY_REMOVING_FONTS)
endif()

target_sources(era-bootloader PUBLIC  ${BOOT_SOURCE}
                                      ${GUI_SOURCE}
                                      ${GUI_SCREENS}
                                      ${IMG_ARRAYS} 
                                      ${FONT_ARRAYS}
                                      shell/shell_bootloader.c
                                    )

set(BOOTUTIL_DIR ${CMAKE_SOURCE_DIR}/mcuboot/boot/bootutil)
target_include_directories(era-bootloader PUBLIC  ${BOOTUTIL_DIR}/include
                                                ${BOOTUTIL_DIR}/src
                                                ${CMAKE_SOURCE_DIR}/src/gui
                                                ${CMAKE_SOURCE_DIR}/src/gui/fonts
                                                ${CMAKE_SOURCE_DIR}/src/gui/screens
                                                ${CMAKE_SOURCE_DIR}/src/shell
                                                ${CMAKE_SOURCE_DIR}/src
                                                ${CMAKE_SOURCE_DIR}
                                                )

target_link_libraries(era-bootloader PUBLIC lvgl)
target_compile_options(lvgl PUBLIC ${HWLT_CFLAGS})  

target_link_libraries(era-bootloader PUBLIC bootutil)
target_include_directories(bootutil PUBLIC  ${HWLT_EASYLOGGER_INCDIRS}    # for elog.h
                                            )
target_compile_options(bootutil PRIVATE ${HWLT_CFLAGS})

if(CMAKE_BUILD_TYPE MATCHES "Release")

    file(READ "${CMAKE_SOURCE_DIR}/version_project.h" version_file)
    string(REGEX MATCH "#define[ ]+VERSION_PROJECT_BASE [ \"]+[0-9]+.[0-9]+" stringdef "${version_file}")
    string(REGEX MATCH "[0-9]+" MajorVersion "${stringdef}")
    string(REPLACE "${MajorVersion}." "" stringdef ${stringdef})
    string(REGEX MATCH "[0-9]+" MinorVersion "${stringdef}")

    string(REGEX MATCH "#define[ ]+VERSION_PROJECT_BUILDNUM [ \"]+[0-9]+" stringdef "${version_file}")
    string(REGEX MATCH "[0-9]+" build "${stringdef}")
    set(MinorVersion ${MinorVersion}.${build})

    if(CMAKE_BUILD_TYPE MATCHES "Newkey")
        add_custom_command(
            TARGET era-bootloader
            POST_BUILD
            COMMAND ${CMAKE_SOURCE_DIR}/scripts/sign_image_new_key.sh 
            ARGS ${CMAKE_BINARY_DIR}/src ${MajorVersion} ${MinorVersion}
        )
    else()
        add_custom_command(
            TARGET era-bootloader
            POST_BUILD
            COMMAND ${CMAKE_SOURCE_DIR}/scripts/sign_image.sh 
            ARGS ${CMAKE_BINARY_DIR}/src ${MajorVersion} ${MinorVersion}
        )
    endif()
    
endif()
