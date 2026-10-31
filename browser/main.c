
#pragma GCC diagnostic ignored "-Wsign-compare"#include <stdbool.h>
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
	FENCE_TIMEOUT_MS = 1000,
	CLEAR_ARGB = 0xFF183060,
	VERTEX_FLOATS = 5,
	STRIDE_BYTES = 20,
	COLOR_OFFSET_BYTES = 8,
	MAX_VERTICES = 49152,
	MIN_VERTICES = 3072,
	POOL_SIZE = 32768,
	MAX_LINES = 2048,
	MAX_LINKS = 64,
	URL_LEN = 96,
	WORD_MAX = 64,
	MARGIN_PX = 40,
	BODY_SCALE = 3,
	H1_SCALE = 5,
	H2_SCALE = 4,
	STATUS_SCALE = 2,
};

enum { C_TEXT, C_H1, C_LINK, C_H2, C_BOLD, C_SEL, C_DIM };

static const float palette[7][3] = {
	{ 1.0f, 1.0f, 1.0f }, { 1.0f, 0.9f, 0.2f }, { 0.4f, 0.9f, 1.0f }, { 0.5f, 1.0f, 0.5f },
	{ 1.0f, 0.6f, 0.3f }, { 1.0f, 0.3f, 0.8f }, { 0.7f, 0.7f, 0.85f },
};

static const XsdkVertexElement layout[] = {
	{ .usage = XSDK_USAGE_POSITION, .format = XSDK_VERTEX_FLOAT2, .offset_bytes = 0 },
	{ .usage = XSDK_USAGE_COLOR, .format = XSDK_VERTEX_FLOAT3, .offset_bytes = COLOR_OFFSET_BYTES },
};

