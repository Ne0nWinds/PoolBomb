
#include "game.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <string.h>

typedef enum Direction {
	DIRECTION_DOWN,
	DIRECTION_UP,
	DIRECTION_LEFT,
	DIRECTION_RIGHT,
	DIRECTION_COUNT,
} Direction;

typedef struct Rectangle {
	U32 x, y;
	U32 width, height;
} Rectangle;

typedef struct WalkAnimation {
	Rectangle *base_rect;
	U32 frame_count;
	U32 neutral_frame;
	Bool mirror_feet;
	Bool flip_x;
} WalkAnimation;

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

void BlitBitmapRectangleToFramebuffer(U32 *dst_frame_buffer, U32 dst_x, U32 dst_y, Bitmap src_bitmap, Rectangle src_rectangle) {

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

void BlitBitmapRectangleToFramebufferReversedX(U32 *dst_frame_buffer, U32 dst_x, U32 dst_y, Bitmap src_bitmap, Rectangle src_rectangle) {

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

static Bitmap base_spriteset;

#define MAX_RECT_COUNT (128)
static Rectangle g_animation_frames[MAX_RECT_COUNT];
static WalkAnimation g_walk_animations[DIRECTION_COUNT];

void GameInit(U32 *frame_buffer, uint32_t pitch) {

	base_spriteset = LoadBitmap("assets/spriteset.png");

	U32 animation_push_index = 0;

	{
		g_walk_animations[DIRECTION_DOWN] = (WalkAnimation){
			.base_rect = g_animation_frames + animation_push_index,
			.frame_count = 7,
			.neutral_frame = 7,
			.mirror_feet = True,
			.flip_x = False,
		};

		U32 animation_push_index_start = animation_push_index;

		for (U32 i = 0; i < 4; ++i) {
			g_animation_frames[animation_push_index].x =		8 + 48*i;
			g_animation_frames[animation_push_index].y =		8 + 48*0;
			g_animation_frames[animation_push_index].width =	48;
			g_animation_frames[animation_push_index].height =	48;
			animation_push_index += 1;
		}

		for (S32 i = 2; i > 0; --i) {
			g_animation_frames[animation_push_index].x =		8 + 48*i;
			g_animation_frames[animation_push_index].y =		8 + 48*0;
			g_animation_frames[animation_push_index].width =	48;
			g_animation_frames[animation_push_index].height =	48;
			animation_push_index += 1;
		}

		g_walk_animations[DIRECTION_DOWN].frame_count = animation_push_index - animation_push_index_start;
	}

	{
		g_walk_animations[DIRECTION_UP] = (WalkAnimation){
			.base_rect = g_animation_frames + animation_push_index,
			.neutral_frame = 7,
			.mirror_feet = True,
			.flip_x = False,
		};

		U32 animation_push_index_start = animation_push_index;

		for (U32 i = 0; i < 4; ++i) {
			g_animation_frames[animation_push_index].x =		8 + 48*i;
			g_animation_frames[animation_push_index].y =		8 + 48*3;
			g_animation_frames[animation_push_index].width =	48;
			g_animation_frames[animation_push_index].height =	48;
			animation_push_index += 1;
		}

		for (S32 i = 2; i > 0; --i) {
			g_animation_frames[animation_push_index].x =		8 + 48*i;
			g_animation_frames[animation_push_index].y =		8 + 48*3;
			g_animation_frames[animation_push_index].width =	48;
			g_animation_frames[animation_push_index].height =	48;
			animation_push_index += 1;
		}

		g_walk_animations[DIRECTION_UP].frame_count = animation_push_index - animation_push_index_start;
	}

	{
		g_walk_animations[DIRECTION_LEFT] = (WalkAnimation){
			.base_rect = g_animation_frames + animation_push_index,
			.neutral_frame = 7,
			.mirror_feet = False,
			.flip_x = True
		};

		U32 animation_push_index_start = animation_push_index;

		for (U32 i = 0; i < 4; ++i) {
			g_animation_frames[animation_push_index].x =		8 + 48*i;
			g_animation_frames[animation_push_index].y =		8 + 48*1;
			g_animation_frames[animation_push_index].width =	48;
			g_animation_frames[animation_push_index].height =	48;
			animation_push_index += 1;
		}

		for (S32 i = 2; i > 0; --i) {
			g_animation_frames[animation_push_index].x =		8 + 48*i;
			g_animation_frames[animation_push_index].y =		8 + 48*1;
			g_animation_frames[animation_push_index].width =	48;
			g_animation_frames[animation_push_index].height =	48;
			animation_push_index += 1;
		}

		for (U32 i = 0; i < 4; ++i) {
			g_animation_frames[animation_push_index].x =		8 + 48*i;
			g_animation_frames[animation_push_index].y =		8 + 48*2;
			g_animation_frames[animation_push_index].width =	48;
			g_animation_frames[animation_push_index].height =	48;
			animation_push_index += 1;
		}

		for (S32 i = 2; i > 0; --i) {
			g_animation_frames[animation_push_index].x =		8 + 48*i;
			g_animation_frames[animation_push_index].y =		8 + 48*2;
			g_animation_frames[animation_push_index].width =	48;
			g_animation_frames[animation_push_index].height =	48;
			animation_push_index += 1;
		}

		g_walk_animations[DIRECTION_LEFT].frame_count = animation_push_index - animation_push_index_start;

	}

	{
		g_walk_animations[DIRECTION_RIGHT] = (WalkAnimation){
			.base_rect = g_animation_frames + animation_push_index,
			.neutral_frame = 7,
			.mirror_feet = False,
			.flip_x = False
		};

		U32 animation_push_index_start = animation_push_index;

		for (U32 i = 0; i < 4; ++i) {
			g_animation_frames[animation_push_index].x =		8 + 48*i;
			g_animation_frames[animation_push_index].y =		8 + 48*1;
			g_animation_frames[animation_push_index].width =	48;
			g_animation_frames[animation_push_index].height =	48;
			animation_push_index += 1;
		}

		for (S32 i = 2; i > 0; --i) {
			g_animation_frames[animation_push_index].x =		8 + 48*i;
			g_animation_frames[animation_push_index].y =		8 + 48*1;
			g_animation_frames[animation_push_index].width =	48;
			g_animation_frames[animation_push_index].height =	48;
			animation_push_index += 1;
		}

		for (U32 i = 0; i < 4; ++i) {
			g_animation_frames[animation_push_index].x =		8 + 48*i;
			g_animation_frames[animation_push_index].y =		8 + 48*2;
			g_animation_frames[animation_push_index].width =	48;
			g_animation_frames[animation_push_index].height =	48;
			animation_push_index += 1;
		}

		for (S32 i = 2; i > 0; --i) {
			g_animation_frames[animation_push_index].x =		8 + 48*i;
			g_animation_frames[animation_push_index].y =		8 + 48*2;
			g_animation_frames[animation_push_index].width =	48;
			g_animation_frames[animation_push_index].height =	48;
			animation_push_index += 1;
		}

		g_walk_animations[DIRECTION_RIGHT].frame_count = animation_push_index - animation_push_index_start;

	}
}

void DisplayWalkAnimationSheet(U32 *frame_buffer, WalkAnimation *animation) {
	U32 frame_count = animation->frame_count;
	for (U32 i = 0; i < frame_count; ++i) {
		Rectangle *rect = animation->base_rect + i;

		U32 x_pos = i%4 * 48;
		U32 y_pos = i/4 * 48;
		BlitBitmapRectangleToFramebuffer(frame_buffer, x_pos, y_pos, base_spriteset, *rect);
	}
}

void GameFrame(U32 *frame_buffer, uint64_t frame_index, GameInput *player_inputs, U32 player_count) {

	memset(frame_buffer, 0, FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT * 4u);

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

	WalkAnimation animation = g_walk_animations[animation_direction];

	U32 animation_length = animation.frame_count;
	if (animation.mirror_feet) {
		animation_length *= 2;
	}

	Bool is_moving = x_movement != 0 || y_movement != 0;
	U32 animation_index = (player_animation_frame / 4) % animation_length;
	Bool animation_not_finished = animation_index != 0 && animation_index != animation.neutral_frame;

	if (is_moving || animation_not_finished) {
		player_animation_frame += 1;
		animation_index = (player_animation_frame / 4) % animation_length;
	}

	Bool mirror_feet = animation_index >= animation.frame_count;

	Bool should_flip = animation.flip_x ^ mirror_feet;
	if (mirror_feet) {
		animation_index -= animation.frame_count;
	}
	Rectangle src_rectangle = animation.base_rect[animation_index];

	if (should_flip) {
		BlitBitmapRectangleToFramebufferReversedX(frame_buffer, 0, 0, base_spriteset, src_rectangle);
	} else {
		BlitBitmapRectangleToFramebuffer(frame_buffer, 0, 0, base_spriteset, src_rectangle);
	}
}

void GameExit(void) {

}
