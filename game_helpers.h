
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

typedef struct Bitmap {
	U32 width, height;
	U32 *pixels;
} Bitmap;

Bitmap LoadBitmap(const char *path) {
	int width, height, channels;
	unsigned char *pixels = stbi_load(path, &width, &height, &channels, 4);

	Bitmap result;
	result.width = width;
	result.height = height;
	result.pixels = (U32 *)pixels;
	return result;
}

typedef enum Direction {
	DIRECTION_DOWN,
	DIRECTION_UP,
	DIRECTION_LEFT,
	DIRECTION_RIGHT,
	DIRECTION_COUNT,
} Direction;

typedef struct Rectangle {
	S32 x, y;
	U32 width, height;
} Rectangle;

typedef struct BasicAnimation {
	U32 *x_offsets;
	U32 frame_count;
} BasicAnimation;

typedef struct WalkAnimation {
	U32 *x_offsets;
	U32 frame_count;
	U32 neutral_frame;
	U32 start_offset;
} WalkAnimation;

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static Rectangle RectangleIntersection(Rectangle a, Rectangle b) {

	S32 left = MAX(a.x, b.x);
	S32 top = MAX(a.y, b.y);
	S32 right = MIN(a.x + (S32)a.width, b.x + (S32)b.width);
	S32 bottom = MIN(a.y + (S32)a.height, b.y + (S32)b.height);

	if (right <= left || bottom <= top) {
		return (Rectangle){0};
	}

	Rectangle result = {
		.x = left - a.x,
		.y = top - a.y,
		.width = (U32)(right - left),
		.height = (U32)(bottom - top)
	};
	return result;
}

static void BlitBitmapRectangleToFramebuffer(U32 *dst_frame_buffer, S32 dst_x, S32 dst_y, Bitmap src_bitmap, Rectangle src_rectangle) {

	Rectangle frame_buffer = { 0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT };
	Rectangle dst_rectangle = { dst_x, dst_y, src_rectangle.width, src_rectangle.height };
	Rectangle clipped = RectangleIntersection(frame_buffer, dst_rectangle);

	const U32 height = clipped.height;
	const U32 width = clipped.width;

	S32 clip_src_x = src_rectangle.x + clipped.x - dst_x;
	S32 clip_src_y = src_rectangle.y + clipped.y - dst_y;

	for (U32 y = 0; y < height; ++y) {
		U32 *dst_row = &dst_frame_buffer[(clipped.y + y) * FRAME_BUFFER_WIDTH + clipped.x];
		U32 *src_row = &src_bitmap.pixels[(clip_src_y + y)*src_bitmap.width + clip_src_x];

		for (U32 x = 0; x < width; ++x) {
			U32 pixel = src_row[x];
			U32 alpha = pixel >> 24u;
			if (alpha != 0) {
				dst_row[x] = pixel | 0xFF000000;
			}
		}
	}
}

static void BlitColorRectangleToFramebuffer(U32 *dst_frame_buffer, Rectangle dst_rect, U32 color) {
	Rectangle frame_buffer = { 0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT };
	Rectangle clipped = RectangleIntersection(frame_buffer, dst_rect);

	const U32 height = clipped.height;
	const U32 width = clipped.width;

	for (U32 y = 0; y < height; ++y) {
		U32 *dst_row = &dst_frame_buffer[(clipped.y + y)*FRAME_BUFFER_WIDTH + clipped.x];
		for (U32 x = 0; x < width; ++x) {
			dst_row[x] = color;
		}
	}
}

static void BlitBitmapRectangleToFramebufferReversedX(U32 *dst_frame_buffer, S32 dst_x, S32 dst_y, Bitmap src_bitmap, Rectangle src_rectangle) {

	Rectangle frame_buffer = { 0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT };
	Rectangle dst_rectangle = { dst_x, dst_y, src_rectangle.width, src_rectangle.height };
	Rectangle clipped = RectangleIntersection(frame_buffer, dst_rectangle);

	const U32 height = clipped.height;
	const U32 width = clipped.width;

	S32 cut_left = clipped.x - dst_x;
	S32 clip_src_y = src_rectangle.y + clipped.y - dst_y;
	S32 src_start_x = src_rectangle.x + (S32)src_rectangle.width - 1 - cut_left;

	for (U32 y = 0; y < height; ++y) {
		U32 *dst_row = &dst_frame_buffer[(clipped.y + y) * FRAME_BUFFER_WIDTH + clipped.x];
		U32 *src_row = &src_bitmap.pixels[(clip_src_y + y)*src_bitmap.width + src_start_x];

		for (U32 x = 0; x < width; ++x) {
			U32 pixel = *(src_row - x);
			U32 alpha = pixel >> 24u;
			if (alpha != 0) {
				dst_row[x] = pixel | 0xFF000000;
			}
		}
	}
}

