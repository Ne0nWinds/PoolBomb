
#include "game.h"
#include "../game_helpers.h"
#include <emscripten.h>
#include <emscripten/console.h>

enum AnimationMode {
	ANIM_MODE_WASD_CONTROL = 0,
	ANIM_MODE_WALK_UP,
	ANIM_MODE_WALK_RIGHT,
	ANIM_MODE_WALK_DOWN,
	ANIM_MODE_WALK_LEFT,
	ANIM_MODE_WALK_DEATH,
};

static Bitmap g_spriteset;

static U32 g_animation_x_offsets[64];
static U32 g_death_animation_x_offset[9];
static WalkAnimation g_walk_animations[DIRECTION_COUNT];
static BasicAnimation g_death_animation;

static U32 CharacterCount(void) {
	if (g_spriteset.pixels == 0) {
		return 0;
	}
	U32 count = g_spriteset.height / 48 - 1;
	return count == 0 ? 1 : count;
}

EM_JS(void, js_on_spriteset_loaded, (int character_count), {
	if (typeof window !== 'undefined' && window.onSpritesetLoaded) {
		window.onSpritesetLoaded(character_count);
	}
});

void GameInit(U32 *frame_buffer, U32 *screen_width, U32 *screen_height) {
	(void)frame_buffer;

	g_spriteset = LoadBitmap("assets/spriteset2.png");

	SetupWalkAnimations(g_walk_animations, (U32 *)g_animation_x_offsets);
	SetupDeathAnimation(&g_death_animation, (U32 *)g_death_animation_x_offset);

	*screen_width = 32;
	*screen_height = 32;

	js_on_spriteset_loaded((int)CharacterCount());
}

static U32 g_previous_animation_speed = 4;
static U32 g_animation_mode = 0;

static Player player;

