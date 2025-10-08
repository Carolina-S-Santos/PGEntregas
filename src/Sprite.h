#ifndef SPRITE_H
#define SPRITE_H

#include <iostream>
#include <string>
#include <assert.h>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW - porque inclui a OpenGL
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

class Sprite
{
public:
    Sprite();
    ~Sprite();
    void initialize(GLuint shaderID, GLuint texID, int nAnimations, int nFrames, vec3 pos, vec3 dimensions, float angle = 0.0);
    void update();
    void draw();
    void moveRight(float deltaTime);
    void moveLeft(float deltaTime);
    vec3 getPos() { return pos; }
    void setPos(vec3 newPos) { pos = newPos; }

private:
    GLuint VAO;      // id do VAO da geometria
    GLuint texID;    // id da textura
    vec3 pos;        // posicao
    vec3 dimensions; // fatores de escala
    float angle;     // angulo para rotação no eixo z
    GLuint shaderID; // para acessar o shader
    int nAnimations, nFrames, iAnimations, iFrames;
    vec2 d;

    float lastTime, FPS;
    float vel;

    GLuint setupGeometry();
};

#endif