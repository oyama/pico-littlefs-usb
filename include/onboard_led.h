#ifndef PICO_LITTLEFS_USB_ONBOARD_LED_H_
#define PICO_LITTLEFS_USB_ONBOARD_LED_H_

#include <stdbool.h>
#include <pico/stdlib.h>

#if defined(PICO_BOARD_PICO_W)
#include <pico/cyw43_arch.h>
#endif

void onboard_led_init(void);
void onboard_led(bool enable);

#endif
