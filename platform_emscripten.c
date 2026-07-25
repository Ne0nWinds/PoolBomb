
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/console.h>

#include "game.h"

static U32 *g_framebuffer;

EM_JS(void, js_init_canvas, (int width, int height), {
	canvas.width = width;
	canvas.height = height;
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

static EM_BOOL OnWindowLoseFocus(int t, const EmscriptenFocusEvent *e, void *_) {
	g_keyboard_state = 0;
	return EM_TRUE;
}

static inline void SetOrClear(U32 *value, U32 bit, Bool set) {
	if (set) {
		*value |= bit;
	} else {
		*value &= ~bit;
	}
}

static EM_BOOL HandleKey(int t, const EmscriptenKeyboardEvent *e, void *_) {

	const char *code = e->code;
	Bool key_down = (t == EMSCRIPTEN_EVENT_KEYDOWN);

	if (!strcmp(code, "ArrowUp") || !strcmp(code, "KeyW")) {
		SetOrClear(&g_keyboard_state, BUTTON_UP, key_down);
		return EM_TRUE;
	}

	if (!strcmp(code, "ArrowDown") || !strcmp(code, "KeyS")) {
		SetOrClear(&g_keyboard_state, BUTTON_DOWN, key_down);
		return EM_TRUE;
	}

	if (!strcmp(code, "ArrowLeft") || !strcmp(code, "KeyA")) {
		SetOrClear(&g_keyboard_state, BUTTON_LEFT, key_down);
		return EM_TRUE;
	}

	if (!strcmp(code, "ArrowRight") || !strcmp(code, "KeyD")) {
		SetOrClear(&g_keyboard_state, BUTTON_RIGHT, key_down);
		return EM_TRUE;
	}

	if (!strcmp(code, "Space")) {
		SetOrClear(&g_keyboard_state, BUTTON_X, key_down);
		return EM_TRUE;
	}

	return EM_FALSE;
}

U32 ReadGamepadState(int gamepad_idx) {
	U32 button_flags = 0;

	EmscriptenGamepadEvent gamepad;
	if (emscripten_get_gamepad_status(gamepad_idx, &gamepad) != EMSCRIPTEN_RESULT_SUCCESS) return 0;

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

	F64 joystick_threshold = 9.0 / 16.0;

	if (gamepad.axis[1] < -joystick_threshold) button_flags |= BUTTON_UP;
	if (gamepad.axis[1] >  joystick_threshold) button_flags |= BUTTON_DOWN;
	if (gamepad.axis[0] < -joystick_threshold) button_flags |= BUTTON_LEFT;
	if (gamepad.axis[0] >  joystick_threshold) button_flags |= BUTTON_RIGHT;

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

	U32 gamepad_states[3] = {0};
	U32 gamepads_connected = 0;
	if (emscripten_sample_gamepad_data() == EMSCRIPTEN_RESULT_SUCCESS) {
		int gamepad_count = emscripten_get_num_gamepads();
		gamepads_connected = (U32)gamepad_count;
		for (int i = 0; i < gamepad_count; ++i) {
			gamepad_states[i] |= ReadGamepadState(i);
		}
	}

	const F64 update_rate = 1000.0 / 60.0;

	if (g_remaining_time >= update_rate) {
		if (g_remaining_time > update_rate * 6) {
			g_remaining_time = update_rate;
		}

		static GameInput inputs[4] = {0};
		for (U32 i = 0; i < 4; ++i) {
			inputs[i].previous_button_state = inputs[i].current_button_state;
		}
		inputs[0].current_button_state = g_keyboard_state;
		for (U32 i = 0; i < gamepads_connected; ++i) {
			inputs[i+1].current_button_state = gamepad_states[i];
		}

		if (g_remaining_time > update_rate*6) {
			g_remaining_time = update_rate;
		}

		if (g_remaining_time >= update_rate) {

			do {
				g_remaining_time -= update_rate;
				Bool should_render = g_remaining_time < update_rate;
				GameFrame(g_framebuffer, g_frame_index, inputs, gamepads_connected + 1, should_render);

				for (U32 i = 0; i < 4; ++i) {
					inputs[i].previous_button_state = inputs[i].current_button_state;
				}

				g_frame_index += 1;
			} while (g_remaining_time >= update_rate);
		}

		js_blit((void *)g_framebuffer, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
	}

	return true;
}

int main(void) {
	g_framebuffer = (U32 *)malloc(FRAME_BUFFER_WIDTH*FRAME_BUFFER_HEIGHT * 4);
	js_init_canvas(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);

	GameInit();

	emscripten_request_animation_frame_loop(RequestAnimationFrameCallback, NULL);
	emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, HandleKey);
	emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, HandleKey);
	emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, OnWindowLoseFocus);
}
