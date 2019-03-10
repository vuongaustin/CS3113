#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;
ShaderProgram textprogram;
ShaderProgram program;

GLuint LoadTexture(const char *filePath) {
    int w,h,comp;
    unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
    if(image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }
    GLuint retTexture;
    glGenTextures(1, &retTexture);
    glBindTexture(GL_TEXTURE_2D, retTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(image);
    return retTexture;
}

class Paddle {
public:
    Paddle(int x) {
        pos = glm::vec3(x, 0.0f, 0.0f);
        direction = glm::vec3(0.0f, 1.0f, 0.0f);
        height = 1.0f;
        width = 0.2f;
    }
    void Draw(ShaderProgram& program) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(pos.x, pos.y, 0.0f));
        program.SetModelMatrix(modelMatrix);
        glUseProgram(program.programID);
        float vertices[] = {-0.1, -0.5, 0.1, -0.5, 0.1, 0.5, -0.1, -0.5, 0.1, 0.5, -0.1, 0.5};
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(program.positionAttribute);
    }
    glm::vec3 pos;
    glm::vec3 direction;
    float height;
    float width;
};

class Ball {
public:
    Ball() {
        pos = glm::vec3(0.0f, 0.0f, 0.0f);
        direction = glm::vec3(-1.0f, 1.0f, 0.0f);
        height = 0.2f;
        width = 0.2f;
    }
    void Draw(ShaderProgram& program) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(pos.x, pos.y, 0.0f));
        program.SetModelMatrix(modelMatrix);
        glUseProgram(program.programID);
        float vertices[] = {-0.1, -0.1, 0.1, -0.1, 0.1, 0.1, -0.1, -0.1, 0.1, 0.1, -0.1, 0.1};
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(program.positionAttribute);
    }
    void reset () {
        pos.x = 0.0f;
        pos.y = 0.0f;
        direction.x = -direction.x;
    }
    glm::vec3 pos;
    glm::vec3 direction;
    float height;
    float width;
};

int main(int argc, char *argv[]) {
    float width = 1280;
    float height = 720;
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("PONG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    float lastFrameTicks = 0.0f;
#ifdef _WINDOWS
    glewInit();
#endif
    glViewport(0, 0, width, height);
    program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");

    SDL_Event event;
    bool done = false;
    
    glClearColor(0.2745f, 0.5098f, 0.7058f, 1.0f); //background color set
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    float aspectRatio = (float)width / (float)height; //dimensions of window
    float projectionHeight = 2.0f;
    float projectionWidth = projectionHeight * aspectRatio;
    float projectionDepth = 1.0f;
    projectionMatrix = glm::ortho(-projectionWidth, projectionWidth, -projectionHeight, projectionHeight, -projectionDepth, projectionDepth);
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);

    Paddle leftpaddle(-3.0);
    Paddle rightpaddle(3.0);
    Ball ball;
    
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        glClear(GL_COLOR_BUFFER_BIT);
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;

        leftpaddle.Draw(program);
        rightpaddle.Draw(program);
        ball.Draw(program);

        if(keys[SDL_SCANCODE_UP] && leftpaddle.pos.y + (leftpaddle.height / 2) <= projectionHeight) {
            leftpaddle.pos.y += elapsed * 1.5f;
        }
        if (keys[SDL_SCANCODE_DOWN] && leftpaddle.pos.y - (leftpaddle.height / 2) >= -projectionHeight) {
            leftpaddle.pos.y -= elapsed * 1.5f;
        }
        if (rightpaddle.pos.y + (rightpaddle.height / 2) >= projectionHeight || rightpaddle.pos.y - (rightpaddle.height / 2) <= -projectionHeight) {
            rightpaddle.direction.y = -rightpaddle.direction.y;
        }
    
        rightpaddle.pos.y += rightpaddle.direction.y * elapsed;
        
        
        ball.pos.x += ball.direction.x * elapsed;
        ball.pos.y += ball.direction.y * elapsed;
        
        float pw = abs(leftpaddle.pos.x - ball.pos.x) - ((leftpaddle.width + ball.width) / 2);
        float ph = abs(leftpaddle.pos.y - ball.pos.y) - ((leftpaddle.height + ball.height) / 2);
        
        float pw2 = abs(rightpaddle.pos.x - ball.pos.x) - ((rightpaddle.width + ball.width) / 2);
        float ph2 = abs(rightpaddle.pos.y - ball.pos.y) - ((rightpaddle.height + ball.height) / 2);
        
        if ((pw < 0 && ph < 0) || (pw2 < 0 && ph2 < 0)) { // hits paddle
            ball.direction.x = -ball.direction.x;
        }
        if (ball.pos.y + (ball.height / 2) >= projectionHeight) { // top
            ball.direction.y = -ball.direction.y;
        }
        if (ball.pos.x + (ball.width / 2) >= projectionWidth) {  // right
            // Player won
            std::cout << "Player wins" << std::endl;
            glClearColor(0.1333f, 0.5451f, 0.1333f, 1.0f);
            ball.reset();
        }
        if (ball.pos.y - (ball.height / 2) <= -projectionHeight) {  // bottom
            ball.direction.y = -ball.direction.y;
        }
        if (ball.pos.x - (ball.width / 2) <= -projectionWidth) {  // left
            // CPU won
            std::cout << "CPU wins" << std::endl;
            glClearColor(0.5451f, 0.0, 0.0, 1.0f);
            ball.reset();
        }
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
