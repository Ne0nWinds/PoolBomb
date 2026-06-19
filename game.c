
#include "game.h"
#include "game_helpers.h"

#include <string.h>

static Bitmap base_spriteset;

static U32 g_animation_x_offsets[64];
static WalkAnimation g_walk_animations[DIRECTION_COUNT];

void GameInit(U32 *frame_buffer, U32 *screen_width, U32 *screen_height) {

	base_spriteset = LoadBitmap("assets/spriteset.png");

	SetupWalkAnimations(g_walk_animations, g_animation_x_offsets);

	*screen_width = FRAME_BUFFER_WIDTH;
	*screen_height = FRAME_BUFFER_HEIGHT;
}

static void DisplayWalkAnimationSheet(U32 *frame_buffer, Bitmap spriteset, WalkAnimation *animation) {
	U32 frame_count = animation->frame_count;
	for (U32 i = 0; i < frame_count; ++i) {
		U32 x_offset = animation->x_offsets[i];
		Rectangle rect = {
			.x = x_offset,
			.y = 16,
			.width = 32,
			.height = 32
		};

		U32 x_pos = i%4 * 48;
		U32 y_pos = i/4 * 48;
		BlitBitmapRectangleToFramebuffer(frame_buffer, x_pos, y_pos, spriteset, rect);
	}
}


void GameFrame(U32 *frame_buffer, uint64_t frame_index, GameInput *player_inputs, U32 player_count) {

	memset(frame_buffer, 0xFF, sizeof(U32)*FRAME_BUFFER_WIDTH*FRAME_BUFFER_HEIGHT);

	Bool down = IsButtonDown(player_inputs[0], BUTTON_DOWN);
	Bool up = IsButtonDown(player_inputs[0], BUTTON_UP);
	Bool left = IsButtonDown(player_inputs[0], BUTTON_LEFT);
	Bool right = IsButtonDown(player_inputs[0], BUTTON_RIGHT);

	S32 y_movement = (S32)up - (S32)down;
	S32 x_movement = (S32)right - (S32)left;

	static Direction animation_direction = DIRECTION_DOWN;
	static U32 player_animation_frame = 0;

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

	AdvanceAndDisplayPlayerAnimationWalkCycle(frame_buffer, g_walk_animations, base_spriteset, 0, &player_animation_frame, animation_direction, x_movement, y_movement);
}

void GameExit(void) {

}
