#include <stdint.h>

/* XSDK Graphics Function */
extern void xsdk_text_draw(
    float x,
    float y,
    float scale,
    uint32_t color,
    const char *text
);

/* G4 Colors */
#define G4_GREEN  0x00FF00FF
#define G4_WHITE  0xFFFFFFFF
#define G4_GRAY   0x999999FF

/* Static Application Information */
static const char *app_name = "G4";
static const char *app_version = "VERSION 1.0";
static const char *app_status = "SYSTEM READY";

static const char *menu_items[] = {
    "HOME",
    "BROWSER",
    "GAME LIBRARY",
    "SETTINGS"
};

#define MENU_COUNT 4

/* Active Menu */
static unsigned int selected_menu = 0;

/* Main UI Drawing Function */
void draw_ui(uint32_t address, uint32_t width, uint32_t height)
{
    unsigned int i;

    /* Avoid unused parameter warnings */
    (void)address;
    (void)width;
    (void)height;

    /* G4 Logo */
    xsdk_text_draw(
        40.0f, 35.0f, 2.0f,
        G4_GREEN,
        app_name
    );

    /* Version */
    xsdk_text_draw(
        110.0f, 48.0f, 0.8f,
        G4_GRAY,
        app_version
    );

    /* Header */
    xsdk_text_draw(
        40.0f, 90.0f, 1.0f,
        G4_WHITE,
        "G4 CLOUD GAMING"
    );

    /* Navigation Menu */
    for (i = 0; i < MENU_COUNT; i++)
    {
        uint32_t color;

        color = (i == selected_menu)
            ? G4_GREEN
            : G4_WHITE;

        xsdk_text_draw(
            45.0f,
            160.0f + ((float)i * 45.0f),
            1.0f,
            color,
            menu_items[i]
        );
    }

    /* Welcome Message */
    xsdk_text_draw(
        350.0f, 180.0f, 1.2f,
        G4_WHITE,
        "WELCOME TO G4"
    );

    xsdk_text_draw(
        350.0f, 220.0f, 0.9f,
        G4_GRAY,
        "YOUR CLOUD GAMING DASHBOARD"
    );

    /* System Status */
    xsdk_text_draw(
        40.0f, 400.0f, 0.9f,
        G4_GREEN,
        app_status
    );

    /* Footer */
    xsdk_text_draw(
        40.0f, 650.0f, 0.8f,
        G4_GRAY,
        "G4 PROJECT - XBOX 360"
    );

    xsdk_text_draw(
        450.0f, 650.0f, 0.8f,
        G4_WHITE,
        "A SELECT   B BACK"
    );
}
