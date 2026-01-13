/**
 * 
 * open window sdl/glfw/whatever - DONE
 * 
 * render to buffer -> buffer to image -> image drawn in sdl/opengl/whatever - DONE
 * 
 * input handling - DONE LOL
 * 
 * simple particle system - DONE
 * 
 * image manipulation functions (vertical flip, image/screen shake, color multilication, etc.)
 * ^ specific effect that would be nice: 'glow' effect that goes around the edge of an image and makes a blended outward spread
 * 
 * audio portaudio/openal/something else idk
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

/* I feel icky on this (along with the rest of the opengl code). It should probably also take an index for the texture */
/* offset if I'm going that route, or take less values; but not this strange middle-of-the-road system. */
/* And the name is terrible and possibly misleading unless I am going to change this to be portable. */
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
static void framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    resizeOpenGL(width, height);
}

int main()
{
    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "window", 0, 0);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

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

    screenBuffer = makeImage(&globalMemory, SCREENWIDTH, SCREENHEIGHT);

    image dogImg = imageMakeFromBMP(&globalMemory, "dog.bmp");

    image tilemapImg = imageMakeFromBMP(&globalMemory, "atlas.bmp");

    int spx, spy, level_width = 10, level_height = 10;
    image levelImg = makeImage(&globalMemory, 16 * level_width, 16 * level_height);
    image scaledLevelImg = makeImage(&globalMemory, SCREENWIDTH, SCREENHEIGHT);

    for (spy = 0; spy < level_width; ++spy)
    {
        for (spx = 0; spx < level_height; ++spx)
        {
            if (spx > 0 && spx < level_width - 1 && spy > 0 && spy < level_height - 1)
            { continue; }
            SPRITE_COPY_TO_IMAGE(&tilemapImg, &levelImg, spx, spy, 2, 1, 1, 1);
        }
    }
    imageScaleToImage(&levelImg, &scaledLevelImg);

    Obj2D playerObj = makeObj(0.0f, 0.0f, 64.0f, 64.0f);
    Obj2D badSquare = makeObj(0.0f, 0.0f, 128.0f, 128.0f);

    image playerImg = makeImage(&globalMemory, playerObj.width, playerObj.height);
    imageScaleToImage(&dogImg, &playerImg);

    ParticleSystem particles = {
        .type = PARTICLESYSTEM_FIRE,
        .particle_limit = MAX_PARTICLE_COUNT,
    };

    float deltaTime, currentTime, previousTime, accumulatedTime = 0.0f;
    currentTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        previousTime = currentTime;
        currentTime = glfwGetTime();
        deltaTime = currentTime - previousTime;
        accumulatedTime += deltaTime;

        glClearColor(0.2, 0.2, 0.3, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        objMove(&playerObj, 0.0f, 0.0f);
        if (glfwGetKey(window, GLFW_KEY_A)) { objMoveX(&playerObj, -100.0f); }
        if (glfwGetKey(window, GLFW_KEY_D)) { objMoveX(&playerObj,  100.0f); }
        if (glfwGetKey(window, GLFW_KEY_S)) { objMoveY(&playerObj, -100.0f); }
        if (glfwGetKey(window, GLFW_KEY_W)) { objMoveY(&playerObj,  100.0f); }

        while (accumulatedTime > PHYSICS_TICK)
        {
            objUpdate(&playerObj);
            particleSystemGenerate(&particles, 1, 0.01f, SCREENWIDTH / 2, SCREENWIDTH / 2, SCREENHEIGHT / 2, SCREENHEIGHT / 2);
            particleSystemUpdate(&particles, &screenBuffer);

            accumulatedTime -= PHYSICS_TICK;
        }

        playerObj.x_pos = MIN(playerObj.x_pos, SCREENWIDTH - playerObj.width);
        playerObj.x_pos = MAX(playerObj.x_pos, 0);
        playerObj.y_pos = MIN(playerObj.y_pos, SCREENHEIGHT - playerObj.height);
        playerObj.y_pos = MAX(playerObj.y_pos, 0);

        imageFillBlack(&screenBuffer);
        imageToImage(&scaledLevelImg, &screenBuffer);

        particleSystemDraw(&particles, &screenBuffer, accumulatedTime);

        imageCopyToImage(&playerImg, &screenBuffer, objGetX(&playerObj, accumulatedTime), objGetY(&playerObj, accumulatedTime), DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE);

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