#ifndef CHIEFKAT_OPENGL_H
#define CHIEFKAT_OPENGL_H

#include <GL/glew.h>
#include "def.h"
#include "memory.h"

static u32 makeVAO()
{
    u32 vao;
    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);
    return vao;
}
static u32 makeVBO(float *vertices, u32 size)
{
    u32 vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

    return vbo;
}
static void addVBOAttributes(u32 VAO, u32 VBO, u32 index, u32 size, u32 stride, u32 offset)
{
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(index, size, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void *)(offset * sizeof(float)));
    glEnableVertexAttribArray(index);
}

static u32 makeShaderProgram(const char *vpath, const char *fpath)
{
    int success;
    char infoLog[512];

    Arena localMemory;
    memoryInit(&localMemory, KILOBYTES(8));

    u32 vShader = glCreateShader(GL_VERTEX_SHADER);
    const char *vsrc = readFile(&localMemory, vpath);
    glShaderSource(vShader, 1, &vsrc, NULL);
    glCompileShader(vShader);
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vShader, 512, NULL, infoLog);
        printf("%s\n", infoLog);
    }

    u32 fShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fsrc = readFile(&localMemory, fpath);
    glShaderSource(fShader, 1, &fsrc, NULL);
    glCompileShader(fShader);
    glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fShader, 512, NULL, infoLog);
        printf("%s\n", infoLog);
    }

    u32 program = glCreateProgram();
    glAttachShader(program, vShader);
    glAttachShader(program, fShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        printf("%s\n", infoLog);
    }

    glDeleteShader(vShader);
    glDeleteShader(fShader);

    memoryClear(&localMemory);

    return program;
}

/* This needs to go somewhere */
float vertices[] = {
    -1.0, -1.0, 0.0, 0.0, 0.0,
     1.0, -1.0, 0.0, 1.0, 0.0,
     1.0, 1.0, 0.0, 1.0, 1.0,

    -1.0, -1.0, 0.0, 0.0, 0.0,
     1.0, 1.0, 0.0, 1.0, 1.0,
     -1.0, 1.0, 0.0, 0.0, 1.0
};

#endif