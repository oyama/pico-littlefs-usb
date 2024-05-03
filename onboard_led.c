#include "onboard_led.h"


void onboard_led_init(void) {
#if defined(PICO_BOARD_PICO_W)
    cyw43_arch_init();
    cyw43_arch_enable_sta_mode();
#else
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif
}

void onboard_led(bool enable) {
#if defined(PICO_BOARD_PICO_W)
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, enable);
#else
    gpio_put(PICO_DEFAULT_LED_PIN, enable);
#endif
}
