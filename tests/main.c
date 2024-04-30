#include "tests.h"
#include <bsp/board.h>
#include <tusb.h>


#define ANSI_GREEN  "\e[32m"
#define ANSI_CLEAR  "\e[0m"

int main(void) {
    board_init();
    tud_init(BOARD_TUD_RHPORT);
    stdio_init_all();

    printf("Start all tests\n");

    test_create();
    test_read();
    test_update();
    test_rename();
    test_move();
    test_delete();

    test_change_storage_size();

    printf(ANSI_GREEN "All tests are ok\n" ANSI_CLEAR);
}
