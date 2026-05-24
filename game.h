
#ifndef GAME_H
#define GAME_H

#define FRAME_BUFFER_WIDTH 256
#define FRAME_BUFFER_HEIGHT 224

#include <stdint.h>

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int8_t S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;

typedef float F32;
typedef double F64;

void GameInit(void *framebuffer, uint32_t pitch);

void GameFrame(uint64_t frame_index);

void GameExit(void);

#endif
