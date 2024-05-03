#include "onboard_led.h"


void onboard_led_init(void) {
#if defined(PICO_BOARD_PICO_W)
    cyw43_arch_init();
#else
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif
}

void onboard_led(bool flag) {
#if defined(PICO_BOARD_PICO_W)
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, flag);
#else
    gpio_put(PICO_DEFAULT_LED_PIN, flag);
#endif
}
