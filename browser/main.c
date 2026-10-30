#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "xsdk/gpu.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "xsdk/present.h"
#include "xsdk/text.h"
#include "xsdk/xam.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

static char current_url[256] = "https://www.google.com";
static char status_text[128] = "Ready";


static const uint32_t BACKGROUND = 0xFF101820;
static const uint32_t GREEN = 0xFF107C10;
static const uint32_t WHITE = 0xFFFFFFFF;
static const uint32_t GRAY = 0xFFAAAAAA;

static void draw_text(int x, int y, const char *text,
                      uint32_t color)
{
    xsdk_text_draw(x, y, 2, color, text);
}

static void open_url_keyboard(void)
{
    uint16_t title[] = {
        'X','b','o','x',' ','B','r','o','w','s','e','r',0
    };

    uint16_t description[] = {
        'E','n','t','e','r',' ','W','e','b','s','i','t','e',
        ' ','U','R','L',0
    };

    uint16_t input[256] = {0};
    XAM_OVERLAPPED overlapped = {0};

    xsdk_xam_prepare(&overlapped);

    uint32_t result = xsdk_xam_keyboard(
        0,
        title,
        description,
        NULL,
        input,
        256,
        &overlapped
    );

    result = xsdk_xam_returned(&overlapped, result);

    if (result == ERROR_IO_PENDING)
        result = xsdk_xam_wait(&overlapped, 30000, NULL);

    if (result == ERROR_SUCCESS && input[0] != 0)
    {
        unsigned int i = 0;

        while (i < sizeof(current_url) - 1 &&
               input[i] != 0 &&
               input[i] < 128)
        {
            current_url[i] = (char)input[i];
            i++;
        }

        current_url[i] = '\0';

        snprintf(status_text, sizeof(status_text),
                 "URL entered - Ready to connect");
    }
}

static void draw_browser(uint32_t address,
                         uint32_t width,
                         uint32_t height)
{
    xsdk_gpu_set_target(width, height);

    /* Header */
    xsdk_text_draw(35, 25, 3, GREEN, "XBOX BROWSER");

    draw_text(35, 105,
              "Internet Browser for Xbox 360",
              WHITE);

    /* Address bar */
    xsdk_gpu_set_target(width, height);

    draw_text(35, 175, "ADDRESS:", GRAY);
    draw_text(35, 220, current_url, WHITE);

    draw_text(35, 285,
              "--------------------------------------------",
              GREEN);

    /* Browser home screen */
    draw_text(35, 350, "WELCOME", GREEN);

    draw_text(35, 405,
              "Your Xbox Browser is ready.",
              WHITE);

    draw_text(35, 455,
              "Enter a website URL using the A button.",
              GRAY);

    /* Status */
    draw_text(35, 540, "STATUS:", GREEN);
    draw_text(180, 540, status_text, WHITE);

    /* Controls */
    draw_text(35, 620,
              "A: Enter URL     Y: Refresh     B: Back",
              WHITE);

    draw_text(35, 675,
              "XBOX 360 HOME BROWSER",
              GREEN);

    xsdk_gpu_resolve(address, width, height, BACKGROUND);
}

int main(void)
{
    xsdk_log_write("Xbox Browser starting");

    NTSTATUS result = xsdk_gpu_open();

    if (!NT_SUCCESS(result))
    {
        xsdk_log_write("GPU open failed: 0x%08X",
                       (unsigned)result);
        return 1;
    }

    uint32_t width = 0;
    uint32_t height = 0;

    result = xsdk_present_open(
        XSDK_DISPLAY_720P,
        2,
        1,
        &width,
        &height
    );

    if (!NT_SUCCESS(result))
    {
        xsdk_log_write("Display open failed: 0x%08X",
                       (unsigned)result);
        return 1;
    }

    result = xsdk_text_open();

    if (!NT_SUCCESS(result))
    {
        xsdk_log_write("Text initialization failed");
        return 1;
    }

    xsdk_log_write("Xbox Browser initialized");

    uint16_t previous_buttons = 0;

    while (true)
    {
        INPUT_STATE input = {0};

        uint32_t input_result =
            XamInputGetState(0, 0, &input);

        if (input_result != ERROR_SUCCESS)
            input = (INPUT_STATE){0};

        uint16_t buttons = input.gamepad.buttons;
        uint16_t pressed = buttons & ~previous_buttons;

        previous_buttons = buttons;

        if (pressed & INPUT_GAMEPAD_BACK)
            break;

        if (pressed & INPUT_GAMEPAD_A)
        {
            open_url_keyboard();
        }

        if (pressed & INPUT_GAMEPAD_Y)
        {
            snprintf(status_text, sizeof(status_text),
                     "Refresh requested");
        }

        if (pressed & INPUT_GAMEPAD_B)
        {
            snprintf(status_text, sizeof(status_text),
                     "Back selected");
        }

        uint32_t address;

        result = xsdk_present_begin(&address, 5000);

        if (!NT_SUCCESS(result))
            break;

        draw_browser(address, width, height);

        xsdk_present_end();
    }

    xsdk_text_close();
    xsdk_present_close();
    xsdk_gpu_close();

    xsdk_log_write("Xbox Browser closed");

    return 0;
}
