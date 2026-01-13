#ifndef DEF_H
#define DEF_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define KILOBYTES(i) (i * 1024)
#define MEGABYTES(i) (KILOBYTES(i) * 1024)
#define GIGABYTES(i) (MEGABYTES(i) * 1024)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define ABS(a) (((a) < 0) ? (-a) : (a))

#define random(min, height) (rand() % ((height) - (min)) + min)
#define PI 3.1415926535f

#define DONT_CARE -1

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

/* remove */
#include <math.h>

/* less "precise" but works better for pixel updates */
static float lerp(float x, float y, float a)
{
    // return (1.0f - a) * x + a * y;
    return x + a * (y - x);
}

static int round_nearest(int num, int multi)
{
    if (num >= 0)
    {
        return ((num + multi / 2) / multi) * multi;
    } else
    {
        return ((num - multi / 2) / multi) * multi;
    }
}

static void letterbox(float width, float height, float box_width, float box_height, float *out_x, float *out_y, float *out_width, float *out_height)
{
    float WidthDiff = width / box_width;
    float HeightDiff = height / box_height;
    float widthAspect = WidthDiff / MIN(WidthDiff, HeightDiff);
    float heightAspect = HeightDiff / MIN(WidthDiff, HeightDiff);

    *out_width = (float)width / widthAspect;
    *out_height = (float)height / heightAspect;
    *out_x = width / 2 - (*out_width / 2.0f);
    *out_y = height / 2 - (*out_height / 2.0f);
}

#endif