
#include "game.h"
#include <Tilengine.h>

static uint64_t g_frame_index;
static TLN_Spriteset character;

static void raster_effect(int line)
{
	int v = (line + g_frame_index) & 0xFF;
	TLN_SetBGColor((uint8_t)v,
	               (uint8_t)(line * 255 / FRAME_BUFFER_HEIGHT),
	               (uint8_t)(255 - v));
}

void GameInit(void *frame_buffer, uint32_t pitch) {
	TLN_Init(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 1, 0, 0);

	TLN_SetRenderTarget((uint8_t *)frame_buffer, (int)pitch);
	TLN_SetRasterCallback(raster_effect);

	TLN_Bitmap character_bitmap = TLN_LoadBitmap("assets/spriteset.png");

}

void GameFrame(uint64_t frame_index) {
	g_frame_index = frame_index;
	TLN_UpdateFrame(frame_index);
}

void GameExit(void) {

}
