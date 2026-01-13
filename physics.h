#ifndef PHYSICS_H
#define PHYSICS_H

#include "def.h"

#define PHYSICS_TICK (1.0f/6000.0f)

typedef struct
{
    float x, y, w, h;
} Rect;
typedef struct
{
    float last_x_pos, last_y_pos;
    float x_pos, y_pos;
    float width, height;
    float x_vel, y_vel;
} Obj2D;

static Obj2D makeObj(float x, float y, float w, float h)
{
    Obj2D obj = {.x_pos = x, .y_pos = y, .width = w, .height = h};
    return obj;
}
static void objPut(Obj2D *obj, float x_pos, float y_pos)
{
    obj->x_pos = x_pos;
    obj->y_pos = y_pos;
}
static void objMove(Obj2D *obj, float x_vel, float y_vel)
{
    obj->x_vel = x_vel;
    obj->y_vel = y_vel;
}
static float vectorLength(float x, float y)
{
    return sqrt(x * x + y * y);
}
static void vectorNormalized(float x, float y, float *out_x, float *out_y)
{
    float length = vectorLength(x, y);
    *out_x = x / length;
    *out_y = y / length;
}
static void objMoveTarget(Obj2D *obj, float x_target, float y_target, float speed)
{
    float x_dist = x_target - obj->x_pos;
    float y_dist = y_target - obj->y_pos;

    float length = vectorLength(x_dist, y_dist);
    if (length == 0.0f)
    { return; }

    float x_dir = x_dist / length;
    float y_dir = y_dist / length;

    obj->x_vel = speed * x_dir;
    obj->y_vel = speed * y_dir;
}
static void objMoveX(Obj2D *obj, float x_vel)
{
    obj->x_vel = x_vel;
}
static void objMoveY(Obj2D *obj, float y_vel)
{
    obj->y_vel = y_vel;
}
static void objUpdate(Obj2D *obj)
{
    obj->last_x_pos = obj->x_pos;
    obj->last_y_pos = obj->y_pos;

    obj->x_pos += obj->x_vel * PHYSICS_TICK;
    obj->y_pos += obj->y_vel * PHYSICS_TICK;
}
static float objGetX(Obj2D *obj, float alphaTime)
{
    return lerp(obj->last_x_pos, obj->x_pos, alphaTime);
}
static float objGetY(Obj2D *obj, float alphaTime)
{
    return lerp(obj->last_y_pos, obj->y_pos, alphaTime);
}

#endif