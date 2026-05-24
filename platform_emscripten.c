
#include <stdlib.h>

#include <emscripten.h>
#include <emscripten/html5.h>

#include "game.h"

static uint8_t *g_framebuffer;

EM_JS(void, js_init_canvas, (int width, int height), {
	var canvas = document.getElementById('canvas');
	canvas.width = width;
	canvas.height = height;
	canvas.style.imageRendering = 'pixelated';
	Module._ctx = canvas.getContext('2d', { alpha: false });
	Module._img = Module._ctx.createImageData(width, height);
});

EM_JS(void, js_blit, (int ptr, int w, int h), {
	Module._img.data.set(HEAPU8.subarray(ptr, ptr + w * h * 4));
	Module._ctx.putImageData(Module._img, 0, 0);
});

static F64 g_last_time = 0;
static F64 g_remaining_time = 0;
static F64 g_frame_index = 0;

static bool RequestAnimationiFrameCallback(F64 time, void *data) {
	if (g_last_time == 0) {
		g_last_time = time;
		return true;
	}

	F64 delta = time - g_last_time;
	g_remaining_time += delta;
	g_last_time = time;

	const F64 update_rate = 1000.0 / 60.0;

	if (g_remaining_time >= update_rate) {
		if (g_remaining_time > update_rate * 6) {
			g_remaining_time = update_rate;
		}
		do {
			GameFrame(g_frame_index);
			g_remaining_time -= update_rate;
			g_frame_index += 1;
		} while (g_remaining_time >= update_rate);

		// swap bgra to rgba
		U32 *framebuffer_color = (U32 *)g_framebuffer;
		for (U32 y = 0; y < FRAME_BUFFER_HEIGHT; ++y) {
			for (U32 x = 0; x < FRAME_BUFFER_WIDTH; ++x) {
				U32 bgra_color = framebuffer_color[y * FRAME_BUFFER_WIDTH + x];
				U32 r = (bgra_color >> 0u) & 0xFF;
				U32 g = (bgra_color >> 8u) & 0xFF;
				U32 b = (bgra_color >> 16u) & 0xFF;
				U32 rgba_color = 0xFF000000 | (r << 16) | (g << 8) | (b);
				framebuffer_color[y * FRAME_BUFFER_WIDTH + x] = rgba_color;
			}
		}
		js_blit((int)g_framebuffer, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
	}

	return true;
}

int main(void) {
	g_framebuffer = (uint8_t *)malloc(FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT * 4);

	js_init_canvas(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);

	GameInit(g_framebuffer, FRAME_BUFFER_WIDTH * 4);

	emscripten_request_animation_frame_loop(RequestAnimationiFrameCallback, NULL);
}
