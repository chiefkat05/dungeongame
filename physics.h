#ifndef PHYSICS_H
#define PHYSICS_H

#include "def.h"
#include "vector.h"

#define PHYSICS_TICK (1.0f/6000.0f)

typedef struct
{
    vec2 pos, size;
    bool colliding;
} Rect;
typedef struct
{
    vec2 last_pos, pos, vel;
    vec2 size;
    bool colliding;
} Obj2D;

static Obj2D makeObj(float x, float y, float w, float h)
{
    Obj2D obj = {.pos = {x, y}, .size = {w, h}};
    return obj;
}
static void objPut(Obj2D *obj, float x, float y)
{
    obj->pos = (vec2){x, y};
}
static void objMove(Obj2D *obj, vec2 move)
{
    obj->vel = move;
}
static void objMoveTarget(Obj2D *obj, float x_target, float y_target, float speed)
{
    vec2 distance = {x_target - obj->pos.x, y_target - obj->pos.y};
    vec2 direction = normalizeVec2(distance);

    obj->vel = multiScalarVec2(direction, speed);
}
static void objMoveX(Obj2D *obj, float x)
{
    obj->vel.x = x;
}
static void objMoveY(Obj2D *obj, float y)
{
    obj->vel.y = y;
}
static void objUpdate(Obj2D *obj)
{
    obj->last_pos = obj->pos;
    obj->pos = addVec2(obj->pos, multiScalarVec2(obj->vel, PHYSICS_TICK));

    obj->colliding = false;
}
static vec2 objGetPosition(Obj2D *obj, float alphaTime)
{
    return lerpVec2(obj->last_pos, obj->pos, alphaTime);
}

static void objCollisionResponseRect(Obj2D *obj, Rect *rect)
{
    if (rect->size.x < __FLT_EPSILON__ || rect->size.y < __FLT_EPSILON__ || obj->size.x < __FLT_EPSILON__ || obj->size.y < __FLT_EPSILON__)
    { return; }

    Rect boundsA = {
        .pos.x = obj->pos.x - MIN(0, obj->vel.x * PHYSICS_TICK),
        .pos.y = obj->pos.y - MIN(0, obj->vel.y * PHYSICS_TICK),
        .size.x = obj->pos.x + obj->size.x + MAX(0, obj->vel.x * PHYSICS_TICK),
        .size.y = obj->pos.y + obj->size.y + MAX(0, obj->vel.y * PHYSICS_TICK),
    };

    if (boundsA.pos.x > rect->pos.x + rect->size.x || boundsA.size.x < rect->pos.x ||
        boundsA.pos.y > rect->pos.y + rect->size.y || boundsA.size.y < rect->pos.y)
    { return; }

    Rect expandedRect = (Rect){
        .pos = rect->pos,
        .size = addVec2(obj->size,  rect->size)
    };

    float maxDistance = -__FLT_MAX__, closestEdge = 0.0f;
    int axis;

    int normalAxis = 0, axisSign = 1;
    for (axis = 0; axis < 2; ++axis)
    {
        if (maxDistance <= expandedRect.pos.arr[axis] - expandedRect.size.arr[axis] / 2.0f - obj->pos.arr[axis])
        {
            maxDistance = expandedRect.pos.arr[axis] - expandedRect.size.arr[axis] / 2.0f - obj->pos.arr[axis];
            closestEdge = expandedRect.pos.arr[axis] - expandedRect.size.arr[axis] / 2.0f;
            normalAxis = axis;
            axisSign = -1;
        }
        if (maxDistance <= obj->pos.arr[axis] - (expandedRect.pos.arr[axis] + expandedRect.size.arr[axis] / 2.0f))
        {
            maxDistance = obj->pos.arr[axis] - (expandedRect.pos.arr[axis] + expandedRect.size.arr[axis] / 2.0f);
            closestEdge = expandedRect.pos.arr[axis] + expandedRect.size.arr[axis] / 2.0f;
            normalAxis = axis;
            axisSign = 1;
        }
    }

    if (axisSign < 0 && obj->vel.arr[normalAxis] < 0.0f ||
        axisSign > 0 && obj->vel.arr[normalAxis] > 0.0f ||
        ABS(maxDistance) < __FLT_EPSILON__)
    {
        return;
    }

    obj->pos.arr[normalAxis] = closestEdge;
    obj->vel.arr[normalAxis] = 0.0f;

    obj->colliding = true;
    rect->colliding = true;
}
static void objCollisionResponse(Obj2D *obja, Obj2D *objb)
{
    if (obja->pos.x - obja->size.x / 2.0f > objb->pos.x + objb->size.x || obja->pos.x + obja->size.x / 2.0f < objb->pos.x ||
        obja->pos.y - obja->size.y / 2.0f > objb->pos.y + objb->size.y || obja->pos.y + obja->size.y / 2.0f < objb->pos.y)
    { return; }

    obja->vel = (vec2){0.0f, 0.0f};
}

#endif