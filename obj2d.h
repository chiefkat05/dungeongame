#ifndef PHYSICS_H
#define PHYSICS_H

#include "def.h"
#include "vector.h"
#include "image.h"

#define PHYSICS_TICK (1.0f/6000.0f)

typedef struct
{
    vec2 pos, size;
    bool colliding;
} Rect;

typedef struct _obj2D
{
    vec2 last_pos, pos, vel;
    vec2 size;
    bool colliding;
    bool alive, allocated;
    image *img;

    struct _obj2D *next;
} Obj2D;

static Obj2D obj2D(float x, float y, float w, float h)
{
    Obj2D obj = {.pos = {x, y}, .size = {w, h}, .alive = true, .allocated = true};
    return obj;
}
static void obj2DPut(Obj2D *obj, float x, float y)
{
    obj->pos = (vec2){x, y};
}
static void obj2DMove(Obj2D *obj, vec2 move)
{
    obj->vel = move;
}
static void obj2DMoveTarget(Obj2D *obj, float x_target, float y_target, float speed)
{
    vec2 distance = {x_target - obj->pos.x, y_target - obj->pos.y};
    vec2 direction = normalizeVec2(distance);

    obj->vel = multiScalarVec2(direction, speed);
}
static void obj2DMoveX(Obj2D *obj, float x)
{
    obj->vel.x = x;
}
static void obj2DMoveY(Obj2D *obj, float y)
{
    obj->vel.y = y;
}
static void obj2DUpdate(Obj2D *obj)
{
    if (!obj->alive) { return; }
    
    obj->last_pos = obj->pos;
    obj->pos = addVec2(obj->pos, multiScalarVec2(obj->vel, PHYSICS_TICK));

    obj->colliding = false;
}
static void obj2DKill(Obj2D *obj)
{
    obj->alive = false;
}
static void obj2DResurrect(Obj2D *obj)
{
    obj->alive = true;
}
static vec2 obj2DGetPosition(Obj2D *obj, float alphaTime)
{
    return lerpVec2(obj->last_pos, obj->pos, alphaTime);
}
static void obj2DSetImage(Obj2D *obj, image *img)
{
    obj->img = img;
}
static void obj2DDraw(Obj2D *obj, image *outImg, float alphaTime)
{
    if (obj->img == NULL || outImg == NULL)
    { return; }

    /* implement spritesheet support */
    imageCopyToImage(obj->img, outImg, obj2DGetPosition(obj, alphaTime).x, obj2DGetPosition(obj, alphaTime).y, 0, 0, -1, -1);
}

