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
GLuint fontTexture;
enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL};

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

class Spongebob {
public:
    SheetSprite sprite; //7up
    glm::vec3 size;
    glm::vec3 pos;
    void Draw (ShaderProgram& program) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(pos.x, pos.y, 0.0f));
        program.SetModelMatrix(modelMatrix);
        glUseProgram(program.programID);
        sprite.Draw(program);
    }
};

class Doodlebob {
public:
    Doodlebob () {
        direction = glm::vec3(-1.0f, 0.0f, 0.0f);
    }
    SheetSprite sprite; //7up
    glm::vec3 size;
    glm::vec3 pos;
    glm::vec3 direction;
    bool living;
    void Draw (ShaderProgram& program) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(pos.x, pos.y, 0.0f));
        program.SetModelMatrix(modelMatrix);
        glUseProgram(program.programID);
        sprite.Draw(program);
    }
};

class Pencil {
public:
    Pencil () {
        direction = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    SheetSprite sprite; //7up
    glm::vec3 size;
    glm::vec3 pos;
    glm::vec3 direction;
    void Draw (ShaderProgram& program) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(pos.x, pos.y, 0.0f));
        program.SetModelMatrix(modelMatrix);
        glUseProgram(program.programID);
        sprite.Draw(program);
    }
};

class GameState {
public:
    Spongebob bob;
    std::vector<Doodlebob> doods;
    std::vector<Pencil> pencils;
};

GameState state;

void UpdateGameLevel (float elapsed) {
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if(keys[SDL_SCANCODE_RIGHT] && state.bob.pos.x + (state.bob.size.x / 2) <= projectionWidth) {
        state.bob.pos.x += elapsed * 1.5f;
    }
    if(keys[SDL_SCANCODE_LEFT] && state.bob.pos.x - (state.bob.size.x / 2) >= -projectionWidth) {
        state.bob.pos.x -= elapsed * 1.5f;
    }
    for (Doodlebob& d : state.doods) {
        if (d.living) {
            d.pos.x += d.direction.x * elapsed * 0.5f;
            if (d.pos.x + (d.size.x / 2) >= projectionWidth) {  // right
                for (Doodlebob& d2 : state.doods) {
                    d2.direction.x *= -1;
                }
            }
            if (d.pos.x - (d.size.x / 2) <= -projectionWidth) {  // left
                for (Doodlebob& d2 : state.doods) {
                    d2.direction.x *= -1;
                }
            }
        }
    }
    for (Pencil& p : state.pencils) {
        p.pos.y += elapsed * p.direction.y;
        for (Doodlebob& d : state.doods) {
            float pw = abs(p.pos.x - d.pos.x) - ((p.size.x + d.size.x) / 2);
            float ph = abs(p.pos.y - d.pos.y) - ((p.size.y + d.size.y) / 2);
            if (pw < 0 && ph < 0) { // hits doodlebob
                d.pos.x = -2000.0f;
                d.living = false;
                p.pos.x = -2000.0f;
            }
        }
    }
}

void UpdateMainMenu (float elapsed) {
    
}

void Update(float elapsed) {
    switch(mode) {
        case STATE_MAIN_MENU:
            UpdateMainMenu(elapsed);
            break;
        case STATE_GAME_LEVEL:
            UpdateGameLevel(elapsed);
            break;
    }
}

void RenderGameLevel (ShaderProgram& program) {
    state.bob.Draw(program);
    for (Doodlebob& d : state.doods) {
        d.Draw(program);
    }
    for (Pencil& p : state.pencils) {
        p.Draw(program);
    }
}

void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing) {
    float character_size = 1.0/16.0f;
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    
    for(int i=0; i < text.size(); i++) {
        int spriteIndex = (int)text[i];
        float texture_x = (float)(spriteIndex % 16) / 16.0f;
        float texture_y = (float)(spriteIndex / 16) / 16.0f;
        
        vertexData.insert(vertexData.end(), {
            ((size+spacing) * i) + (-0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
        });
        texCoordData.insert(texCoordData.end(), {
            texture_x, texture_y,
            texture_x, texture_y + character_size,
            texture_x + character_size, texture_y,
            texture_x + character_size, texture_y + character_size,
            texture_x + character_size, texture_y,
            texture_x, texture_y + character_size,
        });
    }
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    // draw this data (use the .data() method of std::vector to get pointer to data)
    // draw this yourself, use text.size() * 6 or vertexData.size()/2 to get number of vertices
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program.texCoordAttribute);
    glDrawArrays(GL_TRIANGLES, 0, (int)text.size() * 6);
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
}

