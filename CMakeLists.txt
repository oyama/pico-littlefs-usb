cmake_minimum_required(VERSION 3.13...3.27)

if(NOT PICO_SDK_PATH)
  set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/pico-sdk)
endif()
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(littlefs-usb C CXX ASM)

# set(PICO_BOARD pico_w)
set(FAMILY rp2040)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(littlefs-usb
  main.c
  littlefs_driver.c
  usb_fat_mimic_driver.c
  usb_descriptors.c
)
target_link_libraries(littlefs-usb
  pico_stdlib
  hardware_flash
  hardware_sync
  littlefs
  tinyusb_additions
  tinyusb_board
  tinyusb_device
)

target_include_directories(littlefs-usb
  PRIVATE
  lib/littlefs
  ${CMAKE_CURRENT_LIST_DIR}
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPICO_USE_MALLOC_MUTEX=1")
add_library(littlefs INTERFACE)
target_include_directories(littlefs INTERFACE
  lib/littlefs
)
target_link_libraries(littlefs INTERFACE hardware_flash)
target_sources(littlefs INTERFACE
   lib/littlefs/lfs.c
   lib/littlefs/lfs_util.c
)

pico_enable_stdio_usb(littlefs-usb 1)
pico_add_extra_outputs(littlefs-usb)
