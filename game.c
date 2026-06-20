
#include "game.h"
#include "game_helpers.h"

#include <string.h>

static Bitmap base_spriteset;

static U32 g_animation_x_offsets[64];
static WalkAnimation g_walk_animations[DIRECTION_COUNT];

void GameInit(void) {

	base_spriteset = LoadBitmap("assets/spriteset.png");

	SetupWalkAnimations(g_walk_animations, g_animation_x_offsets);
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

	// DisplayWalkAnimationSheet(frame_buffer, base_spriteset, &g_walk_animations[DIRECTION_DOWN]);

	Bool down = IsButtonDown(player_inputs[0], BUTTON_DOWN);
	Bool up = IsButtonDown(player_inputs[0], BUTTON_UP);
	Bool left = IsButtonDown(player_inputs[0], BUTTON_LEFT);
	Bool right = IsButtonDown(player_inputs[0], BUTTON_RIGHT);

	static S32 previous_x_movement = 0;
	static S32 previous_y_movement = 0;

	S32 y_movement = (S32)up - (S32)down;
	S32 x_movement = (S32)right - (S32)left;

	static Direction animation_direction = DIRECTION_DOWN;
	static U32 player_animation_frame = 0;
	static S32 player_y = 0;
	static S32 player_x = 0;

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

	player_y -= y_movement;
	player_x += x_movement;

	AdvanceAndDisplayPlayerAnimationWalkCycle(frame_buffer, g_walk_animations, base_spriteset, 0, &player_animation_frame, animation_direction, x_movement, y_movement, previous_x_movement, previous_y_movement, player_x >> 1, player_y >> 1, 4);

	previous_x_movement = x_movement;
	previous_y_movement = y_movement;
}

void GameExit(void) {

}
