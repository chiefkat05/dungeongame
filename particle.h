#ifndef PARTICLE_H
#define PARTICLE_H

#include "def.h"
#include "image.h"
#include "physics.h"

typedef enum
{
    PARTICLESYSTEM_SNOW,
    PARTICLESYSTEM_SPACEFIELD,
    PARTICLESYSTEM_FIRE,
    PARTICLESYSTEM_SAND,
    PARTICLESYSTEM_BOIDS
} ParticleSystemType;

#define MAX_PARTICLE_COUNT 4096
typedef struct
{
    ParticleSystemType type;
    float last_x_pos[MAX_PARTICLE_COUNT];
    float last_y_pos[MAX_PARTICLE_COUNT];
    float x_pos[MAX_PARTICLE_COUNT];
    float y_pos[MAX_PARTICLE_COUNT];
    float last_x_vel[MAX_PARTICLE_COUNT];
    float last_y_vel[MAX_PARTICLE_COUNT];
    float x_vel[MAX_PARTICLE_COUNT];
    float y_vel[MAX_PARTICLE_COUNT];
    float x_vel_push[MAX_PARTICLE_COUNT];
    float y_vel_push[MAX_PARTICLE_COUNT];

    u8 red[MAX_PARTICLE_COUNT];
    u8 green[MAX_PARTICLE_COUNT];
    u8 blue[MAX_PARTICLE_COUNT];

    bool alive[MAX_PARTICLE_COUNT];
    float life_time[MAX_PARTICLE_COUNT];
    bool life_limited[MAX_PARTICLE_COUNT];

    u32 particle_limit;
    u32 current_editing_particle;

    float generateTimer;
} ParticleSystem;

static bool particleSystemAdd(ParticleSystem *system, float xpos, float ypos)
{
    int particleIndex = -1;

    int i;
    for (i = 0; i < system->particle_limit; ++i)
    {
        if (!system->alive[i])
        {
            particleIndex = i;
            break;
        }
    }
    if (particleIndex == -1)
    { return false; }

    system->x_vel[particleIndex] = 0.0f;
    system->y_vel[particleIndex] = 0.0f;
    system->x_vel_push[particleIndex] = 0.0f;
    system->y_vel_push[particleIndex] = 0.0f;
    system->x_pos[particleIndex] = xpos;
    system->y_pos[particleIndex] = ypos;
    system->alive[particleIndex] = true;

    system->current_editing_particle = particleIndex;

    return true;
}

static void particleSystemPush(ParticleSystem *system, float xpush, float ypush)
{
    system->x_vel_push[system->current_editing_particle] = xpush;
    system->y_vel_push[system->current_editing_particle] = ypush;
}
static void particleSystemMove(ParticleSystem *system, float xvel, float yvel)
{
    system->x_vel[system->current_editing_particle] = xvel;
    system->y_vel[system->current_editing_particle] = yvel;
}
static void particleSystemLifespan(ParticleSystem *system, float time)
{
    system->life_limited[system->current_editing_particle] = true;
    system->life_time[system->current_editing_particle] = time;
}
static void particleSystemColor(ParticleSystem *system, u8 red, u8 green, u8 blue)
{
    system->red[system->current_editing_particle] = red;
    system->green[system->current_editing_particle] = green;
    system->blue[system->current_editing_particle] = blue;
}

