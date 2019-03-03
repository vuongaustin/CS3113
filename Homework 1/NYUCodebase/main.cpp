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

int main(int argc, char *argv[]) {
    float width = 1280;
    float height = 720;
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
    glViewport(0, 0, width, height);
    program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
    textprogram.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

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
    textprogram.SetProjectionMatrix(projectionMatrix);
    textprogram.SetViewMatrix(viewMatrix);
    
    GLuint exhead = LoadTexture(RESOURCE_FOLDER"exodiahead.png");
    GLuint exrightarm = LoadTexture(RESOURCE_FOLDER"exodiarightarm.png");
    GLuint exleftarm = LoadTexture(RESOURCE_FOLDER"exodialeftarm.png");
    GLuint exrightleg = LoadTexture(RESOURCE_FOLDER"exodiarightleg.png");
    GLuint exleftleg = LoadTexture(RESOURCE_FOLDER"exodialeftleg.png");
    
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        glClear(GL_COLOR_BUFFER_BIT);
        
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -0.5f, 0.0f));
        program.SetModelMatrix(modelMatrix);
        glUseProgram(program.programID);
        float vertices[] = {0.5f, 0.5f, 0.0f, -0.5f,-0.5f, 0.5f};
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDisableVertexAttribArray(program.positionAttribute);
        modelMatrix = glm::mat4(1.0f);
        
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.8f, 0.0f));
        textprogram.SetModelMatrix(modelMatrix);
        glUseProgram(textprogram.programID);
        glBindTexture(GL_TEXTURE_2D, exhead);
        float tvertices[] = {-0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5};
        glVertexAttribPointer(textprogram.positionAttribute, 2, GL_FLOAT, false, 0, tvertices);
        glEnableVertexAttribArray(textprogram.positionAttribute);
        float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
        glVertexAttribPointer(textprogram.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(textprogram.texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(textprogram.positionAttribute);
        glDisableVertexAttribArray(textprogram.texCoordAttribute);
        modelMatrix = glm::mat4(1.0f);
        
        modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.1f, 0.65f, 0.0f));
        textprogram.SetModelMatrix(modelMatrix);
        glUseProgram(textprogram.programID);
        glBindTexture(GL_TEXTURE_2D, exrightarm);
        glVertexAttribPointer(textprogram.positionAttribute, 2, GL_FLOAT, false, 0, tvertices);
        glEnableVertexAttribArray(textprogram.positionAttribute);
        glVertexAttribPointer(textprogram.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(textprogram.texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(textprogram.positionAttribute);
        glDisableVertexAttribArray(textprogram.texCoordAttribute);
        modelMatrix = glm::mat4(1.0f);
        
        modelMatrix = glm::translate(modelMatrix, glm::vec3(1.1f, 0.65f, 0.0f));
        textprogram.SetModelMatrix(modelMatrix);
        glUseProgram(textprogram.programID);
        glBindTexture(GL_TEXTURE_2D, exleftarm);
        glVertexAttribPointer(textprogram.positionAttribute, 2, GL_FLOAT, false, 0, tvertices);
        glEnableVertexAttribArray(textprogram.positionAttribute);
        glVertexAttribPointer(textprogram.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(textprogram.texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(textprogram.positionAttribute);
        glDisableVertexAttribArray(textprogram.texCoordAttribute);
        modelMatrix = glm::mat4(1.0f);
        
        modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.1f, -0.65f, 0.0f));
        textprogram.SetModelMatrix(modelMatrix);
        glUseProgram(textprogram.programID);
        glBindTexture(GL_TEXTURE_2D, exrightleg);
        glVertexAttribPointer(textprogram.positionAttribute, 2, GL_FLOAT, false, 0, tvertices);
        glEnableVertexAttribArray(textprogram.positionAttribute);
        glVertexAttribPointer(textprogram.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(textprogram.texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(textprogram.positionAttribute);
        glDisableVertexAttribArray(textprogram.texCoordAttribute);
        modelMatrix = glm::mat4(1.0f);
        
        modelMatrix = glm::translate(modelMatrix, glm::vec3(1.1f, -0.65f, 0.0f));
        textprogram.SetModelMatrix(modelMatrix);
        glUseProgram(textprogram.programID);
        glBindTexture(GL_TEXTURE_2D, exleftleg);
        glVertexAttribPointer(textprogram.positionAttribute, 2, GL_FLOAT, false, 0, tvertices);
        glEnableVertexAttribArray(textprogram.positionAttribute);
        glVertexAttribPointer(textprogram.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(textprogram.texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(textprogram.positionAttribute);
        glDisableVertexAttribArray(textprogram.texCoordAttribute);
        modelMatrix = glm::mat4(1.0f);
        

        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