typedef struct {
	U32 animation_render_index;
	U32 global_animation_tick;
	Bool animation_ended;
	Bool animation_will_end_next_tick;
} AnimationState;

static AnimationState AdvanceBasicAnimation(BasicAnimation animation, U32 frame_delay, U32 animation_tick) {

	U32 animation_index = (animation_tick / frame_delay) % animation.frame_count;

	AnimationState result = {
		.animation_render_index = animation_index,
		.global_animation_tick = animation_tick + 1,
	};

	U32 animation_progress = animation_tick % (animation.frame_count * frame_delay);
	if (animation_tick != 0 && animation_progress == 0) {
		result.animation_ended = True;
	}
	if (animation_progress == animation.frame_count * frame_delay - 1) {
		result.animation_will_end_next_tick = True;
	}
	return result;
}

static void DisplayAnimationFrame(U32 *frame_buffer, U32 *x_offsets, Bitmap spriteset, U32 character_index, U32 animation_render_index, S32 x, S32 y) {

	Rectangle sprite_rect = {
		.x = x_offsets[animation_render_index],
		.y = 16 + 48*character_index,
		.width = 32,
		.height = 32
	};

	BlitBitmapRectangleToFramebuffer(frame_buffer, x, y, spriteset, sprite_rect);
}

static void AdvanceAndDisplayPlayerAnimationWalkCycle(U32 *frame_buffer, WalkAnimation *walk_animations, Bitmap spriteset, U32 character_index, U32 *animation_frame, Direction animation_direction, S32 x_movement, S32 y_movement, S32 previous_x_movement, S32 previous_y_movement, S32 player_x, S32 player_y, U32 animation_frame_delay) {

	WalkAnimation animation = walk_animations[animation_direction];

	U32 animation_length = animation.frame_count;

	Bool is_moving = x_movement != 0 || y_movement != 0;
	Bool started_moving = is_moving && (previous_x_movement == 0 && previous_y_movement == 0);

	if (started_moving) {
		*animation_frame += animation.start_offset * animation_frame_delay;
	}

	U32 animation_index = (*animation_frame / animation_frame_delay) % animation_length;
	Bool animation_finished = animation_index == 0 || animation_index == animation.neutral_frame;

	if (is_moving) {
		*animation_frame += 1;
	} else if (!animation_finished) {
		U32 behind = (animation_index <= animation.neutral_frame) ? 0 : animation.neutral_frame;
		U32 ahead = (animation_index < animation.neutral_frame) ? animation.neutral_frame : animation_length;

		Bool should_reverse = (animation_index - behind) <= (ahead - animation_index);

		if (should_reverse) {
			*animation_frame -= 1;
			U32 next_animation_index = (*animation_frame / animation_frame_delay) % animation_length;
			if (next_animation_index == 0) {
				*animation_frame = animation.neutral_frame * animation_frame_delay;
			}

			if (next_animation_index == animation.neutral_frame) {
				*animation_frame = 0;
			}
		} else {
			*animation_frame += 1;
		}
	}

	Rectangle sprite_rect = {
		.x = animation.x_offsets[animation_index],
		.y = 16 + 48*character_index,
		.width = 32,
		.height = 32
	};

	BlitBitmapRectangleToFramebuffer(frame_buffer, player_x, player_y, spriteset, sprite_rect);
}

static void SetupDeathAnimation(BasicAnimation *death_animation, U32 *animation_x_offsets) {

	death_animation->x_offsets = animation_x_offsets;
	death_animation->frame_count = 9;

	for (U32 i = 0; i < 9; ++i) {
		animation_x_offsets[i] = 16 + (i+29)*48;
	}
}

