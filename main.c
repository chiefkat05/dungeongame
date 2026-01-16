/**
 * 
 * 
 * optimization in memory.h
 * optimization in image.h
 * optimization in obj2d.h
 *
 * Making naming convention and other things consistent across the code (write down the rules and keep with them in the future)
 *          Do this soon so it doesn't become a problem
 * 
 * 
 * NEXT: collision detection/response
 * NEXT: write game pseudocode
 * NEXT: game data setup + scene system (and memory management)
 * NEXT: main menu draft art
 * NEXT: main menu code + buttons
 * 
 * necessary image manipulations:
 *      special image stretch function that keeps buttons looking crisp
 *      
 *      post-processing: vignette (), bloom, ssao?
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
#include "obj2d.h"

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
    int win_width, win_height;
} WindowData;
static void framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    WindowData *data = (WindowData *)glfwGetWindowUserPointer(window);
    resizeOpenGL(width, height);
    data->win_width = width;
    data->win_height = height;
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

    const u8 *hats = glfwGetJoystickHats(jid, &hatCount);
    for (i = 0; i < hatCount; ++i)
    {
        inputSet(&winData->input[jid], PAD_HAT0 + i, hats[i]);
    }

    const u8 *buttons = glfwGetJoystickButtons(jid, &buttonCount);
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
static void mousePosCallback(GLFWwindow *window, double xpos, double ypos)
{
    WindowData *data = (WindowData *)glfwGetWindowUserPointer(window);

    float screen_x, screen_y, screen_width, screen_height;
    letterbox(data->win_width, data->win_height, SCREENWIDTH, SCREENHEIGHT, &screen_x, &screen_y, &screen_width, &screen_height);

    xpos -= screen_x;
    ypos -= screen_y;
    xpos /= screen_width;
    ypos /= screen_height;
    inputSet(&data->input[0], MOUSE_X, xpos * INT16_MAX);
    inputSet(&data->input[0], MOUSE_Y, (1.0f - ypos) * INT16_MAX);
}
static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    WindowData *data = (WindowData *)glfwGetWindowUserPointer(window);

    inputSet(&data->input[0], MOUSE_BUTTON_LEFT + button, action == GLFW_PRESS ? PRESSED : RELEASED);
}

#define TILEMAP_WIDTH 16
#define TILEMAP_HEIGHT 12
typedef struct
{
    i8 tilemap[TILEMAP_HEIGHT][TILEMAP_WIDTH];
    Rect tileCol[TILEMAP_HEIGHT][TILEMAP_WIDTH];
} TileMap;
typedef struct
{
    TileMap *maps;
    u32 currentMap;
} WorldMap;

static void tileMapInit(TileMap *map, image *tilemapImage, image *outImage)
{
    imageClear(outImage);
    int y, x;
    for (y = 0; y < TILEMAP_HEIGHT; ++y)
    {
        for (x = 0; x < TILEMAP_WIDTH; ++x)
        {
            if (map->tilemap[TILEMAP_HEIGHT - y - 1][x] < 0) { continue; }

            SPRITE_COPY_TO_IMAGE(tilemapImage, outImage, x * SPRITE_EDGE, y * SPRITE_EDGE, map->tilemap[TILEMAP_HEIGHT - y - 1][x], 1, 1, 1);
            map->tileCol[TILEMAP_HEIGHT - y - 1][x] = (Rect){{x * SPRITE_EDGE + SPRITE_EDGE / 2.0f, y * SPRITE_EDGE + SPRITE_EDGE / 2.0f},
                        {SPRITE_EDGE, SPRITE_EDGE}};
        }
    }
}

int main()
{
    glfwInit();

    WindowData winData = {.win_width = WINDOW_WIDTH, .win_height = WINDOW_HEIGHT};
    glfwWindowHint(GLFW_POSITION_X, -400);
    glfwWindowHint(GLFW_POSITION_Y, 0);
    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "window", 0, 0);
    glfwSetWindowUserPointer(window, &winData);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyboardCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, mousePosCallback);
    
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

    TileMap screenA = {.tilemap =
        {
        { 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1},
        { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1},
        { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1},
        { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1},
        { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1},
        { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1},
        { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1},
        { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1},
        { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1},
        { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1},
        { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1},
        { 1,  1,  1,  1,  1,  1,  1, -1, -1,  1,  1,  1,  1,  1,  1,  1}}};
    TileMap screenB = {.tilemap =
        {{ 1,  1,  1,  1,  1,  1,  1, -1, -1,  1,  1,  1,  1,  1,  1,  1},
        { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1},
        { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1},
        { 1, -1, -1, -1, -1, -1,  2, -1, -1,  2, -1, -1, -1, -1, -1,  1},
        { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1},
        { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1},
        { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1},
        { 1, -1, -1, -1, -1, -1,  2,  2,  2,  2, -1, -1, -1, -1, -1,  1},
        { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1},
        { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1},
        { 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1},
        { 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1}}};


    image tilemapImg = imageMakeFromBMP(&globalMemory, "atlas.bmp");
    image levelImg = makeImage(&globalMemory, TILEMAP_WIDTH * SPRITE_EDGE, TILEMAP_HEIGHT * SPRITE_EDGE);

    WorldMap world = {};
    world.maps = memGrab(&globalMemory, MEGABYTES(2));
    world.maps[0] = screenA;
    world.maps[1] = screenB;

    tileMapInit(&world.maps[world.currentMap], &tilemapImg, &levelImg);

    Obj2DPool objects = {};
    Obj2DPool bullets = {};

    const float plSpawn = 120.0f;
    Obj2D *playerObj = obj2DPoolAlloc(&objects);
    *playerObj = obj2D(plSpawn, plSpawn, 16.0f, 16.0f);

    float deltaTime, currentTime, previousTime, accumulatedTime = 0.0f;
    currentTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        inputUpdate(&winData.input[0]);

        glfwPollEvents();
        joystickPoll(&winData, 0);

        previousTime = currentTime;
        currentTime = glfwGetTime();
        deltaTime = currentTime - previousTime;
        accumulatedTime += deltaTime;

        glClearColor(0.2, 0.2, 0.3, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        
        vec2 playerMovement = {0.0f, 0.0f};
        
        /* controller movement broken? */
        // playerMovement = addVec2(playerMovement, (vec2){inputGet(&winData.input[0], PAD_AXIS0), -inputGet(&winData.input[0], PAD_AXIS1)});
        if (inputGet(&winData.input[0], KEY_A))
        { playerMovement = addVec2(playerMovement, (vec2){-1.0f, 0.0f}); }
        if (inputGet(&winData.input[0], KEY_D))
        { playerMovement = addVec2(playerMovement, (vec2){1.0f, 0.0f}); }
        if (inputGet(&winData.input[0], KEY_S))
        { playerMovement = addVec2(playerMovement, (vec2){0.0f, -1.0f}); }
        if (inputGet(&winData.input[0], KEY_W))
        { playerMovement = addVec2(playerMovement, (vec2){0.0f, 1.0f}); }

        if (inputJustPressed(&winData.input[0], KEY_SPACE))
        {
            Obj2D *playerBullet = obj2DPoolAlloc(&bullets);
            *playerBullet = obj2D(playerObj->pos.x, playerObj->pos.y, 8, 8);
            obj2DMove(playerBullet, (vec2){200.0f, 0.0f});
            obj2DSetImage(playerBullet, &tilemapImg);
        }
        obj2DPoolResetUpdate(&bullets);
        obj2DPoolResetUpdate(&objects);

        playerMovement = multiScalarVec2(normalizeVec2(playerMovement), 100.0f);
        obj2DMove(playerObj, playerMovement);

        int tcolx, tcoly;
        while (accumulatedTime > PHYSICS_TICK)
        {
            for (tcoly = 0; tcoly < TILEMAP_HEIGHT; ++tcoly)
            {
                for (tcolx = 0; tcolx < TILEMAP_WIDTH; ++tcolx)
                {
                    world.maps[world.currentMap].tileCol[tcoly][tcolx].colliding = false;
                    obj2DCollisionResponseRect(playerObj, &world.maps[world.currentMap].tileCol[tcoly][tcolx]);
                }
            }
            obj2DPoolUpdate(&objects);
            obj2DPoolUpdate(&bullets);

            obj2DPoolRule(&bullets, obj2DRule_OutOfScreenDeath);

            if (playerObj->pos.y < 0)
            {
                world.currentMap = 1;
                tileMapInit(&world.maps[world.currentMap], &tilemapImg, &levelImg);
                obj2DPut(playerObj, playerObj->pos.x, SCREENHEIGHT);
            }
            if (playerObj->pos.y > SCREENHEIGHT)
            {
                world.currentMap = 0;
                tileMapInit(&world.maps[world.currentMap], &tilemapImg, &levelImg);
                obj2DPut(playerObj, playerObj->pos.x, 0);
            }

            accumulatedTime -= PHYSICS_TICK;
        }

        imageFillBlack(&screenBuffer);
        imageCopyToImage(&levelImg, &screenBuffer, 0, 0, 0, 0, -1, -1);

        vec2 pVec = obj2DGetPosition(playerObj, accumulatedTime);
        imageSetRect(&screenBuffer, roundFlt(pVec.x) - SPRITE_EDGE / 2.0f, roundFlt(pVec.y) - SPRITE_EDGE / 2.0f,
        roundFlt(pVec.x) + SPRITE_EDGE / 2.0f, roundFlt(pVec.y) + SPRITE_EDGE / 2.0f, 0.8, 0.8, 0.5, 1.0);
        imageSetPixel(&screenBuffer, pVec.x, pVec.y, 1.0, 0.0, 0.0, 1.0);

        obj2DPoolDraw(&bullets, &screenBuffer, accumulatedTime);
        obj2DPoolDraw(&objects, &screenBuffer, accumulatedTime);

        imageVignette(&screenBuffer, 15.0f, 0.25f);
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