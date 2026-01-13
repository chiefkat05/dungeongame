#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>
#include <stdio.h>

#include "def.h"

typedef struct
{
    void *memory;
    u64 cursor, memsize;
} Arena;

static void *memGrab(Arena *arena, u64 bytes)
{
    if (arena->cursor + bytes > arena->memsize)
    {
        printf("%p ran out of memory\n", arena);
        exit(1);
    }
    void *out = (arena->memory + arena->cursor);
    arena->cursor += bytes;

    return out;
}
static void memoryInit(Arena *arena, u64 bytes)
{
    *arena = (Arena){};
    arena->memory = malloc(bytes);
    arena->memsize = bytes;
    arena->cursor = 0;
}
static void memoryClear(Arena *arena)
{
    free(arena->memory);
}

static void copyMemory(void *dest, void *src, u64 bytes)
{
    u32 i;
    for (i = 0; i < bytes; ++i)
    { ((u8 *)dest)[i] = ((u8 *)src)[i]; }
}
static void copyMemoryReverse(void *dest, void *src, u64 bytes)
{
    u32 i;
    for (i = 0; i < bytes; ++i)
    { ((u8 *)dest)[bytes - (i + 1)] = ((u8 *)src)[i]; }
}
static void copyMemory32(u32 *dest, u32 *src, u64 count)
{
    u32 i;
    for (i = 0; i < count; ++i)
    { ((u32 *)dest)[i] = ((u32 *)src)[i]; }
}
static void copyMemoryReverse32(u32 *dest, u32 *src, u64 count)
{
    u32 i;
    for (i = 0; i < count; ++i)
    { ((u32 *)dest)[count - (i + 1)] = ((u32 *)src)[i]; }
}

// static void copyPixel(u32 *dest, u32 *src, u64 count)
// {
//     u32 i;
//     for (i = 0; i < count; ++i)
//     {
//         if (((((u32 *)src)[i] >> 24) & 255) == 0)
//         { continue; }

//         u32 destpixel = ((u32 *)dest)[i];
//         u32 pixel = ((u32 *)src)[i];

//         float alpha = (float)(pixel >> 24) / 255.0f;
//         float new_red = ((float)(pixel >> 16 & 255) * alpha) + ((float)(destpixel >> 16 & 255) * (1.0 - alpha));
//         float new_green = ((float)(pixel >> 8 & 255) * alpha) + ((float)(destpixel >> 8 & 255) * (1.0 - alpha));
//         float new_blue = ((float)(pixel & 255) * alpha) + ((float)(destpixel & 255) * (1.0 - alpha));
//         u8 red8 = (u8)new_red;
//         u8 green8 = (u8)new_green;
//         u8 blue8 = (u8)new_blue;

//         ((u32 *)dest)[i] = 255 << 24 | red8 << 16 | green8 << 8 | blue8;
//     }
// }
/* special thanks to https://github.com/MaikSteiger/ilerp/blob/master/ilerp.h */
#define _ilerps_base(src, dest, delta, type) (type)((src * (((1LL << ((sizeof(type) * 8) - 1))) - delta) + dest * delta) >> 7)
#define _ilerpd_base(src, dest, delta, type) (type)((src * (((type) (((1LL << ((sizeof(type) * 8) - 1)) * 2) - 1)) - delta) + dest * delta) / 255)
static void copyPixel(u32 *dest, u32 *src, u64 count)
{
    u32 i;
    for (i = 0; i < count; ++i)
    {
        if (((((u32 *)src)[i] >> 24) & 255) == 0)
        { continue; }
        // if (((((u32 *)src)[i] >> 24) & 255) == 255)
        // { ((u8 *)dest)[i] = ((u8 *)src)[i]; }

        u32 destpixel = ((u32 *)dest)[i];
        u32 pixel = ((u32 *)src)[i];

        u8 alpha = pixel >> 24;
        u8 invalpha = 255 - alpha;

        u8 red8 = _ilerpd_base((u8)(pixel >> 16 & 255), (u8)(destpixel >> 16 & 255), invalpha, u8);
        u8 green8 = _ilerpd_base((u8)(pixel >> 8 & 255), (u8)(destpixel >> 8 & 255), invalpha, u8);
        u8 blue8 = _ilerpd_base((u8)(pixel & 255), (u8)(destpixel & 255), invalpha, u8);

        ((u32 *)dest)[i] = 255 << 24 | red8 << 16 | green8 << 8 | blue8;
    }
}
static void copyPixelReverse(u32 *dest, u32 *src, u64 count)
{
    u32 i;
    for (i = 0; i < count; ++i)
    {
        if (((((u32 *)src)[i] >> 24) & 255) == 0)
        { continue; }
        
        ((u32 *)dest)[count - (i + 1)] = ((u32 *)src)[i];
    }
}

static const char *readFile(Arena *arena, const char *path)
{
    FILE *f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    int length = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *out = memGrab(arena, length + 1);
    fread(out, length, 1, f);
    *(out + length) = '\0';

    fclose(f);

    return out;
}
static u8 *readFileRaw(Arena *arena, const char *path)
{
    FILE *f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    int length = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *out = memGrab(arena, length + 1);
    fread(out, length, 1, f);

    fclose(f);

    return out;
}

#endif