#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "xsdk/gpu.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "xsdk/present.h"
#include "xsdk/text.h"

#define TAB_COUNT 6

#define BUFFER_COUNT 2
#define SWAP_INTERVAL 2
#define FRAME_TIMEOUT 5000

#define COLOR_BG      0xFF101010
#define COLOR_GREEN   0xFF76B900
#define COLOR_WHITE   0xFFFFFFFF
#define COLOR_GRAY    0xFFAAAAAA
#define COLOR_DARK    0xFF202020

static const char *tabs[TAB_COUNT] = {
    "HOME",
    "GAMES",
    "SEARCH",
    "FAVORITES",
    "PROFILE",
    "SETTINGS"
};

static const char *descriptions[TAB_COUNT] = {
    "Welcome to Xbox Cloud Gaming",
    "Your cloud game library",
    "Find your favourite games",
    "Your saved games",
    "Your gaming profile",
    "Application settings"
};

static uint32_t selected_tab = 0;

/* Draw Home Screen */

static void draw_screen(
    uint32_t address,
    uint32_t width,
    uint32_t height
)
{
    xsdk_gpu_set_target(width, height);

    const int scale = 1;
    const int margin = 40;

    /* Header */

    xsdk_text_draw(
        margin,
        30,
        2,
        COLOR_GREEN,
        "XBOX CLOUD"
    );

    xsdk_text_draw(
        margin,
        75,
        scale,
        COLOR_GRAY,
        "CLOUD GAMING PLATFORM"
    );

    /* Navigation Bar */

    int nav_x = margin;
    int nav_y = 145;

    const int spacing = 150;

    for (uint32_t i = 0; i < TAB_COUNT; i++)
    {
        uint32_t color = COLOR_WHITE;

        if (i == selected_tab)
            color = COLOR_GREEN;

        xsdk_text_draw(
            nav_x,
            nav_y,
            scale,
            color,
            tabs[i]
        );

        nav_x += spacing;
    }

    /* Selected Tab */

    xsdk_text_draw(
        margin,
        260,
        2,
        COLOR_GREEN,
        tabs[selected_tab]
    );

    xsdk_text_draw(
        margin,
        320,
        scale,
        COLOR_WHITE,
        descriptions[selected_tab]
    );

    /* Home Screen Message */

    if (selected_tab == 0)
    {
        xsdk_text_draw(
            margin,
            390,
            scale,
            COLOR_GRAY,
            "YOUR GAMES. YOUR WORLD."
        );

        xsdk_text_draw(
            margin,
            440,
            scale,
            COLOR_GREEN,
            "CLOUD GAMING READY"
        );
    }

    /* Footer */

    xsdk_text_draw(
        margin,
        height - 85,
        scale,
        COLOR_GRAY,
        "LEFT / RIGHT: NAVIGATE"
    );

    xsdk_text_draw(
        margin,
        height - 55,
        scale,
        COLOR_WHITE,
        "A: SELECT     B: HOME     BACK: EXIT"
    );

    /* Present frame */

    xsdk_gpu_resolve(
        address,
        width,
        height,
        COLOR_BG
    );
}

/* Controller */

static bool run_browser(
    uint32_t width,
    uint32_t height
)
{
    uint16_t previous_buttons = 0;

    while (true)
    {
        INPUT_STATE input = {0};

        uint32_t input_result =
            XamInputGetState(0, 0, &input);

        if (input_result != ERROR_SUCCESS)
        {
            input = (INPUT_STATE){0};
        }

        uint16_t buttons = input.gamepad.buttons;

        uint16_t pressed =
            (uint16_t)(buttons & (uint16_t)~previous_buttons);

        previous_buttons = buttons;

        /* Exit */

        if (pressed & INPUT_GAMEPAD_BACK)
        {
            xsdk_log_write("Browser exit requested");
            break;
        }

        /* Right */

        if (pressed & INPUT_GAMEPAD_DPAD_RIGHT)
        {
            selected_tab =
                (selected_tab + 1u) % TAB_COUNT;
        }

        /* Left */

        if (pressed & INPUT_GAMEPAD_DPAD_LEFT)
        {
            selected_tab =
                (selected_tab + TAB_COUNT - 1u) % TAB_COUNT;
        }

        /* Select */

        if (pressed & INPUT_GAMEPAD_A)
        {
            xsdk_log_write(
                "Selected: %s",
                tabs[selected_tab]
            );
        }

        /* Back to Home */

        if (pressed & INPUT_GAMEPAD_B)
        {
            selected_tab = 0;
        }

        /* Begin Frame */

        uint32_t address = 0;

        NTSTATUS status =
            xsdk_present_begin(
                &address,
                FRAME_TIMEOUT
            );

        if (!NT_SUCCESS(status))
        {
            xsdk_log_write(
                "Frame error: 0x%08X",
                (unsigned)status
            );

            return false;
        }

        draw_screen(
            address,
            width,
            height
        );

        xsdk_present_end();
    }

    return true;
}

/* Application Entry */

int main(void)
{
    xsdk_log_write("Xbox Cloud Gaming UI starting");

    NTSTATUS status = xsdk_gpu_open();

    if (!NT_SUCCESS(status))
    {
        xsdk_log_write(
            "GPU initialization failed: 0x%08X",
            (unsigned)status
        );

        return 1;
    }

    uint32_t width = 0;
    uint32_t height = 0;

    status = xsdk_present_open(
        XSDK_DISPLAY_720P,
        BUFFER_COUNT,
        SWAP_INTERVAL,
        &width,
        &height
    );

    if (!NT_SUCCESS(status))
    {
        xsdk_log_write(
            "Display initialization failed: 0x%08X",
            (unsigned)status
        );

        xsdk_gpu_close();

        return 1;
    }

    status = xsdk_text_open();

    if (!NT_SUCCESS(status))
    {
        xsdk_log_write(
            "Text initialization failed: 0x%08X",
            (unsigned)status
        );

        xsdk_present_close();
        xsdk_gpu_close();

        return 1;
    }

    xsdk_log_write(
        "Display initialized: %ux%u",
        (unsigned)width,
        (unsigned)height
    );

    bool success = run_browser(width, height);

    /* Shutdown */

    xsdk_text_close();

    xsdk_present_close();

    xsdk_gpu_close();

    xsdk_log_write(
        "Xbox Cloud Gaming UI stopped: %s",
        success ? "OK" : "ERROR"
    );

    return success ? 0 : 1;
}
