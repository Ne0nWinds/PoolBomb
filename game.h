
#ifndef GAME_H
#define GAME_H

#define FRAME_BUFFER_WIDTH 256
#define FRAME_BUFFER_HEIGHT 192

#include <stdint.h>

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int8_t S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;

typedef float F32;
typedef double F64;

typedef _Bool Bool;
#define True ((Bool)(1))
#define False ((Bool)(0))

#define ArrayLength(arr) (sizeof(arr) / sizeof(arr[0]))

void GameInit(void);

enum Button {
	BUTTON_UP = 1u << 0u,
	BUTTON_RIGHT = 1u << 1u,
	BUTTON_DOWN = 1u << 2u,
	BUTTON_LEFT = 1u << 3u,

	BUTTON_A = 1u << 4u,
	BUTTON_B = 1u << 5u,
	BUTTON_X = 1u << 6u,
	BUTTON_Y = 1u << 7u,

	BUTTON_SELECT = 1u << 8u,
	BUTTON_START = 1u << 9u,
};

typedef struct GameInput {
	U32 current_button_state;
	U32 previous_button_state;
} GameInput;

static inline Bool IsButtonDown(GameInput input, enum Button button) {
	Bool result = (input.current_button_state & button) != 0;
	return result;
}

static inline Bool IsButtonUp(GameInput input, enum Button button) {
	Bool result = (input.current_button_state & button) == 0;
	return result;
}

static inline Bool WasButtonPressed(GameInput input, enum Button button) {
	Bool button_previously_up = (input.previous_button_state & button) == 0;
	Bool button_currently_down = (input.current_button_state & button) != 0;
	return button_previously_up && button_currently_down;
}
static inline Bool WasButtonReleased(GameInput input, enum Button button) {
	Bool button_previously_down = (input.previous_button_state & button) != 0;
	Bool button_currently_up = (input.current_button_state & button) == 0;
	return button_previously_down && button_currently_up;
}

void GameFrame(U32 *frame_buffer, uint64_t frame_index, GameInput *player_inputs, U32 player_count);

void GameRender(U32 *frame_buffer);

static void Print(const char *fmt, ...);

#define USE_DEBUG

#if defined(USE_DEBUG)

#include <stdio.h>
#include <stdarg.h>

#if defined(PLATFORM_EMSCRIPTEN)
#include <emscripten/console.h>

static void Print(const char *fmt, ...) {
	va_list args;
	char buffer[128];

	va_start(args, fmt);
	vsnprintf(buffer, 128, fmt, args);
	va_end(args);

	emscripten_console_log(buffer);
}

#elif defined(PLATFORM_WIN32)

#define _AMD64_
#include <windef.h>
#include <debugapi.h>

static void Print(const char *fmt, ...) {
	va_list args;
	char buffer[128];

	va_start(args, fmt);
	vsnprintf(buffer, 128, fmt, args);
	va_end(args);

	OutputDebugStringA(buffer);
}
#endif
#endif

#endif
