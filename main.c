/**
 * 
 * image manipulation functions (vertical flip, image/screen shake, color multiplication, etc.)
 * ^ specific effect that would be nice: 'glow' effect that goes around the edge of an image and makes a blended outward spread
 * 
 * audio portaudio/openal/something else idk
 * 
 * programmer graphics for demo game
 *      base gui
 *      main menu
 *      character select
 *      player
 *      enemy
 *      level tileset
 * 
 * enemy logic
 * 
 * player movement rules
 * 
 * collision detection
 * 
 * optimization in memory.h
 * optimization in image.h
 * optimization in physics.h
 * 
 * 
 * NEXT: unified control system
 * NEXT: game data setup + scene system (and memory management)
 * NEXT: main menu draft art
 * NEXT: main menu code + buttons
 * NEXT: collision detection/response
 * 
 * 
 * 
 * 
 * HEY: This is the *most* important thing here. You want a good-looking game:
 *          You need functioning alpha blending, some nice color and vibrancy post-processing, and image-shake.
 *          You need nice sound, so study vytal. Tracks need a constant backing with reverb, but make the main melody clear.
 *          You need good sound effects, so do sound effect study since you have no knowledge on that.
 * 
 * alpha implementation:
 *      first re-implement it as part of the pixels (so make sure every pixel records it)
 *      then skip pixels with a 0 alpha
 *      then implement a simple blending for every image copy
 * 
 * necessary image manipulations:
 *      special image stretch function that keeps buttons looking crisp
 *      
 *      post-processing: vignette (), bloom, 
 * 
 * windows/emscripten build (not until game is basically done)
 * 
 * 
**/

#include "chiefkat_opengl.h"
#include <GLFW/glfw3.h>
#include "memory.h"
#include "def.h"
#include "image.h"
#include "particle.h"
#include "physics.h"

#define PRESSED 1
#define RELEASED 0
#define UP 1
#define UPRIGHT 3
#define RIGHT 2
#define DOWNRIGHT 6
#define DOWN 4
#define DOWNLEFT 12
#define LEFT 8
#define UPLEFT 9
typedef enum
{
    MOUSE_ID, /* ?? test with multiple mice */
    MOUSE_X, MOUSE_Y, MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT,

    KEYBOARD_ID, /* ?? test with multiple keyboards */
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I,
    KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R,
    KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    KEY_SHIFT, KEY_CTRL, KEY_TAB, KEY_ESC, KEY_SPACE, KEY_ENTER,

    PAD_ID, /* test with multiple pads (should be able to take two or more inputs at the same time) */
    PAD_AXIS0, PAD_AXIS1, PAD_AXIS2, PAD_AXIS3, PAD_AXIS4, PAD_AXIS5,
    PAD_AXIS6, PAD_AXIS7, PAD_AXIS8, PAD_AXIS9, PAD_AXIS10, PAD_AXIS11,
    PAD_HAT0, PAD_HAT1, PAD_HAT2, PAD_HAT3, PAD_HAT4, PAD_HAT5, PAD_HAT6, PAD_HAT7, PAD_HAT8,
    PAD_BUTTONA, PAD_BUTTONB, PAD_BUTTONX, PAD_BUTTONY, PAD_BUTTONL, PAD_BUTTONR,
    PAD_BUTTONSELECT, PAD_BUTTONSTART, PAD_BUTTONHOME, PAD_BUTTONSTICKL, PAD_BUTTONSTICKR,
    PAD_BUTTON_DPADUP, PAD_BUTTON_DPADRIGHT, PAD_BUTTON_DPADDOWN, PAD_BUTTON_DPADLEFT,

    JOYSTICK_ID, /* ditto */
    JOYSTICK_UP,

    total_input_count
} InputCode;

typedef struct
{
    i16 currentState[total_input_count];
    i16 lastState[total_input_count];
} InputState;

void inputSet(InputState *state, InputCode input, i16 value)
{
    state->currentState[input] = value;
}
static void inputUpdate(InputState *state)
{
    int i;
    for (i = 0; i < total_input_count; ++i)
    {
        state->lastState[i] = state->currentState[i];
    }
}
i16 inputGet(InputState *state, InputCode input)
{
    return state->currentState[input];
}
bool inputJustReleased(InputState *state, InputCode input)
{
    if (state->currentState[input] != state->lastState[input] && state->currentState[input] == RELEASED)
    {
        return true;
    }
    return false;
}
bool inputJustPressed(InputState *state, InputCode input)
{
    if (state->currentState[input] != state->lastState[input] && state->currentState[input] == PRESSED)
    {
        return true;
    }
    return false;
}


