
#ifndef GAME_H
#define GAME_H

#define FRAME_BUFFER_WIDTH 32
#define FRAME_BUFFER_HEIGHT 32

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

void GameInit(U32 *frame_buffer, U32 *screen_width, U32 *screen_height);

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
	U32 animation_speed;
	U32 character_index;
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

void GameExit(void);

#endif
