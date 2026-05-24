/*
* Tilengine - The 2D retro graphics engine with raster effects
* Copyright (C) 2015-2019 Marc Palacios Domenech <mailto:megamarc@hotmail.com>
* All rights reserved
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
* */

#include <stdlib.h>
#include <string.h>
#include "Tilengine.h"
#include "LoadFile.h"
#ifndef TLN_EXCLUDE_PNG
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"
#endif
#include "DIB.h"
#include "Bitmap.h"
#include "Palette.h"

#ifndef TLN_EXCLUDE_PNG
static TLN_Bitmap LoadPNG (const char* filename);
#endif
static TLN_Bitmap LoadBMP (const char* filename);

typedef struct
{
	uint32_t items[255];
	uint16_t count;
}
Set;

static void set_init(Set* set)
{
	set->count = 0;
}

static int set_get_index(Set* set, uint32_t value)
{
	int c;
	for (c = 0; c < set->count; c += 1)
	{
		if (set->items[c] == value)
			return c;
	}
	return -1;
}

static bool set_add(Set* set, uint32_t value)
{
	int index;
	if (set->count == 255)
		return false;

	index = set_get_index(set, value);
	if (index == -1)
	{
		set->items[set->count] = value;
		set->count += 1;
	}
	return true;
}

static TLN_Palette BuildPaletteFromSet(Set* colors)
{
	int c;
	RGBQUAD srccolor;

	TLN_Palette palette = TLN_CreatePalette(colors->count + 1);
	uint32_t* dstcolor = (uint32_t*)palette->data;

	*dstcolor = 0xFFFF00FF;	/* pink */
	dstcolor += 1;
	for (c = 0; c < colors->count; c += 1)
	{
		srccolor.value = colors->items[c];
		*dstcolor = PackRGB32(srccolor.b, srccolor.g, srccolor.r);
		dstcolor += 1;
	}
	return palette;
}

static TLN_Bitmap Convert24ToIndexed(TLN_Bitmap source)
{
	TLN_Bitmap bitmap = NULL;
	Set colors;
	int x, y;
	uint8_t* srcscan;
	uint8_t* dstscan;

	/* count unique colors up to 255 */
	set_init(&colors);
	srcscan = source->data;
	for (y = 0; y < source->height; y += 1)
	{
		RGBTRIPLE* color = (RGBTRIPLE*)srcscan;
		for (x = 0; x < source->width; x += 1)
		{
			uint32_t value = PackRGB32(color->r, color->g, color->b);
			if (!set_add(&colors, value))
				return NULL;
			color += 1;
		}
		srcscan += source->pitch;
	}

	/* create new bitmap at 8 bpp */
	bitmap = TLN_CreateBitmap(source->width, source->height, 8);
	srcscan = source->data;
	dstscan = bitmap->data;

	/* set colors with palette indexes */
	for (y = 0; y < source->height; y += 1)
	{
		RGBTRIPLE* srccolor = (RGBTRIPLE*)srcscan;
		uint8_t* dstcolor = dstscan;
		for (x = 0; x < source->width; x += 1)
		{
			uint32_t value = PackRGB32(srccolor->r, srccolor->g, srccolor->b);
			*dstcolor = set_get_index(&colors, value) + 1;
			srccolor += 1;
			dstcolor += 1;
		}

		srcscan += source->pitch;
		dstscan += bitmap->pitch;
	}

	/* create attached palette and set actual colors */
	bitmap->palette = BuildPaletteFromSet(&colors);

	return bitmap;
}

static TLN_Bitmap Convert32ToIndexed(TLN_Bitmap source)
{
	TLN_Bitmap bitmap = NULL;
	Set colors;
	int x,y;
	uint8_t* srcscan;
	uint8_t* dstscan;

	/* count unique colors up to 255 */
	set_init(&colors);
	srcscan = source->data;
	for (y = 0; y < source->height; y += 1)
	{
		RGBQUAD* color = (RGBQUAD*)srcscan;
		for (x = 0; x < source->width; x += 1)
		{
			if (color->a >= 128)
			{
				color->a = 255;
				if (!set_add(&colors, color->value))
					return NULL;
			}
			else
				color->a = 0;
			color += 1;
		}
		srcscan += source->pitch;
	}

	/* create new bitmap at 8 bpp */
	bitmap = TLN_CreateBitmap(source->width, source->height, 8);
	srcscan = source->data;
	dstscan = bitmap->data;
	
	/* set colors with palette indexes */
	for (y = 0; y < source->height; y += 1)
	{
		RGBQUAD* srccolor = (RGBQUAD*)srcscan;
		uint8_t* dstcolor = dstscan;
		for (x = 0; x < source->width; x += 1)
		{
			if (srccolor->a == 0)
				*dstcolor = 0;
			else
				*dstcolor = set_get_index(&colors, srccolor->value) + 1;
			srccolor += 1;
			dstcolor += 1;
		}

		srcscan += source->pitch;
		dstscan += bitmap->pitch;
	}

	/* create attached palette and set actual colors */
	bitmap->palette = BuildPaletteFromSet(&colors);

	return bitmap;
}

/*!
 * \brief
 * Load image file (8-bit BMP or PNG)
 * 
 * \param filename
 * File name with the image
 * 
 * \returns
 * Handler to the loaded image or NULL if error
 * 
 * \see
 * TLN_DeleteBitmap()
 */
TLN_Bitmap TLN_LoadBitmap (const char* filename)
{
	TLN_Bitmap bitmap;

	if (!CheckFile (filename))
	{
		TLN_SetLastError (TLN_ERR_FILE_NOT_FOUND);
		return NULL;
	}

	/* try png, else bmp*/
	bitmap = NULL;
#ifndef TLN_EXCLUDE_PNG
	bitmap = LoadPNG (filename);
#endif
	if (bitmap == NULL)
		bitmap = LoadBMP (filename);

	/* bitmap loaded */
	if (bitmap)
	{
		/* accept only 8 bpp */
		int bpp = TLN_GetBitmapDepth (bitmap);
		if (bpp == 24)
		{
			TLN_Bitmap indexed = Convert24ToIndexed(bitmap);
			if (indexed != NULL)
			{
				TLN_DeleteBitmap(bitmap);
				bitmap = indexed;
				bpp = 8;
			}
		}
		else if (bpp == 32)
		{
			TLN_Bitmap indexed = Convert32ToIndexed(bitmap);
			if (indexed != NULL)
			{
				TLN_DeleteBitmap(bitmap);
				bitmap = indexed;
				bpp = 8;
			}
		}

		if (bpp == 8)
			TLN_SetLastError (TLN_ERR_OK);
		else
		{
			TLN_DeleteBitmap (bitmap);
			bitmap = NULL;
		}
	}
	
	if (!bitmap)
		TLN_SetLastError (TLN_ERR_WRONG_FORMAT);

	return bitmap;
}

#ifndef TLN_EXCLUDE_PNG
/* Loads PNG using stb_image. Reads through Tilengine's LoadFile so that
   TLN_SetLoadPath and resource packs keep working, then decodes from memory
   forcing RGBA. The 32 bpp result is quantized to indexed by TLN_LoadBitmap,
   exactly as the previous libpng path produced. */
static TLN_Bitmap LoadPNG (const char* filename)
{
	TLN_Bitmap bitmap;
	uint8_t* filedata;
	uint8_t* pixels;
	ssize_t size = 0;
	int width, height, channels, y;

	filedata = (uint8_t*)LoadFile (filename, &size);
	if (!filedata)
		return NULL;

	pixels = stbi_load_from_memory (filedata, (int)size, &width, &height, &channels, 4);
	free (filedata);
	if (!pixels)
		return NULL;

	bitmap = TLN_CreateBitmap (width, height, 32);
	if (bitmap)
	{
		for (y = 0; y < height; y += 1)
			memcpy (TLN_GetBitmapPtr (bitmap, 0, y), pixels + y * width * 4, width * 4);
	}

	stbi_image_free (pixels);
	return bitmap;
}
#endif /* TLN_EXCLUDE_PNG */

/* loads BMP */
static TLN_Bitmap LoadBMP (const char* filename)
{
	BITMAPFILEHEADER bfh;
	BITMAPV5HEADER bv5;
	uint32_t StructSize;
	FILE* pf;
	TLN_Bitmap bitmap = NULL;
	unsigned int c;
	int pitch;

	/* open file */
	pf = FileOpen (filename);
	if (!pf)
		return NULL;

	/* read BMP header */
	fread (&bfh, sizeof(bfh), 1, pf);
	if (bfh.Type != 0x4D42)
	{
		FileClose (pf);
		return NULL;
	}

	/* load info structure */
	memset (&bv5, 0, sizeof(bv5));
	fread (&StructSize, 4, 1, pf);
	fseek (pf, sizeof(bfh), SEEK_SET);
	fread (&bv5, StructSize, 1, pf);

	/* create */
	bitmap = TLN_CreateBitmap (bv5.bV5Width, bv5.bV5Height, bv5.bV5BitCount);
	if (!bitmap)
	{
		FileClose (pf);
		return NULL;
	}

	/* load scanlines */
	pitch = TLN_GetBitmapPitch (bitmap);
	fseek (pf, bfh.OffsetData, SEEK_SET);
	for (c=0; c<bv5.bV5Height; c++)
	{
		uint8_t* line = TLN_GetBitmapPtr (bitmap, 0, bv5.bV5Height - c - 1);
		fread (line, pitch, 1, pf);
	}

	/* load palette */
	if (bv5.bV5BitCount == 8)
	{
		TLN_Palette palette;

		/* HACK: some editors don't set the bV5ClrUsed field, compute from size */
		if (bv5.bV5ClrUsed == 0)
			bv5.bV5ClrUsed = (bfh.OffsetData - sizeof(bfh) - bv5.bV5Size) / sizeof(RGBQUAD);

		fseek (pf, sizeof(BITMAPFILEHEADER) + bv5.bV5Size, SEEK_SET);
		palette = TLN_CreatePalette (bv5.bV5ClrUsed);
		for (c=0; c<(int)bv5.bV5ClrUsed; c++)
		{
			RGBQUAD color;
			fread (&color, sizeof(RGBQUAD), 1, pf);
			TLN_SetPaletteColor (palette, c, color.r, color.g, color.b);
		}
		TLN_SetBitmapPalette (bitmap, palette);
	}

	FileClose (pf);
	return bitmap;
}
