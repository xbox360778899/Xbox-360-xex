#include <stdbool.h>
#include <stdint.h>

#include "xsdk/gpu.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "xsdk/present.h"
#include "xsdk/text.h"

#define BUFFER_COUNT 2
#define SWAP_INTERVAL 1
#define FRAME_TIMEOUT 5000

#define BG       0xFF101010
#define GREEN    0xFF76B900
#define WHITE    0xFFFFFFFF
#define GRAY     0xFF999999
#define MUTED    0xFF555555

#define NAV_COUNT 7
#define GAME_COUNT 4

static const char *navigation[NAV_COUNT] = {
    "HOME", "GAMES", "SEARCH", "LIBRARY",
    "FAVORITES", "PROFILE", "SETTINGS"
};

static const char *games[GAME_COUNT] = {
    "GRAND THEFT AUTO V",
    "FORZA HORIZON",
    "MINECRAFT",
    "ROBLOX"
};

static uint32_t nav_selected = 0;
static uint32_t game_selected = 0;

static void draw_ui(uint32_t address,
                    uint32_t width,
                    uint32_t height)
{
    /* Header */
    xsdk_text_draw(
        45, 30, 2, GREEN,
        "GEFORCE NOW"
    );

    xsdk_text_draw(
        850, 40, 1, WHITE,
        "XBOX 360 EDITION"
    );

    xsdk_text_draw(
        45, 100, 1, GRAY,
        "YOUR CLOUD GAMING DESTINATION"
    );

    /* Navigation */
    int32_t x = 45;

    for (uint32_t i = 0; i < NAV_COUNT; i++) {
        uint32_t color =
            (i == nav_selected) ? GREEN : WHITE;

        xsdk_text_draw(
            x, 155, 1, color,
            navigation[i]
        );

        x += 165;
    }

    /* Main heading */
    xsdk_text_draw(
        45, 235, 2, WHITE,
        "DISCOVER YOUR NEXT GAME"
    );

    xsdk_text_draw(
        45, 290, 1, GRAY,
        "SELECT A GAME WITH YOUR CONTROLLER"
    );

    /* Game Cards - Text placeholders */
    for (uint32_t i = 0; i < GAME_COUNT; i++) {
        int32_t card_x = 45 + (int32_t)i * 295;
        uint32_t color =
            (i == game_selected) ? GREEN : GRAY;

        xsdk_text_draw(
            card_x, 390, 1, color,
            "------------------------"
        );

        xsdk_text_draw(
            card_x, 430, 1, color,
            games[i]
        );

        xsdk_text_draw(
            card_x, 465, 1, MUTED,
            "GAME COVER"
        );

        xsdk_text_draw(
            card_x, 500, 1, color,
            "------------------------"
        );
    }

    /* Footer */
    xsdk_text_draw(
        45, height - 100, 1, GRAY,
        "DPAD: NAVIGATE"
    );

    xsdk_text_draw(
        45, height - 65, 1, WHITE,
        "A: SELECT    B: HOME    BACK: EXIT"
    );

    /* Display */
    xsdk_gpu_resolve(
        address,
        width,
        height,
        BG
    );
}

static void run_ui(uint32_t width, uint32_t height)
{
    uint16_t previous = 0;

    while (true) {
        INPUT_STATE input = {0};

        if (XamInputGetState(0, 0, &input)
            != ERROR_SUCCESS) {
            input = (INPUT_STATE){0};
        }

        uint16_t buttons = input.gamepad.buttons;
        uint16_t pressed =
            (uint16_t)(buttons & (uint16_t)~previous);

        previous = buttons;

        if (pressed & INPUT_GAMEPAD_BACK)
            break;

        if (pressed & INPUT_GAMEPAD_DPAD_RIGHT) {
            if (nav_selected < NAV_COUNT - 1)
                nav_selected++;
        }

        if (pressed & INPUT_GAMEPAD_DPAD_LEFT) {
            if (nav_selected > 0)
                nav_selected--;
        }

        if (pressed & INPUT_GAMEPAD_DPAD_DOWN) {
            if (game_selected < GAME_COUNT - 1)
                game_selected++;
        }

        if (pressed & INPUT_GAMEPAD_DPAD_UP) {
            if (game_selected > 0)
                game_selected--;
        }

        if (pressed & INPUT_GAMEPAD_B) {
            nav_selected = 0;
            game_selected = 0;
        }

        if (pressed & INPUT_GAMEPAD_A) {
            xsdk_log_write(
                "Selected game: %s",
                games[game_selected]
            );
        }

        uint32_t address = 0;

        NTSTATUS status =
            xsdk_present_begin(
                &address,
                FRAME_TIMEOUT
            );

        if (!NT_SUCCESS(status)) {
            xsdk_log_write(
                "Present failed: 0x%08X",
                (unsigned)status
            );
            break;
        }

        draw_ui(address, width, height);

        xsdk_present_end();
    }
}

int main(void)
{
    xsdk_log_write("GeForce UI starting");

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
        BUFFER_COUNT,
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
        "Display ready: %ux%u",
        (unsigned)width,
        (unsigned)height
    );

    run_ui(width, height);

    xsdk_text_close();
    xsdk_present_close();
    xsdk_gpu_close();

    xsdk_log_write("GeForce UI closed");

    return 0;
}
