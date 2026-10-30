#include <stdint.h>

#include "xsdk/gpu.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "xsdk/splash.h"

int main(void)
{
    xsdk_log_write("browser: starting stub");

    const NTSTATUS splash = xsdk_splash_show(2000);
    if (!NT_SUCCESS(splash))
        xsdk_log_write("browser: xsdk_splash_show failed: 0x%08X", (unsigned)splash);

    NTSTATUS status = xsdk_gpu_open();
    if (!NT_SUCCESS(status)) {
        xsdk_log_write("browser: xsdk_gpu_open failed: 0x%08X", (unsigned)status);
        return 1;
    }

    xsdk_log_write("browser: stub browser built successfully");
    return 0;
}
