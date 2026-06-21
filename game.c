
#include "game.h"
#include "game_helpers.h"

#include <string.h>

static Bitmap base_spriteset;
static Bitmap tiles;

static U8 level[] = {
	#include "levels/test.csv"
};

static U32 g_animation_x_offsets[64];
static WalkAnimation g_walk_animations[DIRECTION_COUNT];

void GameInit(void) {

	base_spriteset = LoadBitmap("assets/spriteset.png");
	tiles = LoadBitmap("assets/tiles.png");

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

	memset(frame_buffer, 0x0, sizeof(U32)*FRAME_BUFFER_WIDTH*FRAME_BUFFER_HEIGHT);

	// DisplayWalkAnimationSheet(frame_buffer, base_spriteset, &g_walk_animations[DIRECTION_DOWN]);

	for (U32 y = 0; y < 10; ++y) {
		for (U32 x = 0; x < 17; ++x) {
			U8 tile_sprite_lookup = level[y*17 + x];

			Rectangle rect = {
				.x = (tile_sprite_lookup % 7) * 16,
				.y = (tile_sprite_lookup / 7) * 16,
				.width = 16,
				.height = 16,
			};
			BlitBitmapRectangleToFramebuffer(frame_buffer, (S32)x*16 - 24, y*16, tiles, rect);
		}
	}

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

	Direction next_animation_direction = DIRECTION_DOWN;
	if (y_movement != 0) {
		if (y_movement > 0) {
			next_animation_direction = DIRECTION_UP;
		} else {
			next_animation_direction = DIRECTION_DOWN;
		}
	} else if (x_movement != 0) {
		if (x_movement > 0) {
			next_animation_direction = DIRECTION_RIGHT;
		} else {
			next_animation_direction = DIRECTION_LEFT;
		}
	}
	if (y_movement != 0 || x_movement != 0) {
		if (animation_direction != next_animation_direction) {
			player_animation_frame = 0;
		}
		animation_direction = next_animation_direction;
	}

	player_y -= y_movement;
	player_x += x_movement;

	AdvanceAndDisplayPlayerAnimationWalkCycle(frame_buffer, g_walk_animations, base_spriteset, 0, &player_animation_frame, animation_direction, x_movement, y_movement, previous_x_movement, previous_y_movement, player_x >> 1, player_y >> 1, 4);

	previous_x_movement = x_movement;
	previous_y_movement = y_movement;
}

void GameExit(void) {

}
