#include "tests.h"
#include <bsp/board.h>
#include <tusb.h>
#include "onboard_led.h"


#define ANSI_GREEN  "\e[32m"
#define ANSI_CLEAR  "\e[0m"

int main(void) {
    board_init();
    tud_init(BOARD_TUD_RHPORT);
    stdio_init_all();
    onboard_led_init();

    printf("Start all tests\n");

    test_create();
    test_read();
    test_update();
    test_rename();
    test_move();
    test_delete();

    test_large_file();

    printf(ANSI_GREEN "All tests are ok\n" ANSI_CLEAR);
}