void GameFrame(U32 *frame_buffer, uint64_t frame_index, GameInput *player_inputs, U32 player_count, Bool should_render) {

	GameInput input = player_inputs[0];

	U32 character_count = CharacterCount();
	U32 character_index = input.character_index;
	if (character_index == 1) {
		character_index += 1;
	}

	for (U32 i = 0; i < FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT; ++i) {
		frame_buffer[i] = 0xFF404040;
	}

	if (g_previous_animation_speed != input.animation_speed) {
		player.animation_tick = 0;
		g_previous_animation_speed = input.animation_speed;
	}

	if (g_animation_mode == ANIM_MODE_WASD_CONTROL) {
		Bool down = IsButtonDown(input, BUTTON_DOWN);
		Bool up = IsButtonDown(input, BUTTON_UP);
		Bool left = IsButtonDown(input, BUTTON_LEFT);
		Bool right = IsButtonDown(input, BUTTON_RIGHT);

		S32 y_movement = (S32)down - (S32)up;
		S32 x_movement = (S32)right - (S32)left;
		static Direction animation_direction = DIRECTION_DOWN;

		if (y_movement != 0) {
			if (animation_direction == DIRECTION_LEFT || animation_direction == DIRECTION_RIGHT) {
				player.animation_tick = 0;
			}
			if (y_movement > 0) {
				animation_direction = DIRECTION_UP;
			} else {
				animation_direction = DIRECTION_DOWN;
			}
		} else if (x_movement != 0) {
			if (animation_direction == DIRECTION_UP || animation_direction == DIRECTION_DOWN) {
				player.animation_tick = 0;
			}
			if (x_movement > 0) {
				animation_direction = DIRECTION_RIGHT;
			} else {
				animation_direction = DIRECTION_LEFT;
			}
		}

		UpdatePlayerAnimationTick(&player, x_movement, y_movement, input.animation_speed, g_walk_animations);

		// RenderPlayer(&player, frame_buffer, g_spriteset, input.character_index, g_walk_animations);

		if (should_render) {
			WalkAnimation walk_animation = g_walk_animations[player.animation_direction];
			Rectangle sprite_rect = {
				.x = walk_animation.x_offsets[player.animation_index],
				.y = 16 + 48*character_index,
				.width = 32,
				.height = 32
			};
			BlitBitmapRectangleToFramebuffer(frame_buffer, 0, 0, g_spriteset, sprite_rect);

			if (input.render_water_displacement) {
				Rectangle sprite_rect = {
					.x = walk_animation.x_offsets[player.animation_index],
					.y = 16 + 48*(1),
					.width = 32,
					.height = 32
				};
				BlitBitmapRectangleToFramebufferWithOpacity(frame_buffer, 0, 0, g_spriteset, sprite_rect, input.water_displacement_opacity);
			}
		}

	} else if (g_animation_mode == ANIM_MODE_WALK_DEATH) {

		static Bool should_play_death_animation = False;

		static const U32 animation_end_tick_delay = 45;
		static U32 animation_end_ticks = animation_end_tick_delay;
		if (WasButtonReleased(input, BUTTON_A)) {
			should_play_death_animation = True;
		}

		if (should_play_death_animation) {
			AnimationState state = AdvanceBasicAnimation(g_death_animation, input.animation_speed, player.animation_tick);

			U32 animation_render_index = state.animation_render_index;
			player.animation_tick = state.global_animation_tick;

			if (should_render) {
				U32 *x_offsets = g_death_animation.x_offsets;
				DisplayAnimationFrame(frame_buffer, x_offsets, g_spriteset, character_index, animation_render_index, 0, 0);
				if (input.render_water_displacement) {
					Rectangle sprite_rect = {
						.x = x_offsets[animation_render_index],
						.y = 16 + 48*(1),
						.width = 32,
						.height = 32
					};
					BlitBitmapRectangleToFramebufferWithOpacity(frame_buffer, 0, 0, g_spriteset, sprite_rect, input.water_displacement_opacity);
				}
			}

			if (state.animation_will_end_next_tick) {
				should_play_death_animation = False;
				animation_end_ticks = 0;
			}
		} else if (animation_end_ticks < animation_end_tick_delay) {
			animation_end_ticks += 1;
		} else if (should_render) {
			U32 *x_offsets = g_walk_animations[DIRECTION_DOWN].x_offsets;
			DisplayAnimationFrame(frame_buffer, x_offsets, g_spriteset, character_index, 0, 0, 0);

			if (input.render_water_displacement) {
				Rectangle sprite_rect = {
					.x = x_offsets[0],
					.y = 16 + 48*(1),
					.width = 32,
					.height = 32
				};
				BlitBitmapRectangleToFramebufferWithOpacity(frame_buffer, 0, 0, g_spriteset, sprite_rect, input.water_displacement_opacity);
			}
		}

	} else {
		WalkAnimation walk_animation;

		switch (g_animation_mode) {
			case ANIM_MODE_WALK_UP: {
				walk_animation = g_walk_animations[DIRECTION_UP];
			} break;
			case ANIM_MODE_WALK_RIGHT: {
				walk_animation = g_walk_animations[DIRECTION_RIGHT];
			} break;
			case ANIM_MODE_WALK_DOWN: {
				walk_animation = g_walk_animations[DIRECTION_DOWN];
			} break;
			case ANIM_MODE_WALK_LEFT: {
				walk_animation = g_walk_animations[DIRECTION_LEFT];
			} break;
		}


		BasicAnimation basic_animation = {
			.x_offsets = walk_animation.x_offsets,
			.frame_count = walk_animation.frame_count
		};
		AnimationState state = AdvanceBasicAnimation(basic_animation, input.animation_speed, player.animation_tick);
		player.animation_tick = state.global_animation_tick;

		U32 *x_offsets = basic_animation.x_offsets;
		DisplayAnimationFrame(frame_buffer, basic_animation.x_offsets, g_spriteset, character_index, state.animation_render_index, 0, 0);

		if (input.render_water_displacement) {
			Rectangle sprite_rect = {
				.x = x_offsets[state.animation_render_index],
				.y = 16 + 48*(1),
				.width = 32,
				.height = 32
			};
			BlitBitmapRectangleToFramebufferWithOpacity(frame_buffer, 0, 0, g_spriteset, sprite_rect, input.water_displacement_opacity);
		}
	}
}

EMSCRIPTEN_KEEPALIVE U32 GetCharacterCount(void) {
	return CharacterCount();
}

EMSCRIPTEN_KEEPALIVE void SetAnimationMode(U32 in_animation_mode) {
	g_animation_mode = in_animation_mode;
	player.animation_tick = 0;
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
