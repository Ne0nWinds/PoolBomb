
#include <stdlib.h>

#include <emscripten.h>
#include <emscripten/html5.h>

#include "game.h"

static uint8_t *g_framebuffer_bgra;
static uint8_t *g_framebuffer_rgba;

EM_JS(void, js_init_canvas, (int width, int height), {
	var canvas = document.getElementById('canvas');
	canvas.width = width;
	canvas.height = height;
	canvas.style.imageRendering = 'pixelated';
	Module._ctx = canvas.getContext('2d');
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
		js_blit((int)g_framebuffer_bgra, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
	}

	return true;
}

int main(void) {
	g_framebuffer_bgra = (uint8_t *)malloc(FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT * 4);
	g_framebuffer_rgba = (uint8_t *)malloc(FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT * 4);

	js_init_canvas(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);

	GameInit(g_framebuffer_bgra, FRAME_BUFFER_WIDTH * 4);

	emscripten_request_animation_frame_loop(RequestAnimationiFrameCallback, NULL);
}
