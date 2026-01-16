#ifndef IMAGE_H
#define IMAGE_H

#include "def.h"
#include "vector.h"

#define SCREENWIDTH 256
#define SCREENHEIGHT 192
typedef struct
{
    void *data;
    u32 width;
    u32 height;
    u32 row;
    u32 pixel_count;
} image;

#define BYTES_PER_PIXEL 4

static void imageSetPixel(image *a, int xpos, int ypos, float red, float green, float blue, float alpha)
{
    if (xpos < 0 || ypos < 0 || xpos >= a->width || ypos >= a->height || alpha < __FLT_EPSILON__)
    { return; }

    u32 *pixel_location = (u32 *)(a->data + (ypos * a->row) + xpos * BYTES_PER_PIXEL);

    u32 destpixel = *pixel_location;

    float invalpha = 1.0f - alpha;
    float dest_redf = (float)(destpixel >> 16 & 255) / 255.0f;
    float dest_greenf = (float)(destpixel >> 8 & 255) / 255.0f;
    float dest_bluef = (float)(destpixel & 255) / 255.0f;
    
    float redmix = lerp(red, dest_redf, invalpha);
    float greenmix = lerp(green, dest_greenf, invalpha);
    float bluemix = lerp(blue, dest_bluef, invalpha);

    u8 red8 = (u8)roundFlt(redmix * 255.0f);
    u8 green8 = (u8)roundFlt(greenmix * 255.0f);
    u8 blue8 = (u8)roundFlt(bluemix * 255.0f);

    *pixel_location = 255 << 24 | blue8 << 16 | green8 << 8 | red8;
}
static void imageSetRect(image *a, int left, int bottom, int right, int top, float red, float green, float blue, float alpha)
{
    left = MAX(left, 0);
    bottom = MAX(bottom, 0);
    right = MIN(right, (int)a->width);
    top = MIN(top, (int)a->height);

    // u32 *pPixel = (u32 *)a->data + bottom * a->width + left;
    // u32 color = (u8)(alpha * 255.0f) << 24 | (u8)(red * 255.0f) << 16 | (u8)(green * 255.0f) << 8 | (u8)(blue * 255.0f);
    int x, y;
    for (y = bottom; y < top; ++y)
    {
        for (x = left; x < right; ++x)
        {
            // *pPixel++ = color;
            imageSetPixel(a, x, y, red, green, blue, alpha);
        }
        // pPixel += a->width - (right - left);
    }
}
static void imageClear(image *a)
{
    u32 *dataCursor = (u32 *)a->data;
    int p;
    for (p = 0; p < a->pixel_count; ++p)
    {
        *dataCursor++ = 0;
    }
}
static void imageFillBlack(image *a)
{
    u32 *dataCursor = (u32 *)a->data;
    int p;
    for (p = 0; p < a->pixel_count; ++p)
    {
        *dataCursor++ = 255 << 24;
    }
}
static void imageFillWhite(image *a)
{
    u32 *dataCursor = (u32 *)a->data;
    int p;
    for (p = 0; p < a->pixel_count; ++p)
    {
        *dataCursor++ = 255 << 24 | 255 << 16 | 255 << 8 | 255;
    }
}
static void imageFill(image *a, u32 color)
{
    u32 *dataCursor = (u32 *)a->data;
    int p;
    for (p = 0; p < a->pixel_count; ++p)
    {
        *dataCursor++ = color;
    }
}

static image makeImage(Arena *arena, u32 width, u32 height)
{
    image img;

    img.width = width;
    img.height = height;
    img.row = img.width * BYTES_PER_PIXEL;
    img.pixel_count = img.width * img.height;
    img.data = memGrab(arena, img.width * img.height * BYTES_PER_PIXEL);

    return img;
}
/* A heavy-duty version of the quake draw-pic function, built to (hopefully) tank any random values I chuck at it. */
/* Could also likely use optimization, as it's very brute-force at the moment. */

