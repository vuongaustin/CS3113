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
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
#include <vector>
#define MAX_BULLETS 30
#include "FlareMap.h"

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;
ShaderProgram program;

float accumulator = 0.0f;
float projectionWidth;
int bulletIndex = 0;
FlareMap flare;
GLuint tileTexture;
enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL};
float tileSize = 0.15f;

GameMode mode = STATE_MAIN_MENU;

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

class SheetSprite {
public:
    SheetSprite() {}
    SheetSprite(unsigned int textureID, float u, float v, float width, float height, float
                size) : textureID(textureID), u(u), v(v), width(width), height(height), size(size) {}
    
    void Draw(ShaderProgram &program);
    
    float size;
    unsigned int textureID;
    float u;
    float v;
    float width;
    float height;
};

void SheetSprite::Draw(ShaderProgram &program) {
    glBindTexture(GL_TEXTURE_2D, textureID);
    GLfloat texCoords[] = {
        u, v+height,
        u+width, v,
        u, v,
        u+width, v,
        u, v+height,
        u+width, v+height
    };
    float aspect = width / height;
    float vertices[] = {
        -0.5f * size * aspect, -0.5f * size,
        0.5f * size * aspect, 0.5f * size,
        -0.5f * size * aspect, 0.5f * size,
        0.5f * size * aspect, 0.5f * size,
        -0.5f * size * aspect, -0.5f * size ,
        0.5f * size * aspect, -0.5f * size};
    // draw our arrays
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program.texCoordAttribute);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
}

void drawtiles () {
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    for(int y=0; y < flare.mapHeight; y++) {
        for(int x=0; x < flare.mapWidth; x++) {
            if (flare.mapData[y][x] != 0){
                float u = (float)(((int)flare.mapData[y][x]) % 16) / (float) 16;
                float v = (float)(((int)flare.mapData[y][x]) / 16) / (float) 8;
                float spriteWidth = 1.0f/(float)16;
                float spriteHeight = 1.0f/(float)8;
                vertexData.insert(vertexData.end(), {
                    tileSize * x, -tileSize * y,
                    tileSize * x, (-tileSize * y)-tileSize,
                    (tileSize * x)+tileSize, (-tileSize * y)-tileSize,
                    tileSize * x, -tileSize * y,
                    (tileSize * x)+tileSize, (-tileSize * y)-tileSize,
                    (tileSize * x)+tileSize, -tileSize * y
                });
                texCoordData.insert(texCoordData.end(), {
                    u, v,
                    u, v+(spriteHeight),
                    u+spriteWidth, v+spriteHeight,
                    
                    u, v,
                    u+spriteWidth, v+spriteHeight,
                    u+spriteWidth, v
                });
            }
            
        }
    }
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    program.SetModelMatrix(modelMatrix);
    glUseProgram(program.programID);
    glBindTexture(GL_TEXTURE_2D, tileTexture);
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program.texCoordAttribute);
    glDrawArrays(GL_TRIANGLES, 0, (int)vertexData.size() / 2);
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
}

void DrawSpriteSheetSprite(ShaderProgram &program, int index, int spriteCountX,
                           int spriteCountY, float sizex, float sizey) {
    float u = (float)(((int)index) % spriteCountX) / (float) spriteCountX;
    float v = (float)(((int)index) / spriteCountX) / (float) spriteCountY;
    float spriteWidth = 1.0/(float)spriteCountX;
    float spriteHeight = 1.0/(float)spriteCountY;
    float texCoords[] = {
        u, v+spriteHeight,
        u+spriteWidth, v,
        u, v,
        u+spriteWidth, v,
        u, v+spriteHeight,
        u+spriteWidth, v+spriteHeight
    };
    float vertices[] = {-0.5f * sizex, -0.5f * sizey, 0.5f * sizex, 0.5f * sizey, -0.5f * sizex, 0.5f * sizey, 0.5f * sizex, 0.5f * sizey,  -0.5f * sizex,
        -0.5f * sizey, 0.5f * sizex, -0.5f * sizey};
    // draw this data
    glBindTexture(GL_TEXTURE_2D, tileTexture);
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program.texCoordAttribute);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
}

