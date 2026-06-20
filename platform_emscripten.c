
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/console.h>

#include "game.h"

static U32 *g_framebuffer;

EM_JS(void, js_init_canvas, (int width, int height), {
	var canvas = document.getElementById('canvas');
	canvas.width = width;
	canvas.height = height;
	canvas.style.imageRendering = 'pixelated';
	Module._ctx = canvas.getContext('2d', { alpha: false });
	Module._img = Module._ctx.createImageData(width, height);
});

EM_JS(void, js_blit, (void *ptr, int w, int h), {
	Module._img.data.set(HEAPU8.subarray(ptr, ptr + w * h * 4));
	Module._ctx.putImageData(Module._img, 0, 0);
});

static F64 g_last_time = 0;
static F64 g_remaining_time = 0;
static U64 g_frame_index = 0;

static U32 g_keyboard_state = 0;
static U32 g_gamepad_state = 0;
static U32 g_previous_button_state = 0;

static EM_BOOL OnKeydown(int t, const EmscriptenKeyboardEvent *e, void *_) {
	const char *code = e->code;

	if (!strcmp(code, "ArrowUp") || !strcmp(code, "KeyW")) {
		g_keyboard_state |= BUTTON_UP;
		return EM_TRUE;
	}

	if (!strcmp(code, "ArrowDown") || !strcmp(code, "KeyS")) {
		g_keyboard_state |= BUTTON_DOWN;
		return EM_TRUE;
	}

	if (!strcmp(code, "ArrowLeft") || !strcmp(code, "KeyA")) {
		g_keyboard_state |= BUTTON_LEFT;
		return EM_TRUE;
	}

	if (!strcmp(code, "ArrowRight") || !strcmp(code, "KeyD")) {
		g_keyboard_state |= BUTTON_RIGHT;
		return EM_TRUE;
	}

	return EM_FALSE;
}

static EM_BOOL OnKeyUp(int t, const EmscriptenKeyboardEvent *e, void *_) {
	const char *code = e->code;

	if (!strcmp(code, "ArrowUp") || !strcmp(code, "KeyW")) {
		g_keyboard_state &= ~(BUTTON_UP);
		return EM_TRUE;
	}

	if (!strcmp(code, "ArrowDown") || !strcmp(code, "KeyS")) {
		g_keyboard_state &= ~(BUTTON_DOWN);
		return EM_TRUE;
	}

	if (!strcmp(code, "ArrowLeft") || !strcmp(code, "KeyA")) {
		g_keyboard_state &= ~(BUTTON_LEFT);
		return EM_TRUE;
	}

	if (!strcmp(code, "ArrowRight") || !strcmp(code, "KeyD")) {
		g_keyboard_state &= ~(BUTTON_RIGHT);
		return EM_TRUE;
	}

	return EM_FALSE;
}

U32 ReadGamepadState(void) {
	U32 button_flags = 0;
	if (emscripten_sample_gamepad_data() != EMSCRIPTEN_RESULT_SUCCESS) return 0;
	int n = emscripten_get_num_gamepads();
	if (n <= 0) return 0;

	EmscriptenGamepadEvent gamepad;
	if (emscripten_get_gamepad_status(0, &gamepad) != EMSCRIPTEN_RESULT_SUCCESS) return 0;

	if (gamepad.digitalButton[0])  button_flags |= BUTTON_A;
	if (gamepad.digitalButton[1])  button_flags |= BUTTON_B;
	if (gamepad.digitalButton[2])  button_flags |= BUTTON_X;
	if (gamepad.digitalButton[3])  button_flags |= BUTTON_Y;
	if (gamepad.digitalButton[8])  button_flags |= BUTTON_SELECT;
	if (gamepad.digitalButton[9])  button_flags |= BUTTON_START;
	if (gamepad.digitalButton[12]) button_flags |= BUTTON_UP;
	if (gamepad.digitalButton[13]) button_flags |= BUTTON_DOWN;
	if (gamepad.digitalButton[14]) button_flags |= BUTTON_LEFT;
	if (gamepad.digitalButton[15]) button_flags |= BUTTON_RIGHT;

	if (gamepad.axis[1] < -0.5) button_flags |= BUTTON_UP;
	if (gamepad.axis[1] >  0.5) button_flags |= BUTTON_DOWN;
	if (gamepad.axis[0] < -0.5) button_flags |= BUTTON_LEFT;
	if (gamepad.axis[0] >  0.5) button_flags |= BUTTON_RIGHT;
	return button_flags;
}

static bool RequestAnimationFrameCallback(F64 time, void *data) {
	if (g_last_time == 0) {
		g_last_time = time;
		return true;
	}

	F64 delta = time - g_last_time;
	g_remaining_time += delta;
	g_last_time = time;

	g_gamepad_state |= ReadGamepadState();

	const F64 update_rate = 1000.0 / 60.0;

	if (g_remaining_time >= update_rate) {
		if (g_remaining_time > update_rate * 6) {
			g_remaining_time = update_rate;
		}

		U32 current_button_state = g_keyboard_state | g_gamepad_state;

		GameInput input = {0};
		input.current_button_state = current_button_state;
		input.previous_button_state = g_previous_button_state;

		g_previous_button_state = current_button_state;
		g_gamepad_state = 0;

		do {
			GameFrame(g_framebuffer, g_frame_index, &input, 1);
			input.previous_button_state = input.current_button_state;
			g_remaining_time -= update_rate;
			g_frame_index += 1;
		} while (g_remaining_time >= update_rate);

		js_blit((void *)g_framebuffer, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
	}

	return true;
}

int main(void) {
	g_framebuffer = (U32 *)malloc(FRAME_BUFFER_WIDTH*FRAME_BUFFER_HEIGHT * 4);
	js_init_canvas(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);

	GameInit();

	emscripten_request_animation_frame_loop(RequestAnimationFrameCallback, NULL);
	emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, OnKeydown);
	emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, OnKeyUp);
}
