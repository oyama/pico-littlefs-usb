cmake_minimum_required(VERSION 3.13...3.27)

if(NOT PICO_SDK_PATH)
  set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/pico-sdk)
endif()
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(littlefs-usb C CXX ASM)
set(PICO_BOARD pico_w)
set(FAMILY rp2040)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(littlefs-usb
  main.c
  littlefs_driver.c
  usb_msc_driver.c
  usb_descriptors.c
  mimic_fat.c
  unicode.c
)
target_link_libraries(littlefs-usb PRIVATE
  hardware_flash
  hardware_sync
  littlefs
  pico_stdlib
  tinyusb_additions
  tinyusb_board
  tinyusb_device
)
target_include_directories(littlefs-usb
  PRIVATE
  lib/littlefs
  ${CMAKE_CURRENT_LIST_DIR}/include
)
target_compile_options(littlefs-usb PRIVATE -Werror -Wall -Wextra -Wnull-dereference)
target_link_options(littlefs-usb PRIVATE -Wl,--print-memory-usage)

add_library(littlefs INTERFACE)
target_sources(littlefs INTERFACE
  lib/littlefs/lfs.c
  lib/littlefs/lfs_util.c
)
target_include_directories(littlefs INTERFACE
  lib/littlefs
)
target_compile_options(littlefs INTERFACE -Wno-unused-function -Wno-null-dereference)
target_link_libraries(littlefs INTERFACE hardware_flash)

pico_enable_stdio_usb(littlefs-usb 1)
pico_add_extra_outputs(littlefs-usb)


find_program(OPENOCD openocd)
if(OPENOCD)
  add_custom_target(flash
    COMMAND ${OPENOCD} -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000" -c "program ${CMAKE_PROJECT_NAME}.elf verify reset exit"
    DEPENDS ${CMAKE_PROJECT_NAME}
  )

  add_subdirectory(tests EXCLUDE_FROM_ALL)
endif()