/* remove */
#include <math.h>
static void particleSystemGenerate(ParticleSystem *system, int particles, float timerReset, int xmin, int xmax, int ymin, int ymax)
{
    system->generateTimer -= PHYSICS_TICK;
    if (system->generateTimer > 0.0f)
    { return; }

    system->generateTimer = timerReset;

    int i;
    for (i = 0; i < particles; ++i)
    {
        int xpos = xmin;
        int ypos = ymin;
        if (xmax > xmin) { xpos = random(xmin, xmax); }
        if (ymax > ymin) { ypos = random(ymin, ymax); }
        if (!particleSystemAdd(system, xpos, ypos))
        { return; }

        switch(system->type)
        {
            case PARTICLESYSTEM_SNOW:
                {
                    int shade = random(200, 255);
                    particleSystemColor(system, shade, shade, shade);
                    particleSystemMove(system, (float)(random(100, 150)), -(float)(random(50, 125)));
                } break;
            case PARTICLESYSTEM_SPACEFIELD:
                {
                    int shade = 255;
                    particleSystemColor(system, shade, shade, shade);
                    float x_speed = (float)(random(-1000, 1000));
                    float y_speed = (float)(random(-1000, 1000));

                    float length = sqrt((x_speed * x_speed) + (y_speed * y_speed));
                    x_speed /= length;
                    y_speed /= length;

                    particleSystemPush(system, x_speed * 1000.0f, y_speed * 1000.0f);
                    particleSystemLifespan(system, 0.5f);
                } break;
            case PARTICLESYSTEM_SAND:
                {
                    int shade = random(150, 255);
                    int red = MIN(255, shade + 50);
                    int green = MIN(255, shade + 30);
                    particleSystemColor(system, red, green, shade);

                    particleSystemMove(system, (float)(random(-100, 100)) / 10.0f, 0.0f);
                    particleSystemPush(system, 0.0f, -(float)(random(700, 800)) / 10.0f);
                } break;
            case PARTICLESYSTEM_FIRE:
                {
                    int shade = random(20, 40);
                    int red = MIN(255, shade + 200);
                    int green = MIN(255, shade + 50);
                    particleSystemColor(system, red, green, shade);

                    particleSystemMove(system, (float)(random(-100, 100)) / 10.0f, 0.0f);
                    particleSystemPush(system, 0.0f, (float)(random(700, 800)) / 10.0f);
                    particleSystemLifespan(system, 1.0f);
                } break;
            default:
                break;
        }
    }
}

static void particleSystemUpdate(ParticleSystem *system, image *buffer)
{
    int p;
    for (p = 0; p < MAX_PARTICLE_COUNT; ++p)
    {
        if (!system->alive[p])
        { continue; }

        system->last_x_vel[p] = system->x_vel[p];
        system->x_vel[p] += system->x_vel_push[p] * PHYSICS_TICK;
        system->last_y_vel[p] = system->y_vel[p];
        system->y_vel[p] += system->y_vel_push[p] * PHYSICS_TICK;

        system->last_x_pos[p] = system->x_pos[p];
        system->last_y_pos[p] = system->y_pos[p];

        system->x_pos[p] += (system->x_vel[p] + system->last_x_vel[p]) / 2.0f * PHYSICS_TICK;
        system->y_pos[p] += (system->y_vel[p] + system->last_y_vel[p]) / 2.0f * PHYSICS_TICK;

        if (system->life_limited[p])
        {
            system->life_time[p] -= PHYSICS_TICK;

            if (system->life_time[p] <= 0.0f)
            {
                system->alive[p] = false;
            }
        }

        u32 pixel_color = system->red[p] | system->green[p] << 8 | system->blue[p] << 16 | 255 << 24;

        /* system-specific activities */
        switch(system->type)
        {
            case PARTICLESYSTEM_SNOW:
                if (system->y_pos[p] < 0)
                {
                    system->alive[p] = false;
                }
                if (system->x_pos[p] > buffer->width)
                {
                    system->x_pos[p] = 0;
                }
                break;
            case PARTICLESYSTEM_SPACEFIELD:
                if (system->y_pos[p] < 0 || system->x_pos[p] < 0 || system->y_pos[p] > buffer->height || system->x_pos[p] > buffer->width)
                {
                    system->alive[p] = false;
                }
                break;
            case PARTICLESYSTEM_SAND:
                if (system->y_pos[p] < 0)
                {
                    system->y_pos[p] = 0;
                    system->current_editing_particle = p;
                    particleSystemPush(system, 0.0f, 0.0f);
                }
                break;
            default:
                break;
        }
    }
}
static void particleSystemDraw(ParticleSystem *system, image *buffer, float alphaTime)
{
    int p;
    for (p = 0; p < MAX_PARTICLE_COUNT; ++p)
    {
        if (!system->alive[p])
        { continue; }

        float particleAlphaX = lerp(system->last_x_pos[p], system->x_pos[p], alphaTime);
        float particleAlphaY = lerp(system->last_y_pos[p], system->y_pos[p], alphaTime);
        u32 pixel_color = system->red[p] | system->green[p] << 8 | system->blue[p] << 16 | 255 << 24;

        imageSetPixel(buffer, particleAlphaX, particleAlphaY, pixel_color);
    }
}

#endif