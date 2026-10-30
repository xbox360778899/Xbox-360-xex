#include <stdbool.h>
#include <stdint.h>

#include "xsdk/gpu.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "xsdk/present.h"

enum {
    DISPLAY_BUFFERS = 2,
    SWAP_INTERVAL = 2,
    BEGIN_TIMEOUT_MS = 5000,
    BACKGROUND_COLOR = 0xFF101820
};

static void run_browser(uint32_t width, uint32_t height)
{
    (void)width;
    (void)height;

    while (true)
    {
        INPUT_STATE input = {0};

        if (XamInputGetState(0, 0, &input) == ERROR_SUCCESS)
        {
            if (input.gamepad.buttons & INPUT_GAMEPAD_BACK)
                break;
        }

        uint32_t address = 0;

        NTSTATUS status =
            xsdk_present_begin(&address, BEGIN_TIMEOUT_MS);

        if (!NT_SUCCESS(status))
        {
            xsdk_log_write("Browser: Display error 0x%08X",
                           (unsigned)status);
            break;
        }

        xsdk_gpu_resolve(
            address,
            width,
            height,
            BACKGROUND_COLOR
        );

        xsdk_present_end();
    }
}

int main(void)
{
    xsdk_log_write("Browser: Starting lightweight mode");

    NTSTATUS status = xsdk_gpu_open();

    if (!NT_SUCCESS(status))
    {
        xsdk_log_write("Browser: GPU initialization failed");
        return 1;
    }

    uint32_t width = 0;
    uint32_t height = 0;

    status = xsdk_present_open(
        XSDK_DISPLAY_720P,
        DISPLAY_BUFFERS,
        SWAP_INTERVAL,
        &width,
        &height
    );

    if (!NT_SUCCESS(status))
    {
        xsdk_log_write("Browser: Display initialization failed");
        xsdk_gpu_close();
        return 1;
    }

    xsdk_log_write(
        "Browser: Running at %ux%u",
        (unsigned)width,
        (unsigned)height
    );

    run_browser(width, height);

    xsdk_present_close();

    status = xsdk_gpu_close();

    if (!NT_SUCCESS(status))
        xsdk_log_write("Browser: GPU shutdown warning");

    xsdk_log_write("Browser: Closed successfully");

    return 0;
}