class Player {
public:
    float x;
    float y;
    float bottom;
    float sizex;
    float sizey;
    float velx;
    float vely;
    Player () {
        velx = 0.0f;
        vely = -1.0f;
    }
    void Draw () {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, 0.0f));
        program.SetModelMatrix(modelMatrix);
        DrawSpriteSheetSprite(program, 98, 16, 8, sizex, sizey);
    }
    float getbottom () {
        return y - (sizey / 2);
    }
};

class Key {
public:
    float x;
    float y;
    float sizex;
    float sizey;
    void Draw () {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, 0.0f));
        program.SetModelMatrix(modelMatrix);
        DrawSpriteSheetSprite(program, 86, 16, 8, sizex, sizey);
    }
};

std::pair<int, int> worldToTileCoordinates(float worldX, float worldY) {
    int gridX = (int)(worldX / tileSize);
    int gridY = (int)(worldY / -tileSize);
    return {gridY, gridX};
}

std::pair<float, float> tileToWorldCoordinates(float worldX, float worldY) {
    float gridX = (int)(worldX * tileSize);
    float gridY = (int)(worldY * -tileSize);
    return {gridY, gridX};
}

int main(int argc, char *argv[]) {
    float width = 720;
    float height = 480;
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Homework 4", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
    glViewport(0, 0, width, height);
    program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    

    SDL_Event event;
    bool done = false;
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glClearColor(0.2745f, 0.5098f, 0.7058f, 1.0f); //background color set
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    float aspectRatio = (float)width / (float)height; //dimensions of window
    float projectionHeight = 2.0f;
    projectionWidth = projectionHeight * aspectRatio;
    float projectionDepth = 1.0f;
    projectionMatrix = glm::ortho(-projectionWidth, projectionWidth, -projectionHeight, projectionHeight, -projectionDepth, projectionDepth);
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
    tileTexture = LoadTexture(RESOURCE_FOLDER"arne_sprites.png");
    
    Player p;
    p.sizex = tileSize;
    p.sizey = tileSize;
    p.bottom = p.y - (p.sizey / 2);
    Key k;
    k.sizex = tileSize;
    k.sizey = tileSize;
    
    flare.Load(RESOURCE_FOLDER"hw4.txt");
    for (int i = 0; i < flare.entities.size(); i++) {
        float x = flare.entities[i].x;
        float y = flare.entities[i].y;
        x = x * tileSize;
        y = y * -tileSize;
        if (flare.entities[i].type == "Player") {
            p.x = x;
            p.y = y;
        } else {
            k.x = x;
            k.y = y;
        }
    }
    
    float lastFrameTicks = 0.0f;
    
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
        
        
        drawtiles();
        viewMatrix = glm::mat4(1.0f);
        viewMatrix = glm::translate(viewMatrix, glm::vec3(std::min(std::max(-p.x, -108*tileSize), -projectionWidth), std::min(-p.y, projectionHeight), 0.0f));
        program.SetViewMatrix(viewMatrix);
        
        p.Draw();
        k.Draw();
        
        p.y += elapsed * p.vely;
        
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        if(keys[SDL_SCANCODE_RIGHT]) {
            p.x += elapsed * 1.5f;
        }
        if(keys[SDL_SCANCODE_LEFT]) {
            p.x -= elapsed * 1.5f;
        }
        
        float playerx = p.x, playery = p.getbottom(); // bottom collision
        std::pair<int, int> coord = worldToTileCoordinates(playerx, playery);
        if (coord.first < flare.mapHeight && coord.second < flare.mapWidth) {
            int currtile = flare.mapData[coord.first][coord.second];
            if (currtile != 0) {
                p.y += fabs((-tileSize * coord.first) - (p.y - p.sizey/2));
            }
        }
        float pw = abs(p.x - k.x) - ((p.sizex + k.sizex) / 2);
        float ph = abs(p.y - k.y) - ((p.sizey + k.sizey) / 2);
        if (pw < 0 && ph < 0) {
            k.x = -2000.0f;
        }
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
