set(CMAKE_BUILD_TYPE Debug)

add_executable(tests
  ../mimic_fat.c
  ../littlefs_driver.c
  ../unicode.c
  ../usb_msc_driver.c
  ../usb_descriptors.c
  ../onboard_led.c
  main.c
  util.c
  test_create.c
  test_read.c
  test_update.c
  test_rename.c
  test_move.c
  test_delete.c
  test_large_file.c
)

target_link_libraries(tests PRIVATE
  hardware_flash
  hardware_sync
  littlefs
  pico_stdlib
  tinyusb_additions
  tinyusb_board
  tinyusb_device
)
if(${PICO_BOARD} STREQUAL "pico_w")
  target_compile_options(tests PRIVATE -DPICO_BOARD_PICO_W)
  target_link_libraries(tests PRIVATE pico_cyw43_arch_none)
endif()

target_include_directories(tests
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../include
)
#target_compile_options(tests PRIVATE -Werror -Wall -Wextra -Wnull-dereference)
#target_compile_options(tests PRIVATE -DENABLE_TRACE)

pico_add_extra_outputs(tests)
pico_enable_stdio_usb(tests 1)


if(OPENOCD)
  add_custom_target(run_tests
    COMMAND ${OPENOCD} -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000" -c "program tests.elf verify reset exit"
    DEPENDS tests
  )
endif()
