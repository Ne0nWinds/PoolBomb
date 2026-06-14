
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

#define MAX_RECT_COUNT (7*2 + 12*2)
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

		for (U32 i = 0; i < 4; ++i) {
			g_animation_frames[animation_push_index].x =		8 + 48*i;
			g_animation_frames[animation_push_index].y =		8 + 48*0;
			g_animation_frames[animation_push_index].width =	48;
			g_animation_frames[animation_push_index].height =	48;
			animation_push_index += 1;
		}

		for (S32 i = 3; i > 0; --i) {
			g_animation_frames[animation_push_index].x =		8 + 48*i;
			g_animation_frames[animation_push_index].y =		8 + 48*0;
			g_animation_frames[animation_push_index].width =	48;
			g_animation_frames[animation_push_index].height =	48;
			animation_push_index += 1;
		}
	}

	{
		g_walk_animations[DIRECTION_UP] = (WalkAnimation){
			.base_rect = g_animation_frames + animation_push_index,
			.frame_count = 7,
			.neutral_frame = 7,
			.mirror_feet = True,
			.flip_x = False,
		};

		for (U32 i = 0; i < 4; ++i) {
			g_animation_frames[animation_push_index].x =		8 + 48*i;
			g_animation_frames[animation_push_index].y =		8 + 48*3;
			g_animation_frames[animation_push_index].width =	48;
			g_animation_frames[animation_push_index].height =	48;
			animation_push_index += 1;
		}

		for (S32 i = 3; i > 0; --i) {
			g_animation_frames[animation_push_index].x =		8 + 48*i;
			g_animation_frames[animation_push_index].y =		8 + 48*3;
			g_animation_frames[animation_push_index].width =	48;
			g_animation_frames[animation_push_index].height =	48;
			animation_push_index += 1;
		}
	}

#if 0
	TLN_Init(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 1, 1, 1);

	TLN_SetRenderTarget((uint8_t *)frame_buffer, (int)pitch);
	TLN_SetBGColor(0x55, 0x55, 0x55);

	{
		g_walk_animations[DIRECTION_UP] =    (WalkAnimation){ 7, true, false, 7 };
		g_walk_animations[DIRECTION_LEFT] =  (WalkAnimation){ 12, false, false, 0 };
		g_walk_animations[DIRECTION_RIGHT] = (WalkAnimation){ 12, true, false, 7 };
	}

	TLN_Bitmap bitmap = TLN_LoadBitmap("assets/spriteset.png");

	{
		TLN_SpriteData frames[7];

		U32 i = 0;
		for (; i < 4; ++i) {
			snprintf(frames[i].name, sizeof(frames[i].name), "walkdown%d", i + 1);
			frames[i].x = 8 + 48*i;
			frames[i].y = 8 + 48*0;
			frames[i].w = 48;
			frames[i].h = 48;
		}

		for (S32 j = 3; j > 0; --j, ++i) {
			snprintf(frames[i].name, sizeof(frames[i].name), "walkdown%d", i + 1);
			frames[i].x = 8 + 48*j;
			frames[i].y = 8 + 48*0;
			frames[i].w = 48;
			frames[i].h = 48;
		}

		TLN_Spriteset character_animation = TLN_CreateSpriteset(bitmap, frames, ArrayLength(frames));
		g_spritesets[DIRECTION_DOWN] = character_animation;
	}

	{
		TLN_SpriteData frames[7];

		U32 i = 0;
		for (; i < 4; ++i) {
			snprintf(frames[i].name, sizeof(frames[i].name), "walkup%d", i + 1);
			frames[i].x = 8 + 48*i;
			frames[i].y = 8 + 48*3;
			frames[i].w = 48;
			frames[i].h = 48;
		}

		for (S32 j = 3; j > 0; --j, ++i) {
			snprintf(frames[i].name, sizeof(frames[i].name), "walkup%d", i + 1);
			frames[i].x = 8 + 48*j;
			frames[i].y = 8 + 48*3;
			frames[i].w = 48;
			frames[i].h = 48;
		}

		TLN_Spriteset character_animation = TLN_CreateSpriteset(bitmap, frames, ArrayLength(frames));
		g_spritesets[DIRECTION_UP] = character_animation;
	}

	{
		TLN_SpriteData frames[12];

		U32 i = 0;
		for (; i < 4; ++i) {
			snprintf(frames[i].name, sizeof(frames[i].name), "walkleft%d", i + 1);
			frames[i].x = 8 + 48*i;
			frames[i].y = 8 + 48*1;
			frames[i].w = 48;
			frames[i].h = 48;
		}
		for (S32 j = 2; j >= 0; --j, ++i) {
			snprintf(frames[i].name, sizeof(frames[i].name), "walkleft%d", i + 1);
			frames[i].x = 8 + 48*j;
			frames[i].y = 8 + 48*1;
			frames[i].w = 48;
			frames[i].h = 48;
		}
		for (U32 j = 0; j < 3; ++j, ++i) {
			snprintf(frames[i].name, sizeof(frames[i].name), "walkleft%d", i + 1);
			frames[i].x = 8 + 48*j;
			frames[i].y = 8 + 48*2;
			frames[i].w = 48;
			frames[i].h = 48;
		}
		for (S32 j = 1; j >= 0; --j, ++i) {
			snprintf(frames[i].name, sizeof(frames[i].name), "walkleft%d", i + 1);
			frames[i].x = 8 + 48*j;
			frames[i].y = 8 + 48*2;
			frames[i].w = 48;
			frames[i].h = 48;
		}
	}

	TLN_ConfigSprite(0, g_spritesets[0], 0);
	TLN_SetSpritePosition(0, 128, 128);
#endif
}

void GameFrame(U32 *frame_buffer, uint64_t frame_index, GameInput *player_inputs, U32 player_count) {

	memset(frame_buffer, 0, FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT * 4u);

	Bool down = IsButtonDown(player_inputs[0], BUTTON_DOWN);
	Bool up = IsButtonDown(player_inputs[0], BUTTON_UP);
#if 0
	Bool left = IsButtonDown(player_inputs[0], BUTTON_LEFT);
	Bool right = IsButtonDown(player_inputs[0], BUTTON_RIGHT);
#else
	Bool left = False;
	Bool right = False;
#endif

	S32 y_movement = (S32)up - (S32)down;
	S32 x_movement = (S32)right - (S32)left;

	static Direction animation_direction = DIRECTION_DOWN;

	if (y_movement != 0) {
		if (y_movement > 0) {
			animation_direction = DIRECTION_UP;
		} else {
			animation_direction = DIRECTION_DOWN;
		}
	} else if (x_movement != 0) {
		if (x_movement > 0) {
			animation_direction = DIRECTION_RIGHT;
		} else {
			animation_direction = DIRECTION_LEFT;
		}
	}

	static U32 player_animation_frame = 0;

	WalkAnimation animation = g_walk_animations[animation_direction];

	Bool is_moving = x_movement != 0 || y_movement != 0;
	if (is_moving) {
		player_animation_frame += 1;
	}

	U32 animation_length = animation.frame_count;
	if (animation.mirror_feet) {
		animation_length *= 2;
	}
	U32 animation_index = (player_animation_frame / 4) % animation_length;
	Bool mirror_feet = animation_index >= animation.frame_count;

	Bool animation_not_finished = animation_index != 0 && animation_index != animation.neutral_frame;
	if (!is_moving && animation_not_finished) {
		player_animation_frame += 1;
		animation_index = (player_animation_frame / 4) % animation_length;
		animation_not_finished = animation_index != 0 && animation_index != animation.neutral_frame;
		mirror_feet = animation_index >= animation.frame_count;
	}

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
