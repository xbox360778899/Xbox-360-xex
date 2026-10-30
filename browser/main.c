#include <stdbool.h>
#include <stdint.h>

#include "xsdk/gpu.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "xsdk/present.h"
#include "xsdk/text.h"

enum {
    BUFFERS = 2,
    SWAP_INTERVAL = 2,
    BEGIN_TIMEOUT_MS = 5000,

    BACKGROUND = 0xFF101010,
    GREEN = 0xFF76B900,
    WHITE = 0xFFFFFFFF,
    GRAY = 0xFFAAAAAA,

    TAB_COUNT = 6
};

static const char *tabs[TAB_COUNT] = {
    "HOME",
    "GAMES",
    "SEARCH",
    "FAVORITES",
    "PROFILE",
    "SETTINGS"
};

static const char *messages[TAB_COUNT] = {
    "Welcome to Xbox Cloud Gaming",
    "Your game library will appear here",
    "Search for your favourite games",
    "Your favourite games will appear here",
    "Your gaming profile",
    "Browser and streaming settings"
};

static int selected_tab = 0;

static void draw_screen(uint32_t address,
                        uint32_t width,
                        uint32_t height)
{
    xsdk_gpu_set_target(width, height);

    int scale = width >= 1000 ? 2 : 1;
    int left = 30 * scale;
    int top = 25 * scale;

    /* Header */
    xsdk_text_draw(
        left, top, scale, GREEN,
        "XBOX CLOUD"
    );

    xsdk_text_draw(
        left, top + 35 * scale,
        scale, GRAY,
        "CLOUD GAMING PLATFORM"
    );

    /* Navigation */
    int nav_y = top + 100 * scale;
    int nav_x = left;

    for (int i = 0; i < TAB_COUNT; i++) {
        uint32_t color =
            (i == selected_tab) ? GREEN : WHITE;

        xsdk_text_draw(
            nav_x,
            nav_y,
            scale,
            color,
            tabs[i]
        );

        nav_x += 135 * scale;
    }

    /* Selected page */
    xsdk_text_draw(
        left,
        top + 220 * scale,
        scale * 2,
        GREEN,
        tabs[selected_tab]
    );

    xsdk_text_draw(
        left,
        top + 285 * scale,
        scale,
        WHITE,
        messages[selected_tab]
    );

    /* Footer */
    xsdk_text_draw(
        left,
        height - 100 * scale,
        scale,
        GRAY,
        "LEFT / RIGHT: NAVIGATE"
    );

    xsdk_text_draw(
        left,
        height - 65 * scale,
        scale,
        WHITE,
        "A: SELECT     B: BACK     BACK: EXIT"
    );

    xsdk_gpu_resolve(
        address,
        width,
        height,
        BACKGROUND
    );
}

static void run_browser(uint32_t width, uint32_t height)
{
    uint16_t previous = 0;

    while (true) {
        INPUT_STATE input = {0};

        uint32_t result =
            XamInputGetState(0, 0, &input);

        if (result != ERROR_SUCCESS)
            input = (INPUT_STATE){0};

        uint16_t buttons = input.gamepad.buttons;
        uint16_t pressed = buttons & ~previous;

        previous = buttons;

        if (pressed & INPUT_GAMEPAD_BACK)
            break;

        if (pressed & INPUT_GAMEPAD_DPAD_RIGHT) {
            selected_tab++;

            if (selected_tab >= TAB_COUNT)
                selected_tab = 0;
        }

        if (pressed & INPUT_GAMEPAD_DPAD_LEFT) {
            selected_tab--;

            if (selected_tab < 0)
                selected_tab = TAB_COUNT - 1;
        }

        if (pressed & INPUT_GAMEPAD_A) {
            xsdk_log_write(
                "Selected tab: %s",
                tabs[selected_tab]
            );
        }

        uint32_t address = 0;

        NTSTATUS status =
            xsdk_present_begin(
                &address,
                BEGIN_TIMEOUT_MS
            );

        if (!NT_SUCCESS(status)) {
            xsdk_log_write(
                "Present failed: 0x%08X",
                (unsigned)status
            );
            break;
        }

        draw_screen(address, width, height);

        xsdk_present_end();
    }
}

int main(void)
{
    xsdk_log_write("Xbox Cloud UI starting");

    NTSTATUS status = xsdk_gpu_open();

    if (!NT_SUCCESS(status)) {
        xsdk_log_write("GPU open failed: 0x%08X",
                       (unsigned)status);
        return 1;
    }

    uint32_t width = 0;
    uint32_t height = 0;

    status = xsdk_present_open(
        XSDK_DISPLAY_720P,
        BUFFERS,
        SWAP_INTERVAL,
        &width,
        &height
    );

    if (!NT_SUCCESS(status)) {
        xsdk_log_write("Display open failed: 0x%08X",
                       (unsigned)status);
        xsdk_gpu_close();
        return 1;
    }

    status = xsdk_text_open();

    if (!NT_SUCCESS(status)) {
        xsdk_log_write("Text open failed: 0x%08X",
                       (unsigned)status);
        xsdk_present_close();
        xsdk_gpu_close();
        return 1;
    }

    xsdk_log_write(
        "Display: %ux%u",
        (unsigned)width,
        (unsigned)height
    );

    run_browser(width, height);

    xsdk_text_close();
    xsdk_present_close();

    status = xsdk_gpu_close();

    if (!NT_SUCCESS(status))
        xsdk_log_write("GPU close failed");

    xsdk_log_write("Xbox Cloud UI closed");

    return 0;
}
