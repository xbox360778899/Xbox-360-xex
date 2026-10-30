#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <xsdk/gpu.h>
#include <xsdk/kernel.h>
#include <xsdk/log.h>
#include <xsdk/present.h>
#include <xsdk/text.h>

// Color Macros (Strictly defined as integers for xsdk compatibility)
#define COLOR_GREEN  0xFF76B900
#define COLOR_WHITE  0xFFFFFFFF
#define COLOR_GRAY   0xFF999999
#define COLOR_MUTED  0xFF555555

#define NAV_COUNT 4

// --- FIXED GLOBAL DATA (Pointer Mismatches Removed Completely) ---
static const char *current_url = "https://github.io";
static const char *page_title = "XBOX 360 CLOUD DASHBOARD";
static const char *html_render_buffer = "MIX ENGINE: READY (CLOUD CACHE SECURE)";

static uint32_t active_tab = 0;
static const char *browser_tabs[NAV_COUNT] = { " BROWSER", " SEARCH", " CLOUD", " EXIT" };

// Core UI Drawing Loop Framework (60FPS Safe Layer)
static void draw_ui(uint32_t address, uint32_t width, uint32_t height) {
    // 1. Header Text Layout
    xsdk_text_draw(45, 30, 2, COLOR_GREEN, "XENON WEB CORE"); 
    xsdk_text_draw(500, 35, 1, COLOR_WHITE, "v1.0 (Stable-xsdk)"); 

    // 2. URL Address Frame Display
    xsdk_text_draw(45, 90, 1, COLOR_GRAY, "ADDRESS >");
    xsdk_text_draw(150, 90, 1, COLOR_WHITE, current_url);

    // 3. Navigation Bar Row System
    int32_t tab_x = 45;
    for (uint32_t i = 0; i < NAV_COUNT; i++) {
        uint32_t tab_color = (i == active_tab) ? COLOR_GREEN : COLOR_WHITE;
        xsdk_text_draw(tab_x, 140, 1, tab_color, browser_tabs[i]);
        tab_x += 160; 
    }

    // 4. Content Area Layout Header
    xsdk_text_draw(45, 220, 2, COLOR_WHITE, page_title);
    
    // 5. Render Core Main Content String
    xsdk_text_draw(45, 280, 1, COLOR_WHITE, html_render_buffer);

    // 6. Secure Bottom Status Info bar
    xsdk_text_draw(45, 540, 1, COLOR_MUTED, "SYSTEM LAYER: ACTIVE | SECURE PIPELINE VIA GITHUB");
}