static U32 SetupWalkAnimations(WalkAnimation *walk_animations, U32 *animation_x_offsets) {

	U32 animation_push_index = 0;

	{
		WalkAnimation down_animation = {
			.x_offsets = animation_x_offsets + animation_push_index,
			.frame_count = 0,
			.neutral_frame = 6,
			.start_offset = 1,
		};

		U32 animation_push_index_start = animation_push_index;

		for (U32 i = 0; i < 4; ++i) {
			animation_x_offsets[animation_push_index] = 16 + i*48;
			animation_push_index += 1;
		}

		for (S32 i = 2; i >= 0; --i) {
			animation_x_offsets[animation_push_index] = 16 + i*48;
			animation_push_index += 1;
		}

		for (U32 i = 4; i <= 6; ++i) {
			animation_x_offsets[animation_push_index] = 16 + i*48;
			animation_push_index += 1;
		}
		for (U32 i = 5; i >= 4; --i) {
			animation_x_offsets[animation_push_index] = 16 + i*48;
			animation_push_index += 1;
		}

		down_animation.frame_count = animation_push_index - animation_push_index_start;
		walk_animations[DIRECTION_DOWN] = down_animation;
	}

	{
		WalkAnimation up_animation = {
			.x_offsets = animation_x_offsets + animation_push_index,
			.frame_count = 0,
			.start_offset = 1,
			.neutral_frame = 6,
		};

		U32 animation_push_index_start = animation_push_index;

		for (U32 i = 21; i < 21+4; ++i) {
			animation_x_offsets[animation_push_index] = 16+i*48;
			animation_push_index += 1;
		}

		for (U32 i = 21+2; i >= 21; --i) {
			animation_x_offsets[animation_push_index] = 16+i*48;
			animation_push_index += 1;
		}

		for (U32 i = 25; i < 25+2; ++i) {
			animation_x_offsets[animation_push_index] = 16+i*48;
			animation_push_index += 1;
		}

		for (U32 i = 25+1; i >= 25; --i) {
			animation_x_offsets[animation_push_index] = 16+i*48;
			animation_push_index += 1;
		}

		up_animation.frame_count = animation_push_index - animation_push_index_start;
		walk_animations[DIRECTION_UP] = up_animation;
	}

	{
		WalkAnimation right_animation = {
			.x_offsets = animation_x_offsets + animation_push_index,
			.frame_count = 0,
			.neutral_frame = 6,
		};

		U32 animation_push_index_start = animation_push_index;

		for (U32 i = 7; i <= 10; ++i) {
			animation_x_offsets[animation_push_index] = 16+i*48;
			animation_push_index += 1;
		}

		for (U32 i = 9; i >= 7; --i) {
			animation_x_offsets[animation_push_index] = 16+i*48;
			animation_push_index += 1;
		}

		for (U32 i = 11; i <= 13; ++i) {
			animation_x_offsets[animation_push_index] = 16+i*48;
			animation_push_index += 1;
		}

		for (U32 i = 12; i >= 11; --i) {
			animation_x_offsets[animation_push_index] = 16+i*48;
			animation_push_index += 1;
		}

		right_animation.frame_count = animation_push_index - animation_push_index_start;
		walk_animations[DIRECTION_RIGHT] = right_animation;
	}

	{
		WalkAnimation left_animation = {
			.x_offsets = animation_x_offsets + animation_push_index,
			.frame_count = 0,
			.neutral_frame = 6,
		};

		U32 animation_push_index_start = animation_push_index;

		for (U32 i = 14; i <= 17; ++i) {
			animation_x_offsets[animation_push_index] = 16+i*48;
			animation_push_index += 1;
		}

		for (U32 i = 16; i >= 14; --i) {
			animation_x_offsets[animation_push_index] = 16+i*48;
			animation_push_index += 1;
		}

		for (U32 i = 18; i <= 20; ++i) {
			animation_x_offsets[animation_push_index] = 16+i*48;
			animation_push_index += 1;
		}

		for (U32 i = 19; i >= 18; --i) {
			animation_x_offsets[animation_push_index] = 16+i*48;
			animation_push_index += 1;
		}

		left_animation.frame_count = animation_push_index - animation_push_index_start;
		walk_animations[DIRECTION_LEFT] = left_animation;
	}

	return animation_push_index;
}