static void obj2DCollisionResponseRect(Obj2D *obj, Rect *rect)
{
    /* still an issue where, when moving along a line of colliders aligned on the same axis, you will get 'stuck' on boxes occasionally.
        The reason this happens is because the box reads the obj's velocity/position on the next frame, which is likely to be
        inside the box coming from a strange angle.
    */
    if (rect->size.x < __FLT_EPSILON__ || rect->size.y < __FLT_EPSILON__ || obj->size.x < __FLT_EPSILON__ || obj->size.y < __FLT_EPSILON__ ||
        !obj->alive)
    { return; }

    Rect boundsA = {
        .pos.x = obj->pos.x - obj->size.x / 2.0f - MIN(0, obj->vel.x * PHYSICS_TICK),
        .pos.y = obj->pos.y - obj->size.y / 2.0f - MIN(0, obj->vel.y * PHYSICS_TICK),
        .size.x = obj->pos.x + obj->size.x / 2.0f + MAX(0, obj->vel.x * PHYSICS_TICK),
        .size.y = obj->pos.y + obj->size.y / 2.0f + MAX(0, obj->vel.y * PHYSICS_TICK),
    };

    if (boundsA.pos.x > rect->pos.x + rect->size.x / 2.0f || boundsA.size.x < rect->pos.x - rect->size.x / 2.0f ||
        boundsA.pos.y > rect->pos.y + rect->size.y / 2.0f || boundsA.size.y < rect->pos.y - rect->size.y / 2.0f)
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

#define MAX_OBJECT_COUNT 16
typedef struct
{
    Obj2D objList[MAX_OBJECT_COUNT];
    u32 head;
    Obj2D *freeList;
} Obj2DPool;

static Obj2D *obj2DPoolAlloc(Obj2DPool *pool)
{
    if (pool->head < MAX_OBJECT_COUNT)
    {
        pool->objList[pool->head] = (Obj2D){.allocated = true};
        return &pool->objList[pool->head++];
    }

    Obj2D *ret = pool->freeList;
    if (ret == NULL)
    {
        printf("\n\n\tNo more objects in Obj2DPool\n\n");
        return NULL;
    }
    pool->freeList = pool->freeList->next;

    *ret = (Obj2D){.allocated = true};
    return ret;
}
static void obj2DPoolFree(Obj2DPool *pool, Obj2D *obj)
{
    obj->allocated = false;
    obj->next = pool->freeList;
    pool->freeList = obj;
}

static u32 obj2DPoolRemaining(Obj2DPool *pool)
{
    return MAX_OBJECT_COUNT - pool->head;
}
static void obj2DPoolClear(Obj2DPool *pool)
{
    pool->freeList = NULL;
    pool->head = 0;
}
static void obj2DPoolResetUpdate(Obj2DPool *pool)
{
    if (pool->head == 0 || pool->freeList == NULL) { return; }

    bool anyInPlay = false;

    int i;
    for (i = 0; i < MAX_OBJECT_COUNT; ++i)
    {
        if (pool->objList[i].allocated)
        {
            anyInPlay = true;
        }
    }

    Obj2D *cursor = pool->freeList;
    while (cursor != NULL)
    {
        if (cursor->allocated)
        {
            anyInPlay = true;
        }
        cursor = cursor->next;
    }

    if (!anyInPlay)
    {
        obj2DPoolClear(pool);
    }
}

static void obj2DRule_OutOfScreenDeath(Obj2D *obj)
{
    if (obj->pos.x - obj->size.x / 2.0f > SCREENWIDTH || obj->pos.x + obj->size.x / 2.0f < 0.0f ||
        obj->pos.y - obj->size.y / 2.0f > SCREENHEIGHT || obj->pos.y + obj->size.y / 2.0f < 0.0f)
    {
        obj->alive = false;
    }
}
static void obj2DPoolRule(Obj2DPool *pool, void (*ruleFunction)(Obj2D *obj))
{
    u32 i;
    for (i = 0; i < pool->head; ++i)
    {
        ruleFunction(&pool->objList[i]);
    }
    if (pool->head >= MAX_OBJECT_COUNT)
    {
        Obj2D *cursor = pool->freeList;
        while (cursor)
        {
            ruleFunction(cursor);
            cursor = cursor->next;
        }
    }
}
static void obj2DPoolUpdate(Obj2DPool *pool)
{
    u32 i;
    for (i = 0; i < pool->head; ++i)
    {
        if (!pool->objList[i].alive && pool->objList[i].allocated)
        {
            obj2DPoolFree(pool, &pool->objList[i]);
            continue;
        }
        obj2DUpdate(&pool->objList[i]);
    }
    if (pool->head >= MAX_OBJECT_COUNT)
    {
        Obj2D *cursor = pool->freeList;
        while (cursor != NULL)
        {
            if (!cursor->alive && cursor->allocated)
            {
                obj2DPoolFree(pool, cursor);
                cursor = cursor->next;
                continue;
            }
            obj2DUpdate(cursor);
            cursor = cursor->next;
        }
    }
}
static void obj2DPoolDraw(Obj2DPool *pool, image *outImg, float alphaTime)
{
    if (outImg == NULL) { return; }

    u32 i;
    for (i = 0; i < pool->head; ++i)
    {
        obj2DDraw(&pool->objList[i], outImg, alphaTime);
    }
    if (pool->head >= MAX_OBJECT_COUNT)
    {
        Obj2D *cursor = pool->freeList;
        while (cursor)
        {
            obj2DDraw(cursor, outImg, alphaTime);
            cursor = cursor->next;
        }
    }
}

#endif