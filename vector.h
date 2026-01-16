#ifndef VECTOR_H
#define VECTOR_H

#include "def.h"
// #include "mathlib.h" /* implement */

typedef union
{
    struct
    {
        float x, y;
    };
    float arr[2];
} vec2;
typedef union
{
    struct
    {
        float x, y, z;
    };
    float arr[3];
} vec3;
typedef union
{
    struct
    {
        float x, y, z, w;
    };
    float arr[4];
} vec4;

static float lengthVec2(vec2 vec)
{
    return sqrt(vec.x * vec.x + vec.y * vec.y);
}
static vec2 normalizeVec2(vec2 vec)
{
    float length = lengthVec2(vec);
    if (length == 0.0f)
    { return (vec2){0.0f, 0.0f}; }
    return (vec2){vec.x / length, vec.y / length};
}
static vec2 multiVec2(vec2 vecA, vec2 vecB)
{
    return (vec2){vecA.x * vecB.x, vecA.y * vecB.y};
}
static vec2 multiScalarVec2(vec2 vec, float scalar)
{
    return (vec2){vec.x * scalar, vec.y * scalar};
}
static vec2 divVec2(vec2 vecA, vec2 vecB)
{
    return (vec2){vecA.x / vecB.x, vecA.y / vecB.y};
}
static float dotVec2(vec2 vecA, vec2 vecB)
{
    return (vecA.x * vecB.x + vecA.y * vecB.y);
}
static vec2 addVec2(vec2 vecA, vec2 vecB)
{
    return (vec2){vecA.x + vecB.x, vecA.y + vecB.y};
}
static vec2 subVec2(vec2 vecA, vec2 vecB)
{
    return (vec2){vecA.x - vecB.x, vecA.y - vecB.y};
}
static vec2 addScalarVec2(vec2 vec, float scalar)
{
    return (vec2){vec.x + scalar, vec.y + scalar};
}
static vec2 lerpVec2(vec2 vecA, vec2 vecB, float a)
{
    return (vec2){lerp(vecA.x, vecB.x, a), lerp(vecA.y, vecB.y, a)};
}
static vec2 vec2YX(vec2 vec)
{
    return (vec2){vec.y, vec.x};
}

#endif