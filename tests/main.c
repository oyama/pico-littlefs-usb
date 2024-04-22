#include "tests.h"
#include <bsp/board.h>
#include <tusb.h>


int main(void) {
    board_init();
    tud_init(BOARD_TUD_RHPORT);
    stdio_init_all();

    printf("Start all tests\n");

    test_read();
    test_write();

    printf("All tests are ok\n");
}