static void imageCopyToScreen(image *a, u32 screenLayer)
{
    glBindTexture(GL_TEXTURE_2D, screenLayer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, a->width, a->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, a->data);
    glGenerateMipmap(GL_TEXTURE_2D);
}

static void resizeOpenGL(int width, int height)
{
    float screen_offset_x, screen_offset_y, screen_width, screen_height;
    letterbox(width, height, SCREENWIDTH, SCREENHEIGHT, &screen_offset_x, &screen_offset_y, &screen_width, &screen_height);
    glViewport(screen_offset_x, screen_offset_y, screen_width, screen_height);
}

/* implement this */
#define MAX_PLAYERS 8
typedef struct
{
    InputState input[MAX_PLAYERS];
} WindowData;
static void framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    resizeOpenGL(width, height);
}

/** TODO: MAKE THIS CONFIGUREABLE */
#define JOYSTICK_AXES_DEADZONE 0.1
static void joystickPoll(WindowData *winData, int jid)
{
    if (glfwJoystickPresent(jid) == GLFW_FALSE)
    {
        return;
    }
    int i, axesCount, hatCount, buttonCount;
    const float *axes = glfwGetJoystickAxes(jid, &axesCount);
    for (i = 0; i < axesCount; ++i)
    {
        float axesValue = axes[i];
        if(ABS(axes[i]) < JOYSTICK_AXES_DEADZONE)
        {
            axesValue = 0.0f;
        }
        InputCode inputCode = PAD_AXIS0 + i;
        i16 inputValue = (i16)(axesValue * (float)INT16_MAX);

        inputSet(&winData->input[jid], inputCode, inputValue);
    }

    u8 *hats = glfwGetJoystickHats(jid, &hatCount);
    for (i = 0; i < hatCount; ++i)
    {
        inputSet(&winData->input[jid], PAD_HAT0 + i, hats[i]);
    }

    u8 *buttons = glfwGetJoystickButtons(jid, &buttonCount);
    for (i = 0; i < buttonCount; ++i)
    {
        inputSet(&winData->input[jid], PAD_BUTTONA + i, buttons[i]);
    }
}
static void keyboardCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_REPEAT)
    { return; }

    WindowData *data = (WindowData *)glfwGetWindowUserPointer(window);
    switch(scancode)
    {
        case 1: { inputSet(&data->input[0], KEY_ESC, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 2: { inputSet(&data->input[0], KEY_1, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 3: { inputSet(&data->input[0], KEY_2, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 4: { inputSet(&data->input[0], KEY_3, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 5: { inputSet(&data->input[0], KEY_4, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 6: { inputSet(&data->input[0], KEY_5, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 7: { inputSet(&data->input[0], KEY_6, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 8: { inputSet(&data->input[0], KEY_7, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 9: { inputSet(&data->input[0], KEY_8, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 10: { inputSet(&data->input[0], KEY_9, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 11: { inputSet(&data->input[0], KEY_0, action == GLFW_PRESS ? PRESSED : RELEASED); } break;

        case 15: { inputSet(&data->input[0], KEY_TAB, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 16: { inputSet(&data->input[0], KEY_Q, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 17: { inputSet(&data->input[0], KEY_W, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 18: { inputSet(&data->input[0], KEY_E, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 19: { inputSet(&data->input[0], KEY_R, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 20: { inputSet(&data->input[0], KEY_T, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 21: { inputSet(&data->input[0], KEY_Y, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 22: { inputSet(&data->input[0], KEY_U, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 23: { inputSet(&data->input[0], KEY_I, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 24: { inputSet(&data->input[0], KEY_O, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 25: { inputSet(&data->input[0], KEY_P, action == GLFW_PRESS ? PRESSED : RELEASED); } break;

        case 28: { inputSet(&data->input[0], KEY_ENTER, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 29: { inputSet(&data->input[0], KEY_CTRL, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 30: { inputSet(&data->input[0], KEY_A, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 31: { inputSet(&data->input[0], KEY_S, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 32: { inputSet(&data->input[0], KEY_D, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 33: { inputSet(&data->input[0], KEY_F, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 34: { inputSet(&data->input[0], KEY_G, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 35: { inputSet(&data->input[0], KEY_H, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 36: { inputSet(&data->input[0], KEY_J, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 37: { inputSet(&data->input[0], KEY_K, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 38: { inputSet(&data->input[0], KEY_L, action == GLFW_PRESS ? PRESSED : RELEASED); } break;

        case 42: { inputSet(&data->input[0], KEY_SHIFT, action == GLFW_PRESS ? PRESSED : RELEASED); } break;

        case 44: { inputSet(&data->input[0], KEY_Z, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 45: { inputSet(&data->input[0], KEY_X, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 46: { inputSet(&data->input[0], KEY_C, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 47: { inputSet(&data->input[0], KEY_V, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 48: { inputSet(&data->input[0], KEY_B, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 49: { inputSet(&data->input[0], KEY_N, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 50: { inputSet(&data->input[0], KEY_M, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        case 51: { inputSet(&data->input[0], KEY_Z, action == GLFW_PRESS ? PRESSED : RELEASED); } break;

        case 57: { inputSet(&data->input[0], KEY_SPACE, action == GLFW_PRESS ? PRESSED : RELEASED); } break;
        default: break;
    }
}

int main()
{
    glfwInit();

    WindowData winData = {};
    glfwWindowHint(GLFW_POSITION_X, -400);
    glfwWindowHint(GLFW_POSITION_Y, 0);
    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "window", 0, 0);
    glfwSetWindowUserPointer(window, &winData);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyboardCallback);
    
    glfwMakeContextCurrent(window);

    glewInit();

    resizeOpenGL(WINDOW_WIDTH, WINDOW_HEIGHT);

    Arena globalMemory;
    memoryInit(&globalMemory, MEGABYTES(32));

    u32 shader = makeShaderProgram("shader.vs", "shader.fs");

    u32 vao = makeVAO();
    u32 vbo = makeVBO(vertices, sizeof(vertices));
    addVBOAttributes(vao, vbo, 0, 3, 5, 0);
    addVBOAttributes(vao, vbo, 1, 2, 5, 3);

    u32 texture;
    glGenTextures(1, &texture);

    image screenBuffer = makeImage(&globalMemory, SCREENWIDTH, SCREENHEIGHT);

    image dogImg = imageMakeFromBMP(&globalMemory, "dog.bmp");

    image tilemapImg = imageMakeFromBMP(&globalMemory, "atlas.bmp");

    int spx, spy, level_width = 16, level_height = 16;
    image levelImg = makeImage(&globalMemory, 16 * level_width, 16 * level_height);

    for (spy = 0; spy < level_width; ++spy)
    {
        for (spx = 0; spx < level_height; ++spx)
        {
            if (spx > 0 && spx < level_width - 1 && spy > 0 && spy < level_height - 1)
            { continue; }
            SPRITE_COPY_TO_IMAGE(&tilemapImg, &levelImg, spx * SPRITE_EDGE, spy * SPRITE_EDGE, 2, 1, 1, 1);
        }
    }

    Obj2D playerObj = makeObj(0.0f, 0.0f, 16.0f, 16.0f);
    image playerImg = makeImage(&globalMemory, playerObj.width, playerObj.height);
    imageScaleToImage(&dogImg, &playerImg);

    Obj2D badSquareObj = makeObj(0.0f, 0.0f, 16.0f, 16.0f);

    Arena tempObjMemory;
    Obj2D playerShot = makeObj(0.0f, 0.0f, 4.0f, 4.0f);
    image alphaTestBMP = imageMakeFromBMP(&globalMemory, "alpha.bmp");
    image playerShotImg = makeImage(&globalMemory, playerShot.width, playerShot.height);
    imageScaleToImage(&alphaTestBMP, &playerShotImg);
    bool playerShotActivated = false;

    image vignette = makeImage(&globalMemory, SCREENWIDTH, SCREENHEIGHT);
    imageFillVignette(&vignette, 0.2f);


    ParticleSystem left_flame = {
        .type = PARTICLESYSTEM_FIRE,
        .particle_limit = MAX_PARTICLE_COUNT,
    };
    ParticleSystem right_flame = {
        .type = PARTICLESYSTEM_FIRE,
        .particle_limit = MAX_PARTICLE_COUNT,
    };

    float deltaTime, currentTime, previousTime, accumulatedTime = 0.0f;
    currentTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        joystickPoll(&winData, 0);

        previousTime = currentTime;
        currentTime = glfwGetTime();
        deltaTime = currentTime - previousTime;
        accumulatedTime += deltaTime;

        glClearColor(0.2, 0.2, 0.3, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        objMove(&playerObj, 0.0f, 0.0f);

        objMoveX(&playerObj, inputGet(&winData.input[0], PAD_AXIS0) / 328);
        objMoveY(&playerObj, -inputGet(&winData.input[0], PAD_AXIS1) / 328);

        if (glfwGetKey(window, GLFW_KEY_A)) { objMoveX(&playerObj, -100.0f); }
        if (glfwGetKey(window, GLFW_KEY_D)) { objMoveX(&playerObj,  100.0f); }
        if (glfwGetKey(window, GLFW_KEY_S)) { objMoveY(&playerObj, -100.0f); }
        if (glfwGetKey(window, GLFW_KEY_W)) { objMoveY(&playerObj,  100.0f); }

        if ((inputGet(&winData.input[0], PAD_AXIS3) || inputGet(&winData.input[0], PAD_AXIS4)))
        {
            playerShot.x_pos = playerObj.x_pos + inputGet(&winData.input[0], PAD_AXIS3) / 800;
            playerShot.y_pos = playerObj.y_pos - inputGet(&winData.input[0], PAD_AXIS4) / 800;
        }

        objMoveTarget(&badSquareObj, playerObj.x_pos, playerObj.y_pos, 10.0f);

        while (accumulatedTime > PHYSICS_TICK)
        {
            objUpdate(&playerObj);
            objUpdate(&badSquareObj);
            objUpdate(&playerShot);
            particleSystemGenerate(&left_flame, 1, 0.001f, 40, 40, 4, 4);
            particleSystemGenerate(&right_flame, 1, 0.001f, SCREENWIDTH - 40, SCREENWIDTH - 40, 4, 4);
            particleSystemUpdate(&left_flame, &screenBuffer);
            particleSystemUpdate(&right_flame, &screenBuffer);

            if (playerShot.x_pos < 0 || playerShot.x_pos > SCREENWIDTH || playerShot.y_pos < 0 || playerShot.y_pos > SCREENHEIGHT)
            {
                playerShotActivated = false;
            }

            accumulatedTime -= PHYSICS_TICK;
        }

        playerObj.x_pos = MIN(playerObj.x_pos, SCREENWIDTH - playerObj.width);
        playerObj.x_pos = MAX(playerObj.x_pos, 0);
        playerObj.y_pos = MIN(playerObj.y_pos, SCREENHEIGHT - playerObj.height);
        playerObj.y_pos = MAX(playerObj.y_pos, 0);

        imageFillBlack(&screenBuffer);

        SPRITE_COPY_TO_IMAGE(&tilemapImg, &screenBuffer, objGetX(&badSquareObj, accumulatedTime), objGetY(&badSquareObj, accumulatedTime), 1, 1, 1, 1);

        imageScaleToImage(&dogImg, &screenBuffer);
        imageCopyToImage(&playerImg, &screenBuffer, objGetX(&playerObj, accumulatedTime), objGetY(&playerObj, accumulatedTime), DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE);
        imageCopyToImage(&playerShotImg, &screenBuffer, objGetX(&playerShot, accumulatedTime), objGetY(&playerShot, accumulatedTime), 0, 0, -1, -1);

        imageCopyToImage(&vignette, &screenBuffer, 0, 0, -1, -1, -1, -1);

        imageCopyToImage(&levelImg, &screenBuffer, 0, 0, 0, 0, DONT_CARE, DONT_CARE);

        particleSystemDraw(&left_flame, &screenBuffer, accumulatedTime);
        particleSystemDraw(&right_flame, &screenBuffer, accumulatedTime);

        /**
         * 
         * drive to studio apartment place and check it out/ask about pricing and availability
         * play games to study
         * work on game
         * 
         */

        imageCopyToScreen(&screenBuffer, texture);

        glUseProgram(shader);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
    }

    memoryClear(&globalMemory);

    return 0;
}