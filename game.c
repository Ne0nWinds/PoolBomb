
#include "game.h"
#include "game_helpers.h"

#include <emscripten/console.h>

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

	DisplayWalkAnimationSheet(frame_buffer, base_spriteset, &g_walk_animations[DIRECTION_DOWN]);

	for (U32 y = 0; y < 13; ++y) {
		for (U32 x = 0; x < 17; ++x) {
			U8 tile_sprite_lookup = level[y*17 + x];

			Rectangle rect = {
				.x = (tile_sprite_lookup % 7) * 16,
				.y = (tile_sprite_lookup / 7) * 16,
				.width = 16,
				.height = 16,
			};
			BlitBitmapRectangleToFramebuffer(frame_buffer, (S32)x*16-8, y*16-8, tiles, rect);
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

	static const S32 player_width = 7;
	static const S32 tile_size = 16;

	static S64 player_y = 0 * 256;
	static S64 player_x = 0 * 256;

	{

		if (x_movement != 0) {
			S32 current_right_tile_index = (player_x + 128*16 + 7*256*x_movement) >> 12;
			S32 next_player_x = player_x + 128*x_movement;
			S32 next_tile_index = (next_player_x + 128*16 + 7*256*x_movement) >> 12;

			S32 top_tile_index = (player_y + 128*16 - 3*256) >> 12;
			S32 bottom_tile_index = (player_y + 128*16 + 3*256) >> 12;

			Bool collision_found = False;
			for (S32 y = top_tile_index; y <= bottom_tile_index; ++y) {
				U8 tile = level[(1 + y)*17 + (1 + next_tile_index)];
				if (tile != 6) {
					collision_found = True;
					break;
				}
			}

			if (collision_found) {
				player_x = (next_tile_index * 256*16) - (15 * 256)*x_movement;
			} else {
				player_x = next_player_x;
			}
		}

		player_y -= y_movement * 128;

#if 0
		S32 left_tile_index = (player_x + 128*16 - 7*256) >> 12;
		S32 right_tile_index = (player_x + 128*16 + 7*256) >> 12;

		S32 top_tile_index = (player_y + 128*16 - 3*256) >> 12;
		S32 bottom_tile_index = (player_y + 128*16 + 3*256) >> 12;

		for (S32 y = top_tile_index; y <= bottom_tile_index; ++y) {
			for (S32 x = left_tile_index; x <= right_tile_index; ++x) {

				Rectangle left_rect = {
					.x = (x + 1)*16 - 8,
					.y = (y + 1)*16 - 8,
					.width = 16,
					.height = 16
				};
				BlitColorRectangleToFramebuffer(frame_buffer, left_rect, 0xFF0000FF);
			}
		}
#endif
	}

	S32 player_render_y = (player_y/256)-8+1;
	S32 player_render_x = (player_x/256);

	AdvanceAndDisplayPlayerAnimationWalkCycle(frame_buffer, g_walk_animations, base_spriteset, 0, &player_animation_frame, animation_direction, x_movement, y_movement, previous_x_movement, previous_y_movement, player_render_x, player_render_y, 4);

	previous_x_movement = x_movement;
	previous_y_movement = y_movement;

	if (0) {
		S32 player_center_x = 16 + (player_x / 256) * 16;
		S32 player_center_y = 16 + (player_y / 256) * 16;

		frame_buffer[(player_center_y - 1) * FRAME_BUFFER_WIDTH + (player_center_x - 1)] = 0xFF0000FF;
		frame_buffer[(player_center_y - 1) * FRAME_BUFFER_WIDTH + player_center_x] = 0xFF0000FF;
		frame_buffer[player_center_y * FRAME_BUFFER_WIDTH + (player_center_x - 1)] = 0xFF0000FF;
		frame_buffer[player_center_y * FRAME_BUFFER_WIDTH + player_center_x] = 0xFF0000FF;
	}
}

void GameExit(void) {

}
