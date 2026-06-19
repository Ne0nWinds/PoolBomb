
#include "game.h"
#include "../game_helpers.h"
#include <emscripten.h>

static Bitmap g_spriteset;

static U32 g_animation_x_offsets[64];
static WalkAnimation g_walk_animations[DIRECTION_COUNT];

static U32 CharacterCount(void) {
	if (g_spriteset.pixels == 0) {
		return 0;
	}
	U32 count = g_spriteset.height / 48;
	return count == 0 ? 1 : count;
}

EM_JS(void, js_on_spriteset_loaded, (int character_count), {
	if (typeof window !== 'undefined' && window.onSpritesetLoaded) {
		window.onSpritesetLoaded(character_count);
	}
});

void GameInit(U32 *frame_buffer, U32 *screen_width, U32 *screen_height) {
	(void)frame_buffer;

	g_spriteset = LoadBitmap("assets/spriteset.png");
	SetupWalkAnimations(g_walk_animations, (U32 *)g_animation_x_offsets);

	*screen_width = 32;
	*screen_height = 32;

	js_on_spriteset_loaded((int)CharacterCount());
}

static U32 g_previous_animation_speed = 4;

void GameFrame(U32 *frame_buffer, uint64_t frame_index, GameInput *player_inputs, U32 player_count) {

	GameInput input = player_inputs[0];

	U32 character_count = CharacterCount();
	U32 character_index = input.character_index;
	if (character_count != 0 && character_index >= character_count) {
		character_index = character_count - 1;
	}

	for (U32 i = 0; i < FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT; ++i) {
		frame_buffer[i] = 0xFF404040;
	}

	Bool down = IsButtonDown(input, BUTTON_DOWN);
	Bool up = IsButtonDown(input, BUTTON_UP);
	Bool left = IsButtonDown(input, BUTTON_LEFT);
	Bool right = IsButtonDown(input, BUTTON_RIGHT);

	S32 y_movement = (S32)up - (S32)down;
	S32 x_movement = (S32)right - (S32)left;
	static Direction animation_direction = DIRECTION_DOWN;
	static U32 player_animation_frame = 0;

	if (g_previous_animation_speed != input.animation_speed) {
		player_animation_frame = 0;
		g_previous_animation_speed = input.animation_speed;
	}

	if (y_movement != 0) {
		if (animation_direction == DIRECTION_LEFT || animation_direction == DIRECTION_RIGHT) {
			player_animation_frame = 0;
		}
		if (y_movement > 0) {
			animation_direction = DIRECTION_UP;
		} else {
			animation_direction = DIRECTION_DOWN;
		}
	} else if (x_movement != 0) {
		if (animation_direction == DIRECTION_UP || animation_direction == DIRECTION_DOWN) {
			player_animation_frame = 0;
		}
		if (x_movement > 0) {
			animation_direction = DIRECTION_RIGHT;
		} else {
			animation_direction = DIRECTION_LEFT;
		}
	}

	AdvanceAndDisplayPlayerAnimationWalkCycle(frame_buffer, g_walk_animations, g_spriteset, character_index, &player_animation_frame, animation_direction, x_movement, y_movement, input.animation_speed);
}

void GameExit(void) {
}

EMSCRIPTEN_KEEPALIVE U32 GetCharacterCount(void) {
	return CharacterCount();
}

EMSCRIPTEN_KEEPALIVE U32 ReloadSpritesetFromMemory(U8 *data, U32 length) {
	int width, height, channels;
	unsigned char *pixels = stbi_load_from_memory(data, (int)length, &width, &height, &channels, 4);
	if (pixels == 0) {
		return CharacterCount();
	}

	if (g_spriteset.pixels != 0) {
		stbi_image_free(g_spriteset.pixels);
	}
	g_spriteset.width = (U32)width;
	g_spriteset.height = (U32)height;
	g_spriteset.pixels = (U32 *)pixels;

	U32 count = CharacterCount();
	js_on_spriteset_loaded((int)count);
	return count;
}
