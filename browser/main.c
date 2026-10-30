#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "triangle_shaders.h"
#include "xsdk/gpu.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "xsdk/present.h"
#include "xsdk/shader.h"
#include "xsdk/splash.h"

enum {
	SPLASH_MS = 2000,
	BUFFERS = 2,
	SWAP_INTERVAL = 1,
	BEGIN_TIMEOUT_MS = 5000,
	CLEAR_ARGB = 0xFF183060,
	VERTEX_FLOATS = 5,
	STRIDE_BYTES = 20,
	COLOR_OFFSET_BYTES = 8,
	MAX_VERTICES = 3072,
	MAX_BYTES = MAX_VERTICES * STRIDE_BYTES,
	CELL_PX = 12,
};

static const XsdkVertexElement layout[] = {
	{ .usage = XSDK_USAGE_POSITION, .format = XSDK_VERTEX_FLOAT2, .offset_bytes = 0 },
	{ .usage = XSDK_USAGE_COLOR, .format = XSDK_VERTEX_FLOAT3, .offset_bytes = COLOR_OFFSET_BYTES },
};

static float buf[MAX_VERTICES * VERTEX_FLOATS];
static uint32_t vcount;

static const uint8_t *glyph(char c)
{
	static const uint8_t blank[7] = { 0, 0, 0, 0, 0, 0, 0 };
	static const uint8_t gx[7] = { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 };
	static const uint8_t gb[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E };
	static const uint8_t go[7] = { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E };
	static const uint8_t gr[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 };
	static const uint8_t gw[7] = { 0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11 };
	static const uint8_t gs[7] = { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E };
	static const uint8_t ge[7] = { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F };
	switch (c) {
	case 'X': return gx;
	case 'B': return gb;
	case 'O': return go;
	case 'R': return gr;
	case 'W': return gw;
	case 'S': return gs;
	case 'E': return ge;
	default: return blank;
	}
}

static void add_vertex(float x, float y)
{
	float *v = &buf[vcount * VERTEX_FLOATS];
	v[0] = x;
	v[1] = y;
	v[2] = 1.0f;
	v[3] = 1.0f;
	v[4] = 1.0f;
	vcount++;
}

static void add_quad(float x0, float y0, float x1, float y1)
{
	if (vcount + 6 > MAX_VERTICES)
		return;
	add_vertex(x0, y0);
	add_vertex(x1, y0);
	add_vertex(x0, y1);
	add_vertex(x1, y0);
	add_vertex(x1, y1);
	add_vertex(x0, y1);
}

static void build_text(const char *text, uint32_t width, uint32_t height)
{
	const uint32_t chars = (uint32_t)strlen(text);
	const float startx = ((float)width - (float)(chars * 6 * CELL_PX)) / 2.0f;
	const float starty = ((float)height - (float)(7 * CELL_PX)) / 2.0f;
	for (uint32_t i = 0; i < chars; i++) {
		const uint8_t *g = glyph(text[i]);
		for (uint32_t r = 0; r < 7; r++) {
			for (uint32_t c = 0; c < 5; c++) {
				if (!((g[r] >> (4 - c)) & 1))
					continue;
				const float x = startx + (float)((i * 6 + c) * CELL_PX);
				const float y = starty + (float)(r * CELL_PX);
				add_quad(x / (float)width * 2.0f - 1.0f, 1.0f - y / (float)height * 2.0f,
					(x + CELL_PX) / (float)width * 2.0f - 1.0f, 1.0f - (y + CELL_PX) / (float)height * 2.0f);
			}
		}
	}
}

static void upload(volatile uint32_t *dst)
{
	for (uint32_t i = 0; i < vcount * VERTEX_FLOATS; i++) {
		uint32_t w;
		memcpy(&w, &buf[i], 4);
		dst[i] = w;
	}
	xsdk_gpu_invalidate(xsdk_gpu_address(dst), vcount * STRIDE_BYTES);
}

static void run(const XsdkShader *vertex, const XsdkShader *pixel, uint32_t vertices, uint32_t width, uint32_t height)
{
	for (;;) {
		INPUT_STATE input = { 0 };
		if (XamInputGetState(0, 0, &input) == ERROR_SUCCESS && input.gamepad.buttons & INPUT_GAMEPAD_BACK)
			return;
		uint32_t address;
		const NTSTATUS status = xsdk_present_begin(&address, BEGIN_TIMEOUT_MS);
		if (!NT_SUCCESS(status)) {
			xsdk_log_write("browser: xsdk_present_begin failed: 0x%08X", (unsigned)status);
			return;
		}
		xsdk_gpu_set_target(width, height);
		xsdk_gpu_set_shaders(vertex, pixel);
		xsdk_built_shader_set_literals(&triangle_vert);
		xsdk_built_shader_set_literals(&triangle_frag);
		xsdk_gpu_set_vertices(0, vertices, vcount * STRIDE_BYTES);
		xsdk_gpu_draw(XSDK_PRIMITIVE_TRIANGLE_LIST, vcount);
		xsdk_gpu_resolve(address, width, height, CLEAR_ARGB);
		xsdk_present_end();
	}
}

int main(void)
{
	xsdk_log_write("browser: press Back to end");
	const NTSTATUS splash = xsdk_splash_show(SPLASH_MS);
	if (!NT_SUCCESS(splash))
		xsdk_log_write("browser: xsdk_splash_show failed: 0x%08X", (unsigned)splash);
	NTSTATUS status = xsdk_gpu_open();
	if (!NT_SUCCESS(status)) {
		xsdk_log_write("browser: xsdk_gpu_open failed: 0x%08X", (unsigned)status);
		return 1;
	}
	XsdkShader vertex = { .microcode = NULL };
	XsdkShader pixel = { .microcode = NULL };
	status = xsdk_shader_load(&vertex, triangle_vert.container, triangle_vert.container_bytes);
	if (NT_SUCCESS(status))
		status = xsdk_shader_load(&pixel, triangle_frag.container, triangle_frag.container_bytes);
	if (NT_SUCCESS(status))
		status = xsdk_shader_set_vertex_layout(
			&vertex, layout, sizeof layout / sizeof layout[0], (const uint32_t[]){ STRIDE_BYTES });
	if (!NT_SUCCESS(status)) {
		xsdk_log_write("browser: loading the shaders failed: 0x%08X", (unsigned)status);
		return 1;
	}
	volatile uint32_t *const vertices = xsdk_gpu_alloc(MAX_BYTES);
	if (!vertices) {
		xsdk_log_write("browser: xsdk_gpu_alloc failed");
		return 1;
	}
	uint32_t width = 0;
	uint32_t height = 0;
	status = xsdk_present_open(XSDK_DISPLAY_720P, BUFFERS, SWAP_INTERVAL, &width, &height);
	if (!NT_SUCCESS(status)) {
		xsdk_log_write("browser: xsdk_present_open failed: 0x%08X", (unsigned)status);
		return 1;
	}
	build_text("XBOX BROWSER", width, height);
	upload(vertices);
	run(&vertex, &pixel, xsdk_gpu_address(vertices), width, height);
	xsdk_present_close();
	xsdk_gpu_free(vertices);
	xsdk_shader_free(&pixel);
	xsdk_shader_free(&vertex);
	status = xsdk_gpu_close();
	if (!NT_SUCCESS(status))
		xsdk_log_write("browser: xsdk_gpu_close failed: 0x%08X", (unsigned)status);
	xsdk_log_write("browser: done");
	return 0;
}
