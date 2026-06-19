
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

typedef struct WalkAnimation {
	U32 *x_offsets;
	U32 frame_count;
	U32 neutral_frame;
	Bool mirror_feet;
	Bool flip_x;
} WalkAnimation;

static void BlitBitmapRectangleToFramebuffer(U32 *dst_frame_buffer, U32 dst_x, U32 dst_y, Bitmap src_bitmap, Rectangle src_rectangle) {

	const U32 height = src_rectangle.height;
	const U32 width = src_rectangle.width;

	for (U32 y = 0; y < height; ++y) {
		U32 *frame_buffer_line = &dst_frame_buffer[(dst_y + y) * FRAME_BUFFER_WIDTH + dst_x];
		U32 *bitmap_line = &src_bitmap.pixels[(src_rectangle.y + y) * src_bitmap.width + src_rectangle.x];

		for (U32 x = 0; x < width; ++x) {
			U32 pixel = bitmap_line[x];
			U32 alpha = pixel >> 24;
			if (alpha != 0) {
				frame_buffer_line[x] = pixel | 0xFF000000;
			}
		}
	}
}

static void BlitBitmapRectangleToFramebufferReversedX(U32 *dst_frame_buffer, U32 dst_x, U32 dst_y, Bitmap src_bitmap, Rectangle src_rectangle) {

	const U32 height = src_rectangle.height;
	const U32 width = src_rectangle.width;

	for (U32 y = 0; y < height; ++y) {
		U32 *frame_buffer_line = &dst_frame_buffer[(dst_y + y) * FRAME_BUFFER_WIDTH + dst_x];
		U32 *bitmap_line = &src_bitmap.pixels[(src_rectangle.y + y) * src_bitmap.width + (src_rectangle.x + width - 1)];

		for (U32 x = 0; x < width; ++x) {
			U32 pixel = *(bitmap_line - x);
			U32 alpha = pixel >> 24;
			if (alpha != 0) {
				frame_buffer_line[x] = pixel | 0xFF000000;
			}
		}
	}
}


static void AdvanceAndDisplayPlayerAnimationWalkCycle(U32 *frame_buffer, WalkAnimation *walk_animations, Bitmap spriteset, U32 character_index, U32 *animation_frame, Direction animation_direction, S32 x_movement, S32 y_movement, U32 animation_frame_delay) {

	WalkAnimation animation = walk_animations[animation_direction];

	U32 animation_length = animation.frame_count;
	if (animation.mirror_feet) {
		animation_length *= 2;
	}

	Bool is_moving = x_movement != 0 || y_movement != 0;
	U32 animation_index = (*animation_frame / animation_frame_delay) % animation_length;
	Bool animation_not_finished = animation_index != 0 && animation_index != animation.neutral_frame;

	if (is_moving || animation_not_finished) {
		*animation_frame += 1;
	}

	Bool mirror_feet = animation_index >= animation.frame_count;

	Bool should_flip = animation.flip_x ^ mirror_feet;
	if (mirror_feet) {
		animation_index -= animation.frame_count;
	}

	Rectangle sprite_rect = {
		.x = animation.x_offsets[animation_index],
		.y = 16 + 48*character_index,
		.width = 32,
		.height = 32
	};

	if (should_flip) {
		BlitBitmapRectangleToFramebufferReversedX(frame_buffer, 0, 0, spriteset, sprite_rect);
	} else {
		BlitBitmapRectangleToFramebuffer(frame_buffer, 0, 0, spriteset, sprite_rect);
	}
}

static void SetupWalkAnimations(WalkAnimation *walk_animations, U32 *animation_x_offsets) {

	U32 animation_push_index = 0;

	{
		WalkAnimation down_animation = {
			.x_offsets = animation_x_offsets + animation_push_index,
			.frame_count = 0,
			.neutral_frame = 7,
			.mirror_feet = False,
			.flip_x = False
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
			.neutral_frame = 7,
			.mirror_feet = True,
			.flip_x = False
		};

		U32 animation_push_index_start = animation_push_index;

		for (U32 i = 14; i < 14+4; ++i) {
			animation_x_offsets[animation_push_index] = 16+i*48;
			animation_push_index += 1;
		}

		for (U32 i = 16; i >= 14; --i) {
			animation_x_offsets[animation_push_index] = 16+i*48;
			animation_push_index += 1;
		}

		up_animation.frame_count = animation_push_index - animation_push_index_start;
		walk_animations[DIRECTION_UP] = up_animation;
	}

	{
		WalkAnimation left_animation = {
			.x_offsets = animation_x_offsets + animation_push_index,
			.frame_count = 0,
			.neutral_frame = 6,
			.mirror_feet = False,
			.flip_x = True
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

		left_animation.frame_count = animation_push_index - animation_push_index_start;
		walk_animations[DIRECTION_LEFT] = left_animation;
	}
	{
		WalkAnimation right_animation = walk_animations[DIRECTION_LEFT];
		right_animation.flip_x = False;
		walk_animations[DIRECTION_RIGHT] = right_animation;
	}
}