#define SPRITE_EDGE 16
#define SPRITE_COPY_TO_IMAGE(src, dest, dx, dy, sx, sy, sw, sh) imageCopyToImage(src, dest, dx, dy, sx * SPRITE_EDGE, sy * SPRITE_EDGE, sw * SPRITE_EDGE, sh * SPRITE_EDGE)
static void imageCopyToImage(image *src, image *dest, int dest_x_pos, int dest_y_pos, int src_x, int src_y, int src_width, int src_height)
{
    /* defaults */
    if (dest_x_pos == DONT_CARE) { dest_x_pos == 0; }
    if (dest_y_pos == DONT_CARE) { dest_y_pos == 0; }
    if (src_x == DONT_CARE) { src_x == 0; }
    if (src_y == DONT_CARE) { src_y == 0; }
    if (src_width == DONT_CARE) { src_width == src->width; }
    if (src_height == DONT_CARE) { src_height == src->height; }

    if (dest_x_pos > (int)dest->width || dest_y_pos > (int)dest->height)
    { return; }

    src_width = MIN(src->width, src_width);
    src_height = MIN(src->height, src_height);

    src_x = MAX(0, src_x);
    src_y = MAX(0, src_y);
    src_x = MIN(src_x, src->width - src_width);
    src_y = MIN(src_y, src->height - src_height);

    src_x %= src->width;

    src_width = MIN(src_width, src->width);
    src_height = MIN(src_height, src->height);

    u32 dest_x = MAX(0, dest_x_pos);
    u32 dest_y = MAX(0, dest_y_pos);

    int draw_offset_x = 0 - dest_x_pos;
    int draw_offset_y = 0 - dest_y_pos;

    u32 *srcData = src->data;
    u32 *destData = dest->data;
    destData += (dest_y * dest->width) + dest_x;

    u32 draw_width = src_width - MAX(0, draw_offset_x);
    u32 draw_height = src_height - MAX(0, draw_offset_y);

    int off_right_side = (dest_x_pos + src->width) - dest->width;
    int off_top_side = (dest_y_pos + src->height) - dest->height;

    if (draw_offset_x >= src_width)
    { draw_width = 0; }
    if (draw_offset_x < 0)
    { draw_width = src->width - (MAX(0, off_right_side));}

    if (draw_offset_y >= src_height)
    { draw_height = 0; }
    if (draw_offset_y < 0)
    { draw_height = src->height - MAX(0, off_top_side); }

    draw_width = MIN(draw_width, src->width);
    draw_height = MIN(draw_height, src->height);

    draw_offset_x = MAX(0, draw_offset_x);
    draw_offset_y = MAX(0, draw_offset_y);

    srcData += src->width * draw_offset_y;
    srcData += draw_offset_x;

    srcData += src_y * src->width;
    srcData += src_x;

    draw_height = MIN(draw_height, src_height);
    draw_width = MIN(draw_width, src_width);
    
    int dy;
    for (dy = draw_offset_y; dy < draw_offset_y + draw_height; ++dy)
    {
        copyPixel(destData, srcData, draw_width);
        destData += dest->width;
        srcData += src->width;
    }
}
static void imageToImage(image *src, image *dest)
{
    if (src->width != dest->width || src->height != dest->height)
    {
        printf("invalid bounds on imageToImage call\n");
        exit(1);
    }

    u32 *srcData = (u32 *)src->data;
    u32 *destData = (u32 *)dest->data;

    int dy;
    for (dy = 0; dy < dest->height; ++dy)
    {
        copyPixel(destData, srcData, dest->width);
        destData += dest->width;
        srcData += src->width;
    }
}

static u32 *loadBMP(Arena *arena, const char *path, u32 *out_width, u32 *out_height)
{
    Arena localMemory;
    memoryInit(&localMemory, MEGABYTES(8));
    u8 *data = (u8 *)readFileRaw(&localMemory, path);

    int start = (*(int *)(data + 10));

    i32 width = *(i32 *)(data + 18);
    *out_width = width;
    i32 height = *(i32 *)(data + 22);
    *out_height = height;

    u8 *bmpData = (u8 *)data + start;

    u32 *pixelPointer = (u32 *)bmpData;
    u32 *outPixelData = memGrab(arena, width * height * BYTES_PER_PIXEL);
    u32 *outPixelPointer = outPixelData;
    int i;
    for (i = 0; i < width * height; ++i)
    {
        u8 *red = (u8 *)pixelPointer;
        u8 *green = (u8 *)pixelPointer + 1;
        u8 *blue = (u8 *)pixelPointer + 2;
        u8 *alpha = (u8 *)pixelPointer + 3;

        *outPixelPointer = *alpha << 24 | *red << 16 | *green << 8 | *blue;

        ++outPixelPointer;
        ++pixelPointer;
    }

    u32 *out_data = (u32 *)outPixelData;

    memoryClear(&localMemory);

    return out_data;
}
static image imageMakeFromBMP(Arena *arena, const char *path)
{
    image img;

    u32 bmpWidth, bmpHeight;
    u32 *bmpData = loadBMP(arena, path, &bmpWidth, &bmpHeight);

    img.width = bmpWidth;
    img.height = bmpHeight;
    img.row = img.width * BYTES_PER_PIXEL;
    img.pixel_count = img.width * img.height;
    img.data = bmpData;

    return img;
}


void imageScaleToImage(image *src, image *dst)
{
    float scaleX = (float)dst->width / (float)src->width;
    float scaleY = (float)dst->height / (float)src->height;
    int x, y;
    u32 *pixel = (u32 *)dst->data;
    u32 *srcpixel = (u32 *)src->data;

    int minw = MIN(src->width, dst->width);
    int minh = MIN(src->height, dst->height);
    int maxw = MAX(src->width, dst->width);
    int maxh = MAX(src->height, dst->height);

    int currentX = 0;
    int currentY = 0;

    for (y = 0; y < dst->height; ++y)
    {
        for (x = 0; x < dst->width; ++x)
        {
            int pX = (int)((float)x / scaleX);
            int pY = (int)((float)y / scaleY);
            int diffX = pX - currentX;
            int diffY = pY - currentY;

            srcpixel += diffX;
            currentX = pX;
            srcpixel += diffY * src->width;
            currentY = pY;

            copyPixel(pixel, srcpixel, 1);
            ++pixel;
        }
    }
}

/* special effects */

static void imageColorMulti(image *src, float red_multi, float green_multi, float blue_multi)
{
    u32 *srcData = src->data;

    int p;
    for (p = 0; p < src->pixel_count; ++p)
    {
        u8 old_red = ((*srcData) >> 16) & 255;
        u8 old_green = ((*srcData) >> 8) & 255;
        u8 old_blue = (*srcData) & 255;
        
        float redf = (float)old_red * red_multi;
        float greenf = (float)old_green * green_multi;
        float bluef = (float)old_blue * blue_multi;

        u8 new_red = (u8)redf;
        u8 new_green = (u8)greenf;
        u8 new_blue = (u8)bluef;

        srcData++;
    }
}

static void imageFlipVertical(image *src, image *dst)
{
    if (src->width != dst->width || src->height != dst->height)
    {
        printf("can't flip image onto image of difference size\n");
        return;
    }

    u32 *dstData = dst->data + dst->pixel_count * BYTES_PER_PIXEL;
    dstData -= src->width;
    u32 *srcData = src->data;

    int dy;
    for (dy = 0; dy < src->height; ++dy)
    {
        copyPixel(dstData, srcData, src->width);
        dstData -= src->width;
        srcData += src->width;
    }
}
static void imageFlipHorizontal(image *src, image *dst)
{
    if (src->width != dst->width || src->height != dst->height)
    {
        printf("can't flip image onto image of difference size\n");
        return;
    }

    u32 *dstData = dst->data;
    u32 *srcData = src->data;

    int dy;
    for (dy = 0; dy < src->height; ++dy)
    {
        copyPixelReverse(dstData, srcData, src->width);
        dstData += src->width;
        srcData += src->width;
    }
}

/*  2d physiccs obj pool, temporary objects (bullets) test, make your own vignette then go do animatin and dnd */
/* thanks to https://www.shadertoy.com/view/lsKSWR */
static void imageVignette(image *src, float strength, float extent)
{
    u32 *srcData = src->data;

    int x, y;
    for (y = 0; y < src->height; ++y)
    {
        for (x = 0; x < src->width; ++x)
        {
            vec2 uv = divVec2((vec2){x, y}, (vec2){src->width, src->height});
            uv = multiVec2(uv, subVec2((vec2){1.0f, 1.0f}, vec2YX(uv)));
            float vig = uv.x * uv.y * strength;

            vig = pow(vig, extent);

            imageSetPixel(src, x, y, 0, 0, 0, 1.0f - vig);
        }
    }
}
// screen shake

// heat wave effect

// simple lighting

// imageDoSFX(&img, &outImg, int heatWaveHorizontalWidth, int heatWaveSpeed)
// {

// }

//

#endif