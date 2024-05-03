cmake_minimum_required(VERSION 3.13...3.27)

include(vendor/pico_sdk_import.cmake)

project(littlefs-usb C CXX ASM)
set(PICO_BOARD pico_w)
set(FAMILY rp2040)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(littlefs-usb
  main.c
  onboard_led.c
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
if(${PICO_BOARD} STREQUAL "pico_w")
  target_compile_options(littlefs-usb PRIVATE -DPICO_BOARD_PICO_W)
  target_link_libraries(littlefs-usb PRIVATE pico_cyw43_arch_none)
  target_compile_options(pico_cyw43_arch_none INTERFACE -Wno-unused-parameter)
endif()

target_include_directories(littlefs-usb
  PRIVATE
  vendor/littlefs
  ${CMAKE_CURRENT_LIST_DIR}/include
)
target_compile_options(littlefs-usb PRIVATE -Werror -Wall -Wextra -Wnull-dereference)
target_link_options(littlefs-usb PRIVATE -Wl,--print-memory-usage)


add_library(littlefs INTERFACE)
target_sources(littlefs INTERFACE
  vendor/littlefs/lfs.c
  vendor/littlefs/lfs_util.c
)
target_include_directories(littlefs INTERFACE
  vendor/littlefs
)
target_compile_options(littlefs INTERFACE -Wno-unused-function -Wno-null-dereference)
target_link_libraries(littlefs INTERFACE hardware_flash)

pico_enable_stdio_usb(littlefs-usb 1)
pico_add_extra_outputs(littlefs-usb)

option(TRACE "Enable trace log" OFF)
if(TRACE)
  target_compile_options(littlefs-usb PRIVATE -DENABLE_TRACE)
endif()

find_program(OPENOCD openocd)
if(OPENOCD)
  add_custom_target(flash
    COMMAND ${OPENOCD} -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000" -c "program ${CMAKE_PROJECT_NAME}.elf verify reset exit"
    DEPENDS ${CMAKE_PROJECT_NAME}
  )
  add_custom_target(reset COMMAND ${OPENOCD} -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c init -c reset -c exit)

  add_subdirectory(tests EXCLUDE_FROM_ALL)
endif()