/* 5x7 font, ASCII 32..126. Each glyph is 5 columns; bit 0 of a column is the top row. */
static const uint8_t font5x7[95][5] = {
	{ 0x00, 0x00, 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x5F, 0x00, 0x00 }, { 0x00, 0x07, 0x00, 0x07, 0x00 },
	{ 0x14, 0x7F, 0x14, 0x7F, 0x14 }, { 0x24, 0x2A, 0x7F, 0x2A, 0x12 }, { 0x23, 0x13, 0x08, 0x64, 0x62 },
	{ 0x36, 0x49, 0x55, 0x22, 0x50 }, { 0x00, 0x05, 0x03, 0x00, 0x00 }, { 0x00, 0x1C, 0x22, 0x41, 0x00 },
	{ 0x00, 0x41, 0x22, 0x1C, 0x00 }, { 0x08, 0x2A, 0x1C, 0x2A, 0x08 }, { 0x08, 0x08, 0x3E, 0x08, 0x08 },
	{ 0x00, 0x50, 0x30, 0x00, 0x00 }, { 0x08, 0x08, 0x08, 0x08, 0x08 }, { 0x00, 0x60, 0x60, 0x00, 0x00 },
	{ 0x20, 0x10, 0x08, 0x04, 0x02 }, { 0x3E, 0x51, 0x49, 0x45, 0x3E }, { 0x00, 0x42, 0x7F, 0x40, 0x00 },
	{ 0x42, 0x61, 0x51, 0x49, 0x46 }, { 0x21, 0x41, 0x45, 0x4B, 0x31 }, { 0x18, 0x14, 0x12, 0x7F, 0x10 },
	{ 0x27, 0x45, 0x45, 0x45, 0x39 }, { 0x3C, 0x4A, 0x49, 0x49, 0x30 }, { 0x01, 0x71, 0x09, 0x05, 0x03 },
	{ 0x36, 0x49, 0x49, 0x49, 0x36 }, { 0x06, 0x49, 0x49, 0x29, 0x1E }, { 0x00, 0x36, 0x36, 0x00, 0x00 },
	{ 0x00, 0x56, 0x36, 0x00, 0x00 }, { 0x00, 0x08, 0x14, 0x22, 0x41 }, { 0x14, 0x14, 0x14, 0x14, 0x14 },
	{ 0x41, 0x22, 0x14, 0x08, 0x00 }, { 0x02, 0x01, 0x51, 0x09, 0x06 }, { 0x32, 0x49, 0x79, 0x41, 0x3E },
	{ 0x7E, 0x11, 0x11, 0x11, 0x7E }, { 0x7F, 0x49, 0x49, 0x49, 0x36 }, { 0x3E, 0x41, 0x41, 0x41, 0x22 },
	{ 0x7F, 0x41, 0x41, 0x22, 0x1C }, { 0x7F, 0x49, 0x49, 0x49, 0x41 }, { 0x7F, 0x09, 0x09, 0x09, 0x01 },
	{ 0x3E, 0x41, 0x49, 0x49, 0x7A }, { 0x7F, 0x08, 0x08, 0x08, 0x7F }, { 0x00, 0x41, 0x7F, 0x41, 0x00 },
	{ 0x20, 0x40, 0x41, 0x3F, 0x01 }, { 0x7F, 0x08, 0x14, 0x22, 0x41 }, { 0x7F, 0x40, 0x40, 0x40, 0x40 },
	{ 0x7F, 0x02, 0x0C, 0x02, 0x7F }, { 0x7F, 0x04, 0x08, 0x10, 0x7F }, { 0x3E, 0x41, 0x41, 0x41, 0x3E },
	{ 0x7F, 0x09, 0x09, 0x09, 0x06 }, { 0x3E, 0x41, 0x51, 0x21, 0x5E }, { 0x7F, 0x09, 0x19, 0x29, 0x46 },
	{ 0x46, 0x49, 0x49, 0x49, 0x31 }, { 0x01, 0x01, 0x7F, 0x01, 0x01 }, { 0x3F, 0x40, 0x40, 0x40, 0x3F },
	{ 0x1F, 0x20, 0x40, 0x20, 0x1F }, { 0x3F, 0x40, 0x38, 0x40, 0x3F }, { 0x63, 0x14, 0x08, 0x14, 0x63 },
	{ 0x07, 0x08, 0x70, 0x08, 0x07 }, { 0x61, 0x51, 0x49, 0x45, 0x43 }, { 0x00, 0x7F, 0x41, 0x41, 0x00 },
	{ 0x02, 0x04, 0x08, 0x10, 0x20 }, { 0x00, 0x41, 0x41, 0x7F, 0x00 }, { 0x04, 0x02, 0x01, 0x02, 0x04 },
	{ 0x40, 0x40, 0x40, 0x40, 0x40 }, { 0x00, 0x01, 0x02, 0x04, 0x00 }, { 0x20, 0x54, 0x54, 0x54, 0x78 },
	{ 0x7F, 0x48, 0x44, 0x44, 0x38 }, { 0x38, 0x44, 0x44, 0x44, 0x20 }, { 0x38, 0x44, 0x44, 0x48, 0x7F },
	{ 0x38, 0x54, 0x54, 0x54, 0x18 }, { 0x08, 0x7E, 0x09, 0x01, 0x02 }, { 0x0C, 0x52, 0x52, 0x52, 0x3E },
	{ 0x7F, 0x08, 0x04, 0x04, 0x78 }, { 0x00, 0x44, 0x7D, 0x40, 0x00 }, { 0x20, 0x40, 0x44, 0x3D, 0x00 },
	{ 0x7F, 0x10, 0x28, 0x44, 0x00 }, { 0x00, 0x41, 0x7F, 0x40, 0x00 }, { 0x7C, 0x04, 0x18, 0x04, 0x78 },
	{ 0x7C, 0x08, 0x04, 0x04, 0x78 }, { 0x38, 0x44, 0x44, 0x44, 0x38 }, { 0x7C, 0x14, 0x14, 0x14, 0x08 },
	{ 0x08, 0x14, 0x14, 0x18, 0x7C }, { 0x7C, 0x08, 0x04, 0x04, 0x08 }, { 0x48, 0x54, 0x54, 0x54, 0x20 },
	{ 0x04, 0x3F, 0x44, 0x40, 0x20 }, { 0x3C, 0x40, 0x40, 0x20, 0x7C }, { 0x1C, 0x20, 0x40, 0x20, 0x1C },
	{ 0x3C, 0x40, 0x30, 0x40, 0x3C }, { 0x44, 0x28, 0x10, 0x28, 0x44 }, { 0x0C, 0x50, 0x50, 0x50, 0x3C },
	{ 0x44, 0x64, 0x54, 0x4C, 0x44 }, { 0x00, 0x08, 0x36, 0x41, 0x00 }, { 0x00, 0x00, 0x7F, 0x00, 0x00 },
	{ 0x00, 0x41, 0x36, 0x08, 0x00 }, { 0x10, 0x08, 0x08, 0x10, 0x08 },
};

/* Built-in pages. Links "page:N" open another built-in page. */
static const char *const pages[] = {
	"<html><head><title>Home</title><style>h1{color:red}</style></head><body>"
	"<h1>Xbox Browser</h1>"
	"<p>Welcome. This is a small HTML viewer running on the Xbox 360.</p>"
	"<h2>Controls</h2>"
	"<ul><li>Right stick up/down: scroll</li>"
	"<li>Left stick left/right: choose a link</li>"
	"<li>A: open the link</li>"
	"<li>Left trigger: go back</li>"
	"<li>Back button: exit</li></ul>"
	"<p>Open <a href=\"page:1\">the about page</a> or <a href=\"page:2\">the status page</a>.</p>"
	"<script>alert(1)</script>"
	"</body></html>",

	"<html><body><h1>About</h1>"
	"<p>Every letter on this screen is drawn by the GPU as small squares. "
	"The page is plain HTML: headings, paragraphs, lists, <b>bold text</b> and links.</p>"
	"<p>CSS and JavaScript are skipped for now. Style and script blocks are ignored.</p>"
	"<p>This page is long on purpose, so you can try scrolling with the right stick.</p>"
	"<h2>Line one</h2><p>The quick brown fox jumps over the lazy dog 0123456789.</p>"
	"<h2>Line two</h2><p>The quick brown fox jumps over the lazy dog 0123456789.</p>"
	"<h2>Line three</h2><p>The quick brown fox jumps over the lazy dog 0123456789.</p>"
	"<h2>Line four</h2><p>The quick brown fox jumps over the lazy dog 0123456789.</p>"
	"<p><a href=\"page:0\">Back to home</a></p></body></html>",

	"<html><body><h1>Status</h1>"
	"<ul><li>Controller: working</li><li>HTML text: working</li>"
	"<li>Internet: not added yet</li><li>CSS: not added yet</li>"
	"<li>JavaScript: not added yet</li></ul>"
	"<p>Try this link: <a href=\"http://example.com\">example.com</a></p>"
	"<p><a href=\"page:0\">Back to home</a></p></body></html>",
};
enum { PAGE_COUNT = 3 };

typedef struct {
	uint32_t start;
	uint32_t len;
	uint8_t scale;
} Line;

static float buf[MAX_VERTICES * VERTEX_FLOATS];
static uint32_t vcount;
static uint32_t vcap;
static uint32_t screen_w;
static uint32_t screen_h;

static char pool[POOL_SIZE];
static uint8_t pcol[POOL_SIZE];
static int8_t plink[POOL_SIZE];
static uint32_t pool_n;
static Line lines[MAX_LINES];
static uint32_t line_n;
static char link_url[MAX_LINKS][URL_LEN];
static int link_n;

static bool doc_full;
static bool need_new;
static int pending_blank;
static int cur_scale;
static int base_col;
static int bold;
static int in_link;
static char word[WORD_MAX];
static uint32_t wl;
static int wcol;
static int wlink;

static uint32_t top_line;
static float scroll_acc;
static int sel;
static const char *message;
static bool dirty;
static int cur_page;
static int prev_page = -1;

/* ---------- drawing ---------- */

static void add_vertex(float x, float y, int col)
{
	float *v = &buf[vcount * VERTEX_FLOATS];
	v[0] = x;
	v[1] = y;
	v[2] = palette[col][0];
	v[3] = palette[col][1];
	v[4] = palette[col][2];
	vcount++;
}

static void add_quad_px(float px, float py, float pw, float ph, int col)
{
	if (vcount + 6 > vcap || vcount + 6 > MAX_VERTICES)
		return;
	const float x0 = px / (float)screen_w * 2.0f - 1.0f;
	const float x1 = (px + pw) / (float)screen_w * 2.0f - 1.0f;
	const float y0 = 1.0f - py / (float)screen_h * 2.0f;
	const float y1 = 1.0f - (py + ph) / (float)screen_h * 2.0f;
	add_vertex(x0, y0, col);
	add_vertex(x1, y0, col);
	add_vertex(x0, y1, col);
	add_vertex(x1, y0, col);
	add_vertex(x1, y1, col);
	add_vertex(x0, y1, col);
}

static void draw_glyph(unsigned char ch, float x, float y, int scale, int col)
{
	if (ch < 32 || ch > 126)
		ch = '?';
	const uint8_t *g = font5x7[ch - 32];
	for (int c = 0; c < 5; c++) {
		const uint8_t b = g[c];
		int r = 0;
		while (r < 7) {
			if ((b >> r) & 1) {
				const int r0 = r;
				while (r < 7 && ((b >> r) & 1))
					r++;
				add_quad_px(x + (float)(c * scale), y + (float)(r0 * scale), (float)scale, (float)((r - r0) * scale),
					col);
			} else {
				r++;
			}
		}
	}
}

static void draw_text(const char *s, float x, float y, int scale, int col)
{
	for (; *s; s++) {
		if (*s != ' ')
			draw_glyph((unsigned char)*s, x, y, scale, col);
		x += (float)(6 * scale);
	}
}

static float view_bottom(void)
{
	return (float)screen_h - (float)MARGIN_PX - 36.0f;
}

static void build_vertices(void)
{
	vcount = 0;
	float y = (float)MARGIN_PX;
	const float bottom = view_bottom();
	for (uint32_t i = top_line; i < line_n; i++) {
		const Line *L = &lines[i];
		const float h = (float)(9 * L->scale);
		if (y + h > bottom)
			break;
		float x = (float)MARGIN_PX;
		for (uint32_t j = 0; j < L->len; j++) {
			const uint32_t k = L->start + j;
			int col = pcol[k];
			if (plink[k] >= 0 && plink[k] == sel)
				col = C_SEL;
			if (pool[k] != ' ')
				draw_glyph((unsigned char)pool[k], x, y, L->scale, col);
			x += (float)(6 * L->scale);
		}
		y += h;
	}
	const float sy = (float)screen_h - (float)MARGIN_PX - (float)(8 * STATUS_SCALE);
	if (message)
		draw_text(message, (float)MARGIN_PX, sy, STATUS_SCALE, C_BOLD);
	else if (sel >= 0)
		draw_text(link_url[sel], (float)MARGIN_PX, sy, STATUS_SCALE, C_DIM);
}

static void upload(volatile uint32_t *dst)
{
	for (uint32_t i = 0; i < vcount * VERTEX_FLOATS; i++) {
		uint32_t w;
		memcpy(&w, &buf[i], 4);
		dst[i] = w;
	}
	if (vcount > 0)
		xsdk_gpu_invalidate(xsdk_gpu_address(dst), vcount * STRIDE_BYTES);
}

/* ---------- HTML layout ---------- */

static char lc(char c)
{
	return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
}

static bool is_alnum(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

static bool ci_match(const char *p, const char *name)
{
	while (*name) {
		if (lc(*p) != *name)
			return false;
		p++;
		name++;
	}
	return true;
}

static int cur_col(void)
{
	if (in_link >= 0)
		return C_LINK;
	if (bold)
		return C_BOLD;
	return base_col;
}

static void open_line(void)
{
	if (line_n > 0 && !need_new)
		return;
	if (pending_blank && line_n > 0 && line_n < MAX_LINES) {
		lines[line_n].start = pool_n;
		lines[line_n].len = 0;
		lines[line_n].scale = BODY_SCALE;
		line_n++;
	}
	pending_blank = 0;
	if (line_n >= MAX_LINES) {
		doc_full = true;
		return;
	}
	lines[line_n].start = pool_n;
	lines[line_n].len = 0;
	lines[line_n].scale = (uint8_t)cur_scale;
	line_n++;
	need_new = false;
}

static void put_char(char c, int col, int link)
{
	if (pool_n >= POOL_SIZE) {
		doc_full = true;
		return;
	}
	pool[pool_n] = c;
	pcol[pool_n] = (uint8_t)col;
	plink[pool_n] = (int8_t)link;
	pool_n++;
	lines[line_n - 1].len++;
}

static void add_word(const char *w, uint32_t n, int col, int link)
{
	if (n == 0 || doc_full)
		return;
	open_line();
	if (doc_full)
		return;
	Line *L = &lines[line_n - 1];
	uint32_t cols = (screen_w - 2 * MARGIN_PX) / (uint32_t)(6 * L->scale);
	if (L->len > 0 && L->len + 1 + n > cols) {
		need_new = true;
		open_line();
		if (doc_full)
			return;
		L = &lines[line_n - 1];
		cols = (screen_w - 2 * MARGIN_PX) / (uint32_t)(6 * L->scale);
	}
	if (L->len > 0)
		put_char(' ', C_TEXT, link);
	for (uint32_t i = 0; i < n && !doc_full; i++) {
		if (L->len >= cols) {
			need_new = true;
			open_line();
			if (doc_full)
				return;
			L = &lines[line_n - 1];
		}
		put_char(w[i], col, link);
	}
}

static void flush_word(void)
{
	add_word(word, wl, wcol, wlink);
	wl = 0;
}

static void word_add(char ch)
{
	if (wl >= WORD_MAX - 1)
		flush_word();
	if (wl == 0) {
		wcol = cur_col();
		wlink = in_link;
	}
	word[wl++] = ch;
}

static const char *skip_block(const char *p, const char *end_name)
{
	for (; *p; p++) {
		if (p[0] == '<' && p[1] == '/' && ci_match(p + 2, end_name)) {
			const char *g = strchr(p, '>');
			return g ? g + 1 : p + strlen(p);
		}
	}
	return p;
}

static void get_href(const char *s, const char *e, char *out)
{
	out[0] = 0;
	for (const char *p = s; p + 4 < e; p++) {
		if (ci_match(p, "href")) {
			p += 4;
			while (p < e && (*p == ' ' || *p == '='))
				p++;
			char q = 0;
			if (p < e && (*p == '"' || *p == '\''))
				q = *p++;
			uint32_t n = 0;
			while (p < e && n < URL_LEN - 1 && ((q && *p != q) || (!q && *p != ' ')))
				out[n++] = *p++;
			out[n] = 0;
			return;
		}
	}
}

static void parse_html(const char *html)
{
	pool_n = 0;
	line_n = 0;
	link_n = 0;
	doc_full = false;
	need_new = true;
	pending_blank = 0;
	cur_scale = BODY_SCALE;
	base_col = C_TEXT;
	bold = 0;
	in_link = -1;
	wl = 0;

	const char *p = html;
	while (*p && !doc_full) {
		const char c = *p;
		if (c == '<') {
			if (p[1] == '!' && p[2] == '-' && p[3] == '-') {
				const char *e = strstr(p + 4, "-->");
				p = e ? e + 3 : p + strlen(p);
				continue;
			}
			const char *te = strchr(p, '>');
			if (!te)
				break;
			flush_word();
			const char *q = p + 1;
			bool closing = false;
			if (*q == '/') {
				closing = true;
				q++;
			}
			char name[8];
			int ni = 0;
			while (q < te && ni < 7 && is_alnum(*q))
				name[ni++] = lc(*q++);
			name[ni] = 0;

			if (!strcmp(name, "script") || !strcmp(name, "style") || !strcmp(name, "title")) {
				if (!closing) {
					p = skip_block(te + 1, name);
					continue;
				}
			} else if (!strcmp(name, "h1") || !strcmp(name, "h2") || !strcmp(name, "h3")) {
				need_new = true;
				pending_blank = 1;
				if (closing) {
					cur_scale = BODY_SCALE;
					base_col = C_TEXT;
				} else {
					cur_scale = name[1] == '1' ? H1_SCALE : H2_SCALE;
					base_col = name[1] == '1' ? C_H1 : C_H2;
				}
			} else if (!strcmp(name, "p") || !strcmp(name, "ul") || !strcmp(name, "ol") || !strcmp(name, "table") ||
				!strcmp(name, "blockquote")) {
				need_new = true;
				pending_blank = 1;
			} else if (!strcmp(name, "div") || !strcmp(name, "tr") || !strcmp(name, "body") ||
				!strcmp(name, "html") || !strcmp(name, "head") || !strcmp(name, "br") || !strcmp(name, "hr")) {
				need_new = true;
			} else if (!strcmp(name, "li")) {
				need_new = true;
				if (!closing)
					add_word("*", 1, C_H2, -1);
			} else if (!strcmp(name, "a")) {
				if (!closing) {
					if (link_n < MAX_LINKS) {
						get_href(q, te, link_url[link_n]);
						in_link = link_n;
						link_n++;
					}
				} else {
					in_link = -1;
				}
			} else if (!strcmp(name, "b") || !strcmp(name, "strong")) {
				bold = closing ? 0 : 1;
			}
			p = te + 1;
			continue;
		}
		if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
			flush_word();
			p++;
			continue;
		}
		if (c == '&') {
			const char *semi = strchr(p, ';');
			char out = 0;
			int skip = 0;
			if (semi && semi - p <= 6) {
				if (!strncmp(p, "&amp;", 5)) {
					out = '&';
					skip = 5;
				} else if (!strncmp(p, "&lt;", 4)) {
					out = '<';
					skip = 4;
				} else if (!strncmp(p, "&gt;", 4)) {
					out = '>';
					skip = 4;
				} else if (!strncmp(p, "&quot;", 6)) {
					out = '"';
					skip = 6;
				} else if (!strncmp(p, "&nbsp;", 6)) {
					out = ' ';
					skip = 6;
				}
			}
			if (skip) {
				if (out == ' ')
					flush_word();
				else
					word_add(out);
				p += skip;
				continue;
			}
		}
		word_add(c);
		p++;
	}
	flush_word();
}

/* ---------- navigation ---------- */

static uint32_t link_line(int id)
{
	for (uint32_t i = 0; i < line_n; i++)
		for (uint32_t j = 0; j < lines[i].len; j++)
			if (plink[lines[i].start + j] == id)
				return i;
	return 0;
}

static bool fits(uint32_t li)
{
	float y = (float)MARGIN_PX;
	for (uint32_t i = top_line; i <= li && i < line_n; i++)
		y += (float)(9 * lines[i].scale);
	return y <= view_bottom();
}

static void ensure_visible(void)
{
	if (sel < 0)
		return;
	const uint32_t li = link_line(sel);
	if (li < top_line)
		top_line = li;
	while (top_line < li && !fits(li))
		top_line++;
}

static void load_page(int n)
{
	cur_page = n;
	parse_html(pages[n]);
	top_line = 0;
	scroll_acc = 0.0f;
	sel = link_n > 0 ? 0 : -1;
	message = NULL;
	dirty = true;
}

static void open_link(void)
{
	if (sel < 0)
		return;
	const char *url = link_url[sel];
	if (!strncmp(url, "page:", 5) && url[5] >= '0' && url[5] < '0' + PAGE_COUNT) {
		prev_page = cur_page;
		load_page(url[5] - '0');
	} else if (!strncmp(url, "http", 4)) {
		message = "INTERNET NOT ADDED YET";
		dirty = true;
	} else {
		message = "UNKNOWN LINK";
		dirty = true;
	}
}

static int iabs(int v)
{
	return v < 0 ? -v : v;
}

static void run(const XsdkShader *vertex, const XsdkShader *pixel, volatile uint32_t *vptr, uint32_t width,
	uint32_t height)
{
	bool armed = true;
	bool a_prev = false;
	bool lt_prev = false;
	uint32_t frames = 0;
	const uint32_t vaddr = xsdk_gpu_address(vptr);
	for (;;) {
		INPUT_STATE input = { 0 };
		if (XamInputGetState(0, 0, &input) != ERROR_SUCCESS)
			input = (INPUT_STATE){ 0 };
		if (input.gamepad.buttons & INPUT_GAMEPAD_BACK)
			return;

		const int ry = input.gamepad.thumb_ry;
		if (iabs(ry) > 8000)
			scroll_acc += -(float)ry / 32768.0f * 0.4f;
		while (scroll_acc >= 1.0f) {
			if (top_line + 1 < line_n)
				top_line++;
			scroll_acc -= 1.0f;
			dirty = true;
		}
		while (scroll_acc <= -1.0f) {
			if (top_line > 0)
				top_line--;
			scroll_acc += 1.0f;
			dirty = true;
		}

		const int lx = input.gamepad.thumb_lx;
		if (iabs(lx) < 8000)
			armed = true;
		if (armed && lx > 20000 && sel + 1 < link_n) {
			sel++;
			armed = false;
			message = NULL;
			ensure_visible();
			dirty = true;
		} else if (armed && lx < -20000 && sel > 0) {
			sel--;
			armed = false;
			message = NULL;
			ensure_visible();
			dirty = true;
		}

		const bool a = (input.gamepad.buttons & INPUT_GAMEPAD_A) != 0;
		if (a && !a_prev)
			open_link();
		a_prev = a;

		const bool lt = input.gamepad.left_trigger > 200;
		if (lt && !lt_prev && prev_page >= 0) {
			const int back = prev_page;
			prev_page = -1;
			load_page(back);
		}
		lt_prev = lt;

		if (dirty) {
			if (frames > 0) {
				const NTSTATUS ws = xsdk_gpu_wait(xsdk_gpu_fence(), FENCE_TIMEOUT_MS);
				if (!NT_SUCCESS(ws))
					xsdk_log_write("browser: xsdk_gpu_wait failed: 0x%08X", (unsigned)ws);
			}
			build_vertices();
			upload(vptr);
			dirty = false;
		}

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
		if (vcount > 0) {
			xsdk_gpu_set_vertices(0, vaddr, vcount * STRIDE_BYTES);
			xsdk_gpu_draw(XSDK_PRIMITIVE_TRIANGLE_LIST, vcount);
		}
		xsdk_gpu_resolve(address, width, height, CLEAR_ARGB);
		xsdk_present_end();
		frames++;
	}
}

int main(void)
{
	xsdk_log_write("browser: right stick scrolls, left stick picks a link, A opens, Back ends");
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
	uint32_t cap = MAX_VERTICES;
	volatile uint32_t *vertices = NULL;
	while (cap >= MIN_VERTICES) {
		vertices = xsdk_gpu_alloc(cap * STRIDE_BYTES);
		if (vertices)
			break;
		cap /= 2;
	}
	if (!vertices) {
		xsdk_log_write("browser: xsdk_gpu_alloc failed");
		return 1;
	}
	vcap = cap;
	uint32_t width = 0;
	uint32_t height = 0;
	status = xsdk_present_open(XSDK_DISPLAY_720P, BUFFERS, SWAP_INTERVAL, &width, &height);
	if (!NT_SUCCESS(status)) {
		xsdk_log_write("browser: xsdk_present_open failed: 0x%08X", (unsigned)status);
		return 1;
	}
	screen_w = width;
	screen_h = height;
	load_page(0);
	run(&vertex, &pixel, vertices, width, height);
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