void RenderMainMenu (ShaderProgram& program) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.05f, 0.0f, 0.0f));
    program.SetModelMatrix(modelMatrix);
    glUseProgram(program.programID);
    DrawText(program, fontTexture, "Me Hoy Minoy Invaders", 0.1f, 0.0f);
}

void Render (ShaderProgram& program) {
    switch(mode) {
        case STATE_MAIN_MENU:
            RenderMainMenu(program);
            break;
        case STATE_GAME_LEVEL:
            RenderGameLevel(program);
            break;
    }
}

void shoot () {
    state.pencils[bulletIndex].pos.x = state.bob.pos.x;
    state.pencils[bulletIndex].pos.y = state.bob.pos.y;
    bulletIndex++;
    if(bulletIndex > MAX_BULLETS-1) {
        bulletIndex = 0;
    }
}

int main(int argc, char *argv[]) {
    float width = 600;
    float height = 850;
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Me Hoy Minoy", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
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
    fontTexture = LoadTexture(RESOURCE_FOLDER"font1.png");


    GLuint spriteSheetTexture = LoadTexture(RESOURCE_FOLDER"spongebobsprite.png");
    SheetSprite spongebobsprite = SheetSprite(spriteSheetTexture, 0.0f/500.0f, 0.0f/500.0f, 74.0f/500.0f, 100.0f/
                           500.0f, 0.4f);
    SheetSprite doodlebobsprite = SheetSprite(spriteSheetTexture, 0.0f/500.0f, 100.0f/500.0f, 138.0f/500.0f, 100.0f/
                                              500.0f, 0.3f);
    SheetSprite pencilsprite = SheetSprite(spriteSheetTexture, 0.0f/500.0f, 200.0f/500.0f, 11.0f/500.0f, 100.0f/
                                              500.0f, 0.2f);
    Spongebob bob;
    bob.sprite = spongebobsprite;
    bob.pos.x = 0.0f;
    bob.pos.y = -1.6f;
    bob.size.x = 2.0f * spongebobsprite.size * (spongebobsprite.width / spongebobsprite.height) * 0.5f;
    bob.size.y = 2.0f * spongebobsprite.size * 0.5f;
    state.bob = bob;
    
    std::vector<Doodlebob> doods;
    for (int i = 0; i < 30; i++) {
        Doodlebob dood;
        dood.sprite = doodlebobsprite;
        dood.size.x = 2.0f * doodlebobsprite.size * (doodlebobsprite.width / doodlebobsprite.height) * 0.5f;
        dood.size.y = 2.0f * doodlebobsprite.size * 0.5f;
        dood.pos.x = -2.5f * dood.size.x + (i % 5) * dood.size.x;
        dood.pos.y = (i / 6) * dood.size.y;
        dood.living = true;
        doods.push_back(dood);
    }
    state.doods = doods;

    std::vector<Pencil> bullets;
    for (int i=0; i < MAX_BULLETS; i++) {
        Pencil p;
        p.sprite = pencilsprite;
        p.pos.y = 0.0f;
        p.pos.x = -2000.0f;
        p.size.x = 2.0f * pencilsprite.size * (pencilsprite.width / pencilsprite.height) * 0.5f;
        p.size.y = 2.0f * pencilsprite.size * 0.5f;
        bullets.push_back(p);
    }
    state.pencils = bullets;
    
    float lastFrameTicks = 0.0f;
    
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
                    shoot();
                }
                if (event.key.keysym.scancode == SDL_SCANCODE_RETURN) {
                    mode = STATE_GAME_LEVEL;
                }
            }
        }
        glClear(GL_COLOR_BUFFER_BIT);
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        elapsed += accumulator;
        if(elapsed < FIXED_TIMESTEP) {
            accumulator = elapsed;
            continue;
        }
        while(elapsed >= FIXED_TIMESTEP) {
            Update(FIXED_TIMESTEP);
            elapsed -= FIXED_TIMESTEP;
        }
        accumulator = elapsed;
        
        Render(program);

        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
