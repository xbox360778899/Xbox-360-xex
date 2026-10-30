#include <stdint.h>

/*
 * G4 Hybrid Browser / Cloud Gaming Dashboard
 * Target: Xbox 360 RGH/JTAG - Cedosworks XSDK
 *
 * IMPORTANT:
 * The framework must provide xsdk_text_draw() and register/call
 * draw_ui() through its own frame presenter.
 *
 * No traditional main() entry point is defined here.
 */

/* Framework graphics API */
extern void xsdk_text_draw(
    float x,
    float y,
    float scale,
    uint32_t color_hex,
    const char *string
);

/* Screen colors */
#define COLOR_GREEN  0x00FF55FF
#define COLOR_WHITE  0xFFFFFFFF
#define COLOR_BLACK  0x101820FF
#define COLOR_GRAY   0xAAB3BFFF
#define COLOR_BLUE   0x40A0FFFF
#define COLOR_YELLOW 0xFFFF55FF

/* Persistent application state */
static const char *g_current_url = "https://github.io";
static const char *g_active_title = "G4 Hybrid Browser";
static const char *g_html_state = "HTML ENGINE: READY";

static const char *g_browser_status = "BROWSER: INITIALIZED";
static const char *g_network_status = "NETWORK: WAITING";
static const char *g_stream_status = "CLOUD STREAM: STANDBY";
static const char *g_engine_status = "MEDIA ENGINE: NOT CONNECTED";

/* Dashboard navigation tabs */
static const char * const g_tabs[] = {
    "BROWSER",
    "SEARCH",
    "CLOUD",
    "EXIT"
};

#define G4_TAB_COUNT 4

/* Application selection state */
static unsigned int g_active_tab = 0;

/*
 * Main drawing callback.
 * The XSDK frame presenter should call this function.
 */
void draw_ui(uint32_t address, uint32_t width, uint32_t height)
{
    unsigned int i;
    float screen_width;
    float center_x;

    /* Explicit parameter handling for strict compiler flags */
    (void)address;
    (void)width;
    (void)height;

    screen_width = (float)width;
    center_x = screen_width * 0.5f;

    /* =========================================
       TOP GREEN HEADER
       ========================================= */

    xsdk_text_draw(
        35.0f, 25.0f, 2.0f,
        COLOR_GREEN,
        "G4"
    );

    xsdk_text_draw(
        105.0f, 31.0f, 1.2f,
        COLOR_WHITE,
        "HYBRID BROWSER"
    );

    xsdk_text_draw(
        screen_width - 190.0f, 35.0f, 0.8f,
        COLOR_GRAY,
        "XBOX 360"
    );

    xsdk_text_draw(
        35.0f, 65.0f, 0.8f,
        COLOR_GREEN,
        "============================================================"
    );

    /* =========================================
       ACTIVE ADDRESS BAR
       ========================================= */

    xsdk_text_draw(
        35.0f, 100.0f, 1.0f,
        COLOR_GRAY,
        "ADDRESS"
    );

    xsdk_text_draw(
        130.0f, 100.0f, 1.0f,
        COLOR_WHITE,
        g_current_url
    );

    xsdk_text_draw(
        35.0f, 130.0f, 0.8f,
        COLOR_BLUE,
        "------------------------------------------------------------"
    );

    /* =========================================
       APPLICATION NAVIGATION
       ========================================= */

    xsdk_text_draw(
        35.0f, 165.0f, 1.2f,
        COLOR_WHITE,
        "G4 NAVIGATION"
    );

    for (i = 0; i < G4_TAB_COUNT; i++)
    {
        if (i == g_active_tab)
        {
            xsdk_text_draw(
                45.0f + ((float)i * 175.0f),
                210.0f,
                1.0f,
                COLOR_GREEN,
                "[ ACTIVE ]"
            );
        }

        xsdk_text_draw(
            45.0f + ((float)i * 175.0f),
            235.0f,
            1.0f,
            (i == g_active_tab) ? COLOR_GREEN : COLOR_WHITE,
            g_tabs[i]
        );
    }

    /* =========================================
       MAIN CONTENT FRAME
       ========================================= */

    xsdk_text_draw(
        35.0f, 290.0f, 1.2f,
        COLOR_GREEN,
        "DASHBOARD"
    );

    xsdk_text_draw(
        35.0f, 330.0f, 1.0f,
        COLOR_WHITE,
        g_active_title
    );

    xsdk_text_draw(
        35.0f, 370.0f, 0.9f,
        COLOR_GRAY,
        "Welcome to G4 Cloud Gaming."
    );

    xsdk_text_draw(
        35.0f, 405.0f, 0.9f,
        COLOR_GRAY,
        "Browser and streaming modules are separate components."
    );

    /* =========================================
       SYSTEM STATUS FRAME
       ========================================= */

    xsdk_text_draw(
        35.0f, 465.0f, 1.1f,
        COLOR_GREEN,
        "SYSTEM STATUS"
    );

    xsdk_text_draw(
        35.0f, 500.0f, 0.85f,
        COLOR_WHITE,
        g_browser_status
    );

    xsdk_text_draw(
        35.0f, 530.0f, 0.85f,
        COLOR_YELLOW,
        g_network_status
    );

    xsdk_text_draw(
        35.0f, 560.0f, 0.85f,
        COLOR_GRAY,
        g_stream_status
    );

    xsdk_text_draw(
        35.0f, 590.0f, 0.85f,
        COLOR_GRAY,
        g_engine_status
    );

    /* =========================================
       HTML STATE
       ========================================= */

    xsdk_text_draw(
        center_x + 100.0f,
        500.0f,
        0.8f,
        COLOR_GREEN,
        g_html_state
    );

    /* =========================================
       CONTROLLER HELP
       ========================================= */

    xsdk_text_draw(
        35.0f, 650.0f, 0.8f,
        COLOR_GREEN,
        "A SELECT"
    );

    xsdk_text_draw(
        190.0f, 650.0f, 0.8f,
        COLOR_WHITE,
        "B BACK"
    );

    xsdk_text_draw(
        340.0f, 650.0f, 0.8f,
        COLOR_BLUE,
        "X SEARCH"
    );

    xsdk_text_draw(
        500.0f, 650.0f, 0.8f,
        COLOR_YELLOW,
        "Y MENU"
    );
}
