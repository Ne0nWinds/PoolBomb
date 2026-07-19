
#include "game.h"
#include "game_helpers.h"

#include <string.h>

static Bitmap base_spriteset;
static Bitmap tiles;

static U8 level[] = {
	#include "levels/test.csv"
};

static S32 S32_Abs(S32 a) {
	if (a < 0) a = -a;
	return a;
}

static U32 g_animation_x_offsets[64];
static WalkAnimation g_walk_animations[DIRECTION_COUNT];

void GameInit(void) {

	SetupWalkAnimations(g_walk_animations, g_animation_x_offsets);

	base_spriteset = LoadBitmap("assets/spriteset.png");
	tiles = LoadBitmap("assets/tiles.png");
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

static const S32 move_speed = 160+16;

static S64 player_y = 0 * 256;
static S64 player_x = 0 * 256;

static Direction g_animation_direction = DIRECTION_DOWN;
static U32 g_animation_tick = 0;

static S32 y_movement = 0;
static S32 x_movement = 0;

static void GameRender(U32 *frame_buffer);

void GameFrame(U32 *frame_buffer, uint64_t frame_index, GameInput *player_inputs, U32 player_count, Bool should_render) {

	Bool down = IsButtonDown(player_inputs[0], BUTTON_DOWN);
	Bool up = IsButtonDown(player_inputs[0], BUTTON_UP);
	Bool left = IsButtonDown(player_inputs[0], BUTTON_LEFT);
	Bool right = IsButtonDown(player_inputs[0], BUTTON_RIGHT);

	S32 previous_x_movement = x_movement;
	S32 previous_y_movement = y_movement;

	y_movement = (S32)down - (S32)up;
	x_movement = (S32)right - (S32)left;

	{
		S32 player_tile_x = WorldToTile(player_x);
		S32 player_tile_y = WorldToTile(player_y);

		S32 local_tiles[9];
		for (S32 y = -1; y <= 1; ++y) {
			for (S32 x = -1; x <= 1; ++x) {
				S32 level_y = player_tile_y + y;
				S32 level_x = player_tile_x + x;
				Bool in_bounds = level_y >= 0 && level_x >= 0 && level_y < 13 && level_x < 17;
				local_tiles[(y+1)*3 + (x+1)] = in_bounds ? level[level_y*17 + level_x] : 0;
			}
		}
		Bool bottom_tile_occupied = local_tiles[2*3 + 1] != 6;
		Bool top_tile_occupied = local_tiles[0*3 + 1] != 6;
		Bool bottom_right_occupied = local_tiles[2*3 + 2] != 6;
		Bool top_right_occupied = local_tiles[0*3 + 2] != 6;
		Bool bottom_left_occupied = local_tiles[2*3 + 0] != 6;
		Bool top_left_occupied = local_tiles[0*3 + 0] != 6;
		Bool left_tile_occupied = local_tiles[1*3 + 0] != 6;
		Bool right_tile_occupied = local_tiles[1*3 + 2] != 6;

		if (y_movement != 0 || x_movement != 0) {

			S32 current_tile_offset_y = (player_y - SUBPIXELS_PER_TILE/2) & (SUBPIXELS_PER_TILE - 1);
			S32 current_tile_offset_x = (player_x - SUBPIXELS_PER_TILE/2) & (SUBPIXELS_PER_TILE - 1);

			S32 offset_from_center_x = current_tile_offset_x - SUBPIXELS_PER_TILE/2;
			S32 offset_from_center_y = current_tile_offset_y - SUBPIXELS_PER_TILE/2;

			Bool is_in_center_x = offset_from_center_x == 0;
			Bool is_in_center_y = offset_from_center_y == 0;

			S32 next_player_y = player_y + move_speed*y_movement;
			S32 next_player_x = player_x + move_speed*x_movement;

			S32 corner_assist_threshold = SUBPIXELS_PER_TILE/3;
			S32 corner_assist_from_center = SUBPIXELS_PER_TILE/2 - corner_assist_threshold;

			S32 distance_to_top_edge = SUBPIXELS_PER_TILE/2 + offset_from_center_y;
			S32 distance_to_bottom_edge = SUBPIXELS_PER_TILE/2 - offset_from_center_y;
			S32 distance_to_right_edge = SUBPIXELS_PER_TILE/2 - offset_from_center_x;
			S32 distance_to_left_edge = SUBPIXELS_PER_TILE/2 + offset_from_center_x;

			S32 held_x_movement = x_movement;

			Bool force_auto_adjust = x_movement != 0 && y_movement != 0;
			if (y_movement < 0 && x_movement > 0) {
				if (bottom_tile_occupied && top_tile_occupied) {
					y_movement = 0;
				} else if (left_tile_occupied && right_tile_occupied) {
					x_movement = 0;
				} else if (right_tile_occupied) {
					if (!top_tile_occupied) {
						x_movement = 0;
					} else if (offset_from_center_x >= 0) {
						x_movement = 0;
					} else {
						y_movement = 0;
					}
				} else if (top_tile_occupied) {
					y_movement = 0;
				} else if (distance_to_right_edge < distance_to_top_edge) {
					y_movement = 0;
				} else {
					x_movement = 0;
				}
			} else if (y_movement < 0 && x_movement < 0) {
				if (bottom_tile_occupied && top_tile_occupied) {
					y_movement = 0;
				} else if (left_tile_occupied && right_tile_occupied) {
					x_movement = 0;
				} else if (left_tile_occupied) {
					if (!top_tile_occupied) {
						x_movement = 0;
					} else if (offset_from_center_x <= 0) {
						x_movement = 0;
					} else {
						y_movement = 0;
					}
				} else if (top_tile_occupied) {
					y_movement = 0;
				} else if (distance_to_left_edge < distance_to_top_edge) {
					y_movement = 0;
				} else {
					x_movement = 0;
				}
			} else if (y_movement > 0 && x_movement > 0) {
				if (bottom_tile_occupied && top_tile_occupied) {
					y_movement = 0;
				} else if (left_tile_occupied && right_tile_occupied) {
					x_movement = 0;
				} else if (right_tile_occupied) {
					if (!bottom_tile_occupied) {
						x_movement = 0;
					} else if (offset_from_center_x >= 0) {
						x_movement = 0;
					} else {
						y_movement = 0;
					}
				} else if (bottom_tile_occupied) {
					y_movement = 0;
				} else if (distance_to_right_edge < distance_to_bottom_edge) {
					y_movement = 0;
				} else {
					x_movement = 0;
				}
			} else if (y_movement > 0 && x_movement < 0) {
				if (bottom_tile_occupied && top_tile_occupied) {
					y_movement = 0;
				} else if (left_tile_occupied && right_tile_occupied) {
					x_movement = 0;
				} else if (left_tile_occupied) {
					if (!bottom_tile_occupied) {
						x_movement = 0;
					} else if (offset_from_center_x <= 0) {
						x_movement = 0;
					} else {
						y_movement = 0;
					}
				} else if (bottom_tile_occupied) {
					y_movement = 0;
				} else if (distance_to_left_edge < distance_to_bottom_edge) {
					y_movement = 0;
				} else {
					x_movement = 0;
				}
			}

			if (y_movement != 0 && !x_movement) {
				Bool adjacent_occupied = local_tiles[(1 + y_movement)*3 + 1] != 6;

				Bool should_corner_assist_right = offset_from_center_x >= corner_assist_from_center && adjacent_occupied && held_x_movement >= 0;
				should_corner_assist_right &= (y_movement > 0 && !bottom_right_occupied) || (y_movement < 0 && !top_right_occupied);

				Bool should_corner_assist_left = offset_from_center_x <= -corner_assist_from_center && adjacent_occupied && held_x_movement <= 0;
				should_corner_assist_left &= (y_movement > 0 && !bottom_left_occupied) || (y_movement < 0 && !top_left_occupied);

				Bool continued_corner_assist_right = !adjacent_occupied && offset_from_center_x < -(SUBPIXELS_PER_TILE/4) && held_x_movement >= 0;
				if (y_movement > 0) {
					continued_corner_assist_right &= offset_from_center_y > SUBPIXELS_PER_TILE/4;
				} else {
					continued_corner_assist_right &= offset_from_center_y < -(SUBPIXELS_PER_TILE/4);
				}

				Bool continued_corner_assist_left = !adjacent_occupied && offset_from_center_x > SUBPIXELS_PER_TILE/4 && held_x_movement <= 0;
				if (y_movement > 0) {
					continued_corner_assist_left &= offset_from_center_y > SUBPIXELS_PER_TILE/4;
				} else {
					continued_corner_assist_left &= offset_from_center_y < -(SUBPIXELS_PER_TILE/4);
				}

				if (should_corner_assist_right || continued_corner_assist_right) {
					player_x += move_speed;
				} else if (should_corner_assist_left || continued_corner_assist_left) {
					player_x -= move_speed;
				} else {

					if (y_movement > 0) {
						S32 bottom_edge = TileToWorld(player_tile_y + 1) - 13*256;
						if (bottom_tile_occupied && next_player_y >= bottom_edge) {
							player_y = bottom_edge;
						} else {
							player_y = next_player_y;
						}
					} else {
						S32 top_edge = TileToWorld(player_tile_y - 1) + 12*256;
						if (top_tile_occupied && next_player_y <= top_edge) {
							player_y = top_edge;
						} else {
							player_y = next_player_y;
						}
					}

					if (!is_in_center_x) {
						if (!adjacent_occupied || (y_movement < 0 && offset_from_center_y >= 0) || (y_movement > 0 && offset_from_center_y <= 0)) {
							if (offset_from_center_x >= -move_speed && offset_from_center_x <= move_speed) {
								player_x -= offset_from_center_x;
							} else {
								S32 x_move = (offset_from_center_x >> 31) | 1;
								player_x += move_speed*(x_move * -1);
							}
						}
					}
				}

			} else if (x_movement != 0 && !y_movement) {
				Bool adjacent_occupied = local_tiles[1*3 + 1 + x_movement] != 6;


				Bool top_right_occupied = local_tiles[0*3 + 2] != 6;
				Bool top_left_occupied = local_tiles[0*3 + 0] != 6;

				Bool bottom_right_occupied = local_tiles[2*3 + 2] != 6;
				Bool bottom_left_occupied = local_tiles[2*3 + 0] != 6;

				Bool should_corner_assist_up = offset_from_center_y <= -corner_assist_from_center && adjacent_occupied;
				should_corner_assist_up &= (x_movement > 0 && !top_right_occupied) || (x_movement < 0 && !top_left_occupied);

				Bool should_corner_assist_down = offset_from_center_y >= corner_assist_from_center && adjacent_occupied;
				should_corner_assist_down &= (x_movement > 0 && !bottom_right_occupied) || (x_movement < 0 && !bottom_left_occupied);

				if (should_corner_assist_up) {
					player_y -= move_speed;
				} else if (should_corner_assist_down) {
					player_y += move_speed;
				} else if (is_in_center_y || adjacent_occupied) {

					if (x_movement > 0) {
						Bool right_tile_occupied = local_tiles[1*3 + 2] != 6;
						S32 right_edge = TileToWorld(player_tile_x + 1) - 15*256;
						if (right_tile_occupied && next_player_x >= right_edge) {
							player_x = right_edge;
						} else {
							player_x = next_player_x;
						}
					} else {
						Bool right_tile_occupied = local_tiles[1*3 + 0] != 6;
						S32 left_edge = TileToWorld(player_tile_x - 1) + 15*256;
						if (right_tile_occupied && next_player_x <= left_edge) {
							player_x = left_edge;
						} else {
							player_x = next_player_x;
						}
					}

				} else {
					player_x = next_player_x;

					Bool cancel_auto_adjust_y = False;

					if (offset_from_center_y < -(SUBPIXELS_PER_TILE/4)) {

						if (x_movement > 0) {
							if (!top_right_occupied) {
								cancel_auto_adjust_y = True;
							}
						} else {
							if (!top_left_occupied) {
								cancel_auto_adjust_y = True;
							}
						}
					} else if (offset_from_center_y > SUBPIXELS_PER_TILE/4) {

						if (x_movement > 0) {
							if (!bottom_right_occupied) {
								cancel_auto_adjust_y = True;
							}
						} else {
							if (!bottom_left_occupied) {
								cancel_auto_adjust_y = True;
							}
						}
					}


					if (!cancel_auto_adjust_y || force_auto_adjust) {
						if (offset_from_center_y >= -move_speed && offset_from_center_y <= move_speed) {
							player_y -= offset_from_center_y;
						} else {
							S32 y_move = (offset_from_center_y >> 31) | 1;
							player_y += move_speed*(y_move * -1);
						}
					}
				}
			}

		}
	}


	Direction next_animation_direction = DIRECTION_DOWN;

	if (y_movement != 0 && x_movement == 0) {
		if (y_movement > 0) {
			next_animation_direction = DIRECTION_DOWN;
		} else {
			next_animation_direction = DIRECTION_UP;
		}
	} else if (x_movement != 0 && y_movement == 0) {
		if (x_movement > 0) {
			next_animation_direction = DIRECTION_RIGHT;
		} else {
			next_animation_direction = DIRECTION_LEFT;
		}
	}

	if (y_movement != 0 || x_movement != 0) {
		if (g_animation_direction != next_animation_direction) {
			g_animation_tick = 0;
		}
		g_animation_direction = next_animation_direction;
	}

	U32 animation_frame_delay = 4;

	WalkAnimation walk_animation = g_walk_animations[g_animation_direction];
	U32 animation_length = walk_animation.frame_count;
	Bool started_moving = (x_movement != 0 || y_movement != 0) && (previous_x_movement != 0 && previous_y_movement != 0);
	if (started_moving) {
		g_animation_tick += walk_animation.start_offset * animation_frame_delay;
	}
	U32 animation_index = (g_animation_tick / animation_frame_delay) % animation_length;
	Bool animation_finished = animation_index == 0 || animation_index == walk_animation.neutral_frame;
	if (x_movement != 0 || y_movement != 0) {
		g_animation_tick += 1;
	} else if (!animation_finished) {
		U32 behind = (animation_index <= walk_animation.neutral_frame) ? 0 : walk_animation.neutral_frame;
		U32 ahead = (animation_index < walk_animation.neutral_frame) ? walk_animation.neutral_frame : animation_length;

		Bool should_reverse = (animation_index - behind) <= (ahead - animation_index);

		if (should_reverse) {
			g_animation_tick -= 1;
			U32 next_animation_index = (g_animation_tick / animation_frame_delay) % animation_length;
			if (next_animation_index == 0) {
				g_animation_tick = walk_animation.neutral_frame * animation_frame_delay;
			}

			if (next_animation_index == walk_animation.neutral_frame) {
				g_animation_tick = 0;
			}
		} else {
			g_animation_tick += 1;
		}
	}

	if (should_render) {
		for (U32 y = 0; y < 13; ++y) {
			for (U32 x = 0; x < 17; ++x) {
				U8 tile_sprite_lookup = level[y*17 + x];

				Rectangle rect = {
					.x = (tile_sprite_lookup % 7) * 16,
					.y = (tile_sprite_lookup / 7) * 16,
					.width = 16,
					.height = 16,
				};

				BlitBitmapRectangleToFramebuffer(frame_buffer, (S32)x*16-8, (S32)y*16-8, tiles, rect);
			}
		}

		Rectangle sprite_rect = {
			.x = walk_animation.x_offsets[animation_index],
			.y = 16 + 48*0,
			.width = 32,
			.height = 32
		};

		S32 player_render_y = ((player_y - 128)>>8)-8;
		S32 player_render_x = ((player_x + 128)>>8);
		BlitBitmapRectangleToFramebuffer(frame_buffer, player_render_x, player_render_y, base_spriteset, sprite_rect);
	}
}
