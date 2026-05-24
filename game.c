
#include "game.h"
#include <Tilengine.h>

static uint64_t g_frame_index;

static void raster_effect(int line)
{
	int v = (line + g_frame_index) & 0xFF;
	TLN_SetBGColor((uint8_t)v,
	               (uint8_t)(line * 255 / FRAME_BUFFER_HEIGHT),
	               (uint8_t)(255 - v));
}

void GameInit(void *frame_buffer, uint32_t pitch) {
	TLN_Init(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 1, 1, 1);

	TLN_SetRenderTarget((uint8_t *)frame_buffer, (int)pitch);
	// TLN_SetRasterCallback(raster_effect);
	TLN_SetBGColor(0x55, 0x55, 0x55);

	TLN_Bitmap bitmap = TLN_LoadBitmap("assets/spriteset.png");
	S32 columns = TLN_GetBitmapWidth(bitmap) / 64;

	TLN_SpriteData frames[9];
	U32 i = 0;
	for (; i < 5; ++i) {
		snprintf(frames[i].name, sizeof(frames[i].name), "walk%d", i + 1);
		frames[i].x = 8 + 48*i;
		frames[i].y = 8 + 48*4;
		frames[i].w = 48;
		frames[i].h = 48;
	}
	for (U32 j = 0; j < 4; ++j, ++i) {
		snprintf(frames[i].name, sizeof(frames[i].name), "walk%d", i + 1);
		frames[i].x = 8 + 48*j;
		frames[i].y = 8 + 48*5;
		frames[i].w = 48;
		frames[i].h = 48;
	}
	TLN_Spriteset character_animation = TLN_CreateSpriteset(bitmap, frames, 9);

	TLN_Sequence walk = TLN_CreateSpriteSequence(NULL, character_animation, "walk", 8);
	TLN_ConfigSprite(0, character_animation, 0);
	TLN_SetSpritePosition(0, 128, 128);
	TLN_SetSpriteAnimation(0, walk, 1);
}

void GameFrame(uint64_t frame_index) {
	g_frame_index = frame_index;
	TLN_UpdateFrame(frame_index);
}

void GameExit(void) {

}
