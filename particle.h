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

#define MAX_PARTICLE_COUNT 1024
typedef struct
{
    float last_x_pos;
    float last_y_pos;
    float x_pos;
    float y_pos;
    float last_x_vel;
    float last_y_vel;
    float x_vel;
    float y_vel;
    float x_vel_push;
    float y_vel_push;
} ParticleBody;
typedef struct
{
    float red_pos;
    float green_pos;
    float blue_pos;
    float alpha_pos;
    u32 pixel_color;
} ParticleColor;
typedef struct
{
    bool alive;
    float life_time;
    bool life_limited;
} ParticleLife;
typedef struct
{
    ParticleSystemType type;

    ParticleBody bodies[MAX_PARTICLE_COUNT];
    ParticleColor colors[MAX_PARTICLE_COUNT];
    ParticleLife lives[MAX_PARTICLE_COUNT];
    image *images[MAX_PARTICLE_COUNT];

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
        if (!system->lives[i].alive)
        {
            particleIndex = i;
            break;
        }
    }
    if (particleIndex == -1)
    { return false; }

    system->lives[i].alive = true;

    system->bodies[particleIndex].x_vel = 0.0f;
    system->bodies[particleIndex].y_vel = 0.0f;
    system->bodies[particleIndex].x_vel_push = 0.0f;
    system->bodies[particleIndex].y_vel_push = 0.0f;
    system->bodies[particleIndex].x_pos = xpos;
    system->bodies[particleIndex].y_pos = ypos;

    system->current_editing_particle = particleIndex;

    return true;
}

static void particleSystemPush(ParticleSystem *system, float xpush, float ypush)
{
    system->bodies[system->current_editing_particle].x_vel_push = xpush;
    system->bodies[system->current_editing_particle].y_vel_push = ypush;
}
static void particleSystemMove(ParticleSystem *system, float xvel, float yvel)
{
    system->bodies[system->current_editing_particle].x_vel = xvel;
    system->bodies[system->current_editing_particle].y_vel = yvel;
}
static void particleSystemLifespan(ParticleSystem *system, float time)
{
    system->lives[system->current_editing_particle].life_limited = true;
    system->lives[system->current_editing_particle].life_time = time;
}
static void particleSystemColor(ParticleSystem *system, u8 red, u8 green, u8 blue, u8 alpha)
{
    system->colors[system->current_editing_particle].red_pos = red;
    system->colors[system->current_editing_particle].green_pos = green;
    system->colors[system->current_editing_particle].blue_pos = blue;
    system->colors[system->current_editing_particle].alpha_pos = alpha;
}

/* particle generation hints functions here (so you don't have to keep changing particle effects) */
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
                    particleSystemColor(system, shade, shade, shade, 160);
                    particleSystemMove(system, (float)(random(100, 150)), -(float)(random(50, 125)));
                } break;
            case PARTICLESYSTEM_SPACEFIELD:
                {
                    int shade = 255;
                    particleSystemColor(system, shade, shade, shade, 255);
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
                    particleSystemColor(system, red, green, shade, 255);

                    particleSystemMove(system, (float)(random(-100, 100)) / 10.0f, 0.0f);
                    particleSystemPush(system, 0.0f, -(float)(random(700, 800)) / 10.0f);
                } break;
            case PARTICLESYSTEM_FIRE:
                {
                    int shade = random(20, 40);
                    int red = MIN(255, shade + 240);
                    int green = MIN(255, shade + 200);
                    int blue = MIN(255, shade + 100);
                    particleSystemColor(system, red, green, blue, 255);

                    particleSystemMove(system, (float)(random(-500, 500)) / 10.0f, 0.0f);
                    particleSystemPush(system, 0.0f, (float)(random(1000, 1200)) / 10.0f);
                    particleSystemLifespan(system, 0.8f);
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
        if (!system->lives[p].alive)
        { continue; }

        system->bodies[p].last_x_vel = system->bodies[p].x_vel;
        system->bodies[p].x_vel += system->bodies[p].x_vel_push * PHYSICS_TICK;
        system->bodies[p].last_y_vel = system->bodies[p].y_vel;
        system->bodies[p].y_vel += system->bodies[p].y_vel_push * PHYSICS_TICK;

        system->bodies[p].last_x_pos = system->bodies[p].x_pos;
        system->bodies[p].last_y_pos = system->bodies[p].y_pos;

        system->bodies[p].x_pos += (system->bodies[p].x_vel + system->bodies[p].last_x_vel) / 2.0f * PHYSICS_TICK;
        system->bodies[p].y_pos += (system->bodies[p].y_vel + system->bodies[p].last_y_vel) / 2.0f * PHYSICS_TICK;

        if (system->lives[p].life_limited)
        {
            system->lives[p].life_time -= PHYSICS_TICK;

            if (system->lives[p].life_time <= 0.0f)
            {
                system->lives[p].alive = false;
                continue;
            }
        }

        /* system-specific activities */
        switch(system->type)
        {
            case PARTICLESYSTEM_SNOW:
                if (system->bodies[p].y_pos < 0)
                {
                    system->lives[p].alive = false;
                }
                if (system->bodies[p].x_pos > buffer->width)
                {
                    system->bodies[p].x_pos = 0;
                }
                break;
            case PARTICLESYSTEM_SPACEFIELD:
                if (system->bodies[p].y_pos < 0 || system->bodies[p].x_pos < 0 || system->bodies[p].y_pos > buffer->height || system->bodies[p].x_pos > buffer->width)
                {
                    system->lives[p].alive = false;
                }
                break;
            case PARTICLESYSTEM_SAND:
                if (system->bodies[p].y_pos < 0)
                {
                    system->bodies[p].y_pos = 0;
                    system->current_editing_particle = p;
                    particleSystemPush(system, 0.0f, 0.0f);
                }
                break;
            case PARTICLESYSTEM_FIRE:
                system->colors[p].red_pos -= 10.0f * PHYSICS_TICK;
                system->colors[p].green_pos -= 120.0f * PHYSICS_TICK;
                system->colors[p].blue_pos -= 240.0f * PHYSICS_TICK;
                system->colors[p].alpha_pos -= 200.0f * PHYSICS_TICK;
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
        if (!system->lives[p].alive)
        { continue; }

        float particleAlphaX = lerp(system->bodies[p].last_x_pos, system->bodies[p].x_pos, alphaTime);
        float particleAlphaY = lerp(system->bodies[p].last_y_pos, system->bodies[p].y_pos, alphaTime);
        float red = MAX(0, MIN(255, system->colors[p].red_pos));
        float green = MAX(0, MIN(255, system->colors[p].green_pos));
        float blue = MAX(0, MIN(255, system->colors[p].blue_pos));
        float alpha = MAX(0, MIN(255, system->colors[p].alpha_pos));
        u8 red8 = (u8)red;
        u8 green8 = (u8)green;
        u8 blue8 = (u8)blue;
        u8 alpha8 = (u8)alpha;
        system->colors[p].pixel_color = red8 | green8 << 8 | blue8 << 16 | alpha8 << 24;

        if (!system->images[p])
        { imageSetPixel(buffer, particleAlphaX, particleAlphaY, system->colors[p].pixel_color); }
    }
}

#endif