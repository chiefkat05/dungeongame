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