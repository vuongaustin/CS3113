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
#define MAX_BULLETS 3
#include "../SDL2_mixer.framework/Headers/SDL_mixer.h"
#include <random>
#include <math.h>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;
ShaderProgram program;
ShaderProgram program2;

float accumulator = 0.0f;
float projectionWidth;
float projectionHeight = 2.0f;
int bulletIndex = 0, numpencils = 0, bubbleIndex = 0;
GLuint fontTexture, bubbletexture;
enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL_1, STATE_GAME_LEVEL_2, STATE_GAME_LEVEL_3, STATE_WIN, STATE_LOSE };

GameMode mode = STATE_MAIN_MENU;

Mix_Chunk* gamestart;
Mix_Chunk* doodlesound;
Mix_Chunk* deadspongebob;

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
        float randomx = -1.0f + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(1.0f+1.0f)));
        float randomy = -1.0f + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(1.0f+1.0f)));
        direction = glm::vec3(randomx, randomy, 0.0f);
        timer = 2.5f;
    }
    SheetSprite sprite; //7up
    glm::vec3 size;
    glm::vec3 pos;
    glm::vec3 direction;
    float timer;
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
        direction = glm::vec3(0.0f, 0.0f, 0.0f);
        held = false;
    }
    SheetSprite sprite; //7up
    int index;
    bool held;
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

class Bubble {
public:
    SheetSprite sprite;
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

class Particle {
public:
    glm::vec3 position;
    glm::vec3 velocity;
    float lifetime;
};


class ParticleEmitter {
public:
    ParticleEmitter(unsigned int particleCount) {
        for (int i= 0; i < particleCount; i++) {
            float random1 = -0.1f + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(0.1f+0.1f)));
            float random2 = -0.1f + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(0.1f+0.1f)));
            Particle part;
            float random3 = -1.5f + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(1.5f+1.5f)));
            float random4 = -1.5f + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(1.5f+1.5f)));
            part.velocity.x = random3;
            part.velocity.y = random4;
            part.position.x = random1 + position.x;
            part.position.y = random2 + position.y;
            particles.push_back(part);
        }
    }
    void trigger (float elapsed) {
        on = true;
        for (Particle& part : particles) {
            float random1 = -0.1f + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(0.1f+0.1f)));
            float random2 = -0.1f + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(0.1f+0.1f)));
            float random3 = -1.5f + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(1.5f+1.5f)));
            float random4 = -1.5f + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(1.5f+1.5f)));
            part.velocity.x = random3;
            part.velocity.y = random4;
            part.position.x = random1 + position.x;
            part.position.y = random2 + position.y;
            part.lifetime = 0;
        }
    }
    void Update(float elapsed) {
        if (on) {
            for (Particle& part : particles) {
                part.lifetime += elapsed;
                part.velocity.y += -1.0f * elapsed;
                part.position.x += part.velocity.x * elapsed;
                part.position.y += part.velocity.y * elapsed;
            }
        }
    }
    void Render() {
        if (on) {
            std::vector<float> vertices;
            for(int i=0; i < particles.size(); i++) {
                vertices.push_back(particles[i].position.x);
                vertices.push_back(particles[i].position.y);
            }
            glVertexAttribPointer(program2.positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
            glEnableVertexAttribArray(program2.positionAttribute);
            glDrawArrays(GL_POINTS, 0, (int)particles.size());
        }
    }
    bool on;
    glm::vec3 position;
    glm::vec3 gravity;
    float maxLifetime;
    std::vector<Particle> particles;
};

ParticleEmitter emit(200);

class GameState {
public:
    Spongebob bob;
    std::vector<Doodlebob> doods;
    std::vector<Pencil> pencils;
    std::vector<Bubble> bubbles;
};

GameState state;

void shoot () {
    if (numpencils > 0) {
        for (int i = 0; i < state.pencils.size(); i++) {
            if (state.pencils[i].held) {
                state.pencils[i].held = false;
                state.pencils[i].direction.x = 1.0f;
                state.pencils[i].direction.y = 0.0f;
                state.pencils[i].pos.x = state.bob.pos.x + 0.3f;
                state.pencils[i].pos.y = state.bob.pos.y;
                bulletIndex++;
                numpencils--;
                if(bulletIndex > MAX_BULLETS-1) {
                    bulletIndex = 0;
                }
                break;
            }
        }
    }
}

void shootup () {
    if (numpencils > 0) {
        for (int i = 0; i < state.pencils.size(); i++) {
            if (state.pencils[i].held) {
                state.pencils[i].held = false;
                state.pencils[i].direction.x = 1.0f;
                state.pencils[i].direction.y = 1.0f;
                state.pencils[i].pos.x = state.bob.pos.x + 0.3f;
                state.pencils[i].pos.y = state.bob.pos.y;
                bulletIndex++;
                numpencils--;
                if(bulletIndex > MAX_BULLETS-1) {
                    bulletIndex = 0;
                }
                break;
            }
        }
    }
}

void shootdown () {
    if (numpencils > 0) {
        for (int i = 0; i < state.pencils.size(); i++) {
            if (state.pencils[i].held) {
                state.pencils[i].held = false;
                state.pencils[i].direction.x = 1.0f;
                state.pencils[i].direction.y = -1.0f;
                state.pencils[i].pos.x = state.bob.pos.x + 0.3f;
                state.pencils[i].pos.y = state.bob.pos.y;
                bulletIndex++;
                numpencils--;
                if(bulletIndex > MAX_BULLETS-1) {
                    bulletIndex = 0;
                }
                break;
            }
        }
    }
}
void shoottarget (Doodlebob& d) {
    float up = d.pos.y - state.bob.pos.y;
    float over = d.pos.x - state.bob.pos.x;
    if (abs(up) > abs(over)) {
        up /= up;
        over /= up;
    } else {
        up /= over;
        over /= over;
    }
    state.bubbles[bubbleIndex].direction.x = -over * 1.75;
    state.bubbles[bubbleIndex].direction.y = -up * 1.75;
    state.bubbles[bubbleIndex].pos.x = d.pos.x - 0.3f;
    state.bubbles[bubbleIndex].pos.y = d.pos.y;
    bubbleIndex++;
    if (bubbleIndex >= 10) {
        bubbleIndex = 0;
    }
}
void shootbubble (Doodlebob& d) {
    state.bubbles[bubbleIndex].direction.x = -1.0f;
    state.bubbles[bubbleIndex].direction.y = 0.0f;
    state.bubbles[bubbleIndex].pos.x = d.pos.x - 0.3f;
    state.bubbles[bubbleIndex].pos.y = d.pos.y;
    bubbleIndex++;
    if (bubbleIndex >= 10) {
        bubbleIndex = 0;
    }
}
void shootbubbleup (Doodlebob& d) {
    state.bubbles[bubbleIndex].direction.x = -1.0f;
    state.bubbles[bubbleIndex].direction.y = 0.75f;
    state.bubbles[bubbleIndex].pos.x = d.pos.x - 0.3f;
    state.bubbles[bubbleIndex].pos.y = d.pos.y;
    bubbleIndex++;
    if (bubbleIndex >= 10) {
        bubbleIndex = 0;
    }
}
void shootbubbledown (Doodlebob& d) {
    state.bubbles[bubbleIndex].direction.x = -1.0f;
    state.bubbles[bubbleIndex].direction.y = -0.75f;
    state.bubbles[bubbleIndex].pos.x = d.pos.x - 0.3f;
    state.bubbles[bubbleIndex].pos.y = d.pos.y;
    bubbleIndex++;
    if (bubbleIndex >= 10) {
        bubbleIndex = 0;
    }
}

std::vector<bool> middlepencils = {false, false, false};

void resetpencil (Pencil& p) {
    p.pos.y = -projectionHeight / 2 + (projectionHeight / 2 * p.index);
    p.pos.x = -0.1f;
    p.direction.x = 0.0f;
    p.direction.y = 0.0f;
}

void UpdateGameLevel3 (float elapsed) {
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if(keys[SDL_SCANCODE_RIGHT] && state.bob.pos.x + (state.bob.size.x / 2) <= 0.0f) {
        state.bob.pos.x += elapsed * 1.5f;
    }
    if(keys[SDL_SCANCODE_LEFT] && state.bob.pos.x - (state.bob.size.x / 2) >= -projectionWidth) {
        state.bob.pos.x -= elapsed * 1.5f;
    }
    if(keys[SDL_SCANCODE_UP] && state.bob.pos.y + (state.bob.size.y / 2) <= projectionHeight) {
        state.bob.pos.y += elapsed * 1.5f;
    }
    if(keys[SDL_SCANCODE_DOWN] && state.bob.pos.y - (state.bob.size.y / 2) >= -projectionHeight) {
        state.bob.pos.y -= elapsed * 1.5f;
    }
    for (Doodlebob& d : state.doods) {
        d.timer -= elapsed;
        if (d.living) {
            d.pos.y += d.direction.y * elapsed * 0.5f;
            d.pos.x += d.direction.x * elapsed * 0.5f;
            if (d.pos.y + (d.size.y / 2) >= projectionHeight) {  // right
                d.direction.y *= -1;
            }
            if (d.pos.y - (d.size.y / 2) <= -projectionHeight) {  // left
                d.direction.y *= -1;
            }
            if (d.pos.x + (d.size.x / 2) >= projectionWidth) {
                d.direction.x *= -1;
            }
            if (d.pos.x - (d.size.x / 2) <= 0.1f) {
                d.direction.x *= -1;
            }
            if (d.timer < 0.0f) {
                shoottarget(d);
            }
        }
        if (d.timer < 0.0f) {
            d.timer = 2.5f;
        }
    }
    for (Pencil& p : state.pencils) {
        if (p.pos.y + (p.size.y / 2) >= projectionHeight) { // top
            p.direction.y = -p.direction.y;
        }
        
        if (p.pos.y - (p.size.y / 2) <= -projectionHeight) {  // bottom
            p.direction.y = -p.direction.y;
        }
        if (p.pos.x + (p.size.x / 2) >= projectionWidth) {  // right
            resetpencil(p);
        }
        p.pos.x += elapsed * 1.5f * p.direction.x;
        p.pos.y += elapsed * 1.5f * p.direction.y;
        float pwplayer = abs(p.pos.x - state.bob.pos.x) - ((p.size.x + state.bob.size.x) / 2);
        float phplayer = abs(p.pos.y - state.bob.pos.y) - ((p.size.y + state.bob.size.y) / 2);
        if (pwplayer < 0 && phplayer < 0) {
            numpencils++;
            p.pos.x = -200.0f;
            p.held = true;
        }
        int dead = 0;
        for (Doodlebob& d : state.doods) {
            if (!d.living) {dead++;}
            float pw = abs(p.pos.x - d.pos.x) - ((p.size.x + d.size.x) / 2);
            float ph = abs(p.pos.y - d.pos.y) - ((p.size.y + d.size.y) / 2);
            if (pw < 0 && ph < 0) { // hits doodlebob
                Mix_PlayChannel(-1, doodlesound, 0);
                emit.position.x = d.pos.x;
                emit.position.y = d.pos.y;
                emit.trigger(elapsed);
                d.pos.x = -2000.0f;
                d.living = false;
                p.pos.x = -2000.0f;
                resetpencil(p);
            }
        }
        if (dead == state.doods.size()) {
            mode = STATE_WIN;
        }
    }
    for (Bubble& b : state.bubbles) {
        b.pos.x += elapsed * 1.3f * b.direction.x;
        b.pos.y += elapsed * 1.3f * b.direction.y;
        if (b.pos.y + (b.size.y / 2) >= projectionHeight) { // top
            b.direction.y = -b.direction.y;
        }
        
        if (b.pos.y - (b.size.y / 2) <= -projectionHeight) {  // bottom
            b.direction.y = -b.direction.y;
        }
        if (b.pos.x - (b.size.x / 2) <= -projectionWidth) {  // left
            b.pos.x = 2000.0f;
        }
        float pw = abs(state.bob.pos.x - b.pos.x) - ((state.bob.size.x + b.size.x) / 2);
        float ph = abs(state.bob.pos.y - b.pos.y) - ((state.bob.size.y + b.size.y) / 2);
        if (pw < 0 && ph < 0) { // hits player
            Mix_PlayChannel(-1, deadspongebob, 0);
            mode = STATE_LOSE;
        }
        for (Pencil& p : state.pencils) {
            float pw2 = abs(p.pos.x - b.pos.x) - ((p.size.x + b.size.x) / 2);
            float ph2 = abs(p.pos.y - b.pos.y) - ((p.size.y + b.size.y) / 2);
            if (pw2 < 0 && ph2 < 0) { // hits pencil
                resetpencil(p);
                b.pos.x = 1200.0f;
            }
        }
    }
    emit.Update(elapsed);
}

void UpdateGameLevel2 (float elapsed) {
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if(keys[SDL_SCANCODE_RIGHT] && state.bob.pos.x + (state.bob.size.x / 2) <= 0.0f) {
        state.bob.pos.x += elapsed * 1.5f;
    }
    if(keys[SDL_SCANCODE_LEFT] && state.bob.pos.x - (state.bob.size.x / 2) >= -projectionWidth) {
        state.bob.pos.x -= elapsed * 1.5f;
    }
    if(keys[SDL_SCANCODE_UP] && state.bob.pos.y + (state.bob.size.y / 2) <= projectionHeight) {
        state.bob.pos.y += elapsed * 1.5f;
    }
    if(keys[SDL_SCANCODE_DOWN] && state.bob.pos.y - (state.bob.size.y / 2) >= -projectionHeight) {
        state.bob.pos.y -= elapsed * 1.5f;
    }
    for (Doodlebob& d : state.doods) {
        d.timer -= elapsed;
        if (d.living) {
            d.pos.y += d.direction.y * elapsed * 0.5f;
            d.pos.x += d.direction.x * elapsed * 0.5f;
            if (d.pos.y + (d.size.y / 2) >= projectionHeight) {  // right
                d.direction.y *= -1;
            }
            if (d.pos.y - (d.size.y / 2) <= -projectionHeight) {  // left
                d.direction.y *= -1;
            }
            if (d.pos.x + (d.size.x / 2) >= projectionWidth) {
                d.direction.x *= -1;
            }
            if (d.pos.x - (d.size.x / 2) <= 0.1f) {
                d.direction.x *= -1;
            }
            if (d.timer < 0.0f) {
                if (d.pos.y > state.bob.pos.y) {
                    shootbubbledown(d);
                } else if (d.pos.y < state.bob.pos.y) {
                    shootbubbleup(d);
                } else {
                    shootbubble(d);
                }
            }
        }
        if (d.timer < 0.0f) {
            d.timer = 2.5f;
        }
    }
    for (Pencil& p : state.pencils) {
        if (p.pos.y + (p.size.y / 2) >= projectionHeight) { // top
            p.direction.y = -p.direction.y;
        }
        
        if (p.pos.y - (p.size.y / 2) <= -projectionHeight) {  // bottom
            p.direction.y = -p.direction.y;
        }
        if (p.pos.x + (p.size.x / 2) >= projectionWidth) {  // right
            resetpencil(p);
        }
        p.pos.x += elapsed * 1.5f * p.direction.x;
        p.pos.y += elapsed * 1.5f * p.direction.y;
        float pwplayer = abs(p.pos.x - state.bob.pos.x) - ((p.size.x + state.bob.size.x) / 2);
        float phplayer = abs(p.pos.y - state.bob.pos.y) - ((p.size.y + state.bob.size.y) / 2);
        if (pwplayer < 0 && phplayer < 0) {
            numpencils++;
            p.pos.x = -200.0f;
            p.held = true;
        }
        int dead = 0;
        for (Doodlebob& d : state.doods) {
            if (!d.living) {dead++;}
            float pw = abs(p.pos.x - d.pos.x) - ((p.size.x + d.size.x) / 2);
            float ph = abs(p.pos.y - d.pos.y) - ((p.size.y + d.size.y) / 2);
            if (pw < 0 && ph < 0) { // hits doodlebob
                Mix_PlayChannel(-1, doodlesound, 0);
                emit.position.x = d.pos.x;
                emit.position.y = d.pos.y;
                emit.trigger(elapsed);
                d.pos.x = -2000.0f;
                d.living = false;
                p.pos.x = -2000.0f;
                resetpencil(p);
            }
        }
        if (dead == state.doods.size()) {
            mode = STATE_WIN;
        }
    }
    for (Bubble& b : state.bubbles) {
        b.pos.x += elapsed * 1.0f * b.direction.x;
        b.pos.y += elapsed * 1.0f * b.direction.y;
        if (b.pos.y + (b.size.y / 2) >= projectionHeight) { // top
            b.direction.y = -b.direction.y;
        }
        
        if (b.pos.y - (b.size.y / 2) <= -projectionHeight) {  // bottom
            b.direction.y = -b.direction.y;
        }
        if (b.pos.x - (b.size.x / 2) <= -projectionWidth) {  // left
            b.pos.x = 2000.0f;
        }
        float pw = abs(state.bob.pos.x - b.pos.x) - ((state.bob.size.x + b.size.x) / 2);
        float ph = abs(state.bob.pos.y - b.pos.y) - ((state.bob.size.y + b.size.y) / 2);
        if (pw < 0 && ph < 0) { // hits player
            Mix_PlayChannel(-1, deadspongebob, 0);
            mode = STATE_LOSE;
        }
        for (Pencil& p : state.pencils) {
            float pw2 = abs(p.pos.x - b.pos.x) - ((p.size.x + b.size.x) / 2);
            float ph2 = abs(p.pos.y - b.pos.y) - ((p.size.y + b.size.y) / 2);
            if (pw2 < 0 && ph2 < 0) { // hits pencil
                resetpencil(p);
                b.pos.x = 2000.0f;
            }
        }
    }
    emit.Update(elapsed);
}

void UpdateGameLevel1 (float elapsed) {
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if(keys[SDL_SCANCODE_RIGHT] && state.bob.pos.x + (state.bob.size.x / 2) <= 0.0f) {
        state.bob.pos.x += elapsed * 1.5f;
    }
    if(keys[SDL_SCANCODE_LEFT] && state.bob.pos.x - (state.bob.size.x / 2) >= -projectionWidth) {
        state.bob.pos.x -= elapsed * 1.5f;
    }
    if(keys[SDL_SCANCODE_UP] && state.bob.pos.y + (state.bob.size.y / 2) <= projectionHeight) {
        state.bob.pos.y += elapsed * 1.5f;
    }
    if(keys[SDL_SCANCODE_DOWN] && state.bob.pos.y - (state.bob.size.y / 2) >= -projectionHeight) {
        state.bob.pos.y -= elapsed * 1.5f;
    }
    for (Doodlebob& d : state.doods) {
        d.timer -= elapsed;
        if (d.living) {
            d.pos.y += d.direction.y * elapsed * 0.5f;
            if (d.pos.y + (d.size.y / 2) >= projectionHeight) {  // right
                d.direction.y *= -1;
            }
            if (d.pos.y - (d.size.y / 2) <= -projectionHeight) {  // left
                d.direction.y *= -1;
            }
            if (d.pos.x + (d.size.x / 2) >= projectionWidth) {
                d.direction.x *= -1;
            }
            if (d.pos.x - (d.size.x / 2) <= 0.1f) {
                d.direction.x *= -1;
            }
            if (d.timer < 0.0f) {
                if (d.pos.y > state.bob.pos.y) {
                    shootbubbledown(d);
                } else if (d.pos.y < state.bob.pos.y) {
                    shootbubbleup(d);
                } else {
                    shootbubble(d);
                }
            }
        }
        if (d.timer < 0.0f) {
            d.timer = 2.5f;
        }
    }
    for (Pencil& p : state.pencils) {
        if (p.pos.y + (p.size.y / 2) >= projectionHeight) { // top
            p.direction.y = -p.direction.y;
        }
        
        if (p.pos.y - (p.size.y / 2) <= -projectionHeight) {  // bottom
            p.direction.y = -p.direction.y;
        }
        if (p.pos.x + (p.size.x / 2) >= projectionWidth) {  // right
            resetpencil(p);
        }
        p.pos.x += elapsed * 1.5f * p.direction.x;
        p.pos.y += elapsed * 1.5f * p.direction.y;
        float pwplayer = abs(p.pos.x - state.bob.pos.x) - ((p.size.x + state.bob.size.x) / 2);
        float phplayer = abs(p.pos.y - state.bob.pos.y) - ((p.size.y + state.bob.size.y) / 2);
        if (pwplayer < 0 && phplayer < 0) {
            numpencils++;
            p.pos.x = -200.0f;
            p.held = true;
        }
        int dead = 0;
        for (Doodlebob& d : state.doods) {
            if (!d.living) {dead++;}
            float pw = abs(p.pos.x - d.pos.x) - ((p.size.x + d.size.x) / 2);
            float ph = abs(p.pos.y - d.pos.y) - ((p.size.y + d.size.y) / 2);
            if (pw < 0 && ph < 0) { // hits doodlebob
                Mix_PlayChannel(-1, doodlesound, 0);
                emit.position.x = d.pos.x;
                emit.position.y = d.pos.y;
                emit.trigger(elapsed);
                d.pos.x = -2000.0f;
                d.living = false;
                p.pos.x = -2000.0f;
                resetpencil(p);
            }
        }
        if (dead == state.doods.size()) {
            mode = STATE_WIN;
        }
    }
    for (Bubble& b : state.bubbles) {
        b.pos.x += elapsed * 1.0f * b.direction.x;
        b.pos.y += elapsed * 1.0f * b.direction.y;
        if (b.pos.y + (b.size.y / 2) >= projectionHeight) { // top
            b.direction.y = -b.direction.y;
        }
        
        if (b.pos.y - (b.size.y / 2) <= -projectionHeight) {  // bottom
            b.direction.y = -b.direction.y;
        }
        if (b.pos.x - (b.size.x / 2) <= -projectionWidth) {  // left
            b.pos.x = 2000.0f;
        }
        float pw = abs(state.bob.pos.x - b.pos.x) - ((state.bob.size.x + b.size.x) / 2);
        float ph = abs(state.bob.pos.y - b.pos.y) - ((state.bob.size.y + b.size.y) / 2);
        if (pw < 0 && ph < 0) { // hits player
            Mix_PlayChannel(-1, deadspongebob, 0);
            mode = STATE_LOSE;
        }
        for (Pencil& p : state.pencils) {
            float pw2 = abs(p.pos.x - b.pos.x) - ((p.size.x + b.size.x) / 2);
            float ph2 = abs(p.pos.y - b.pos.y) - ((p.size.y + b.size.y) / 2);
            if (pw2 < 0 && ph2 < 0) { // hits pencil
                resetpencil(p);
                b.pos.x = 2000.0f;
            }
        }
    }
    emit.Update(elapsed);
}

void UpdateMainMenu (float elapsed) {
    
}

void Update(float elapsed) {
    switch(mode) {
        case STATE_MAIN_MENU:
            break;
        case STATE_GAME_LEVEL_1:
            UpdateGameLevel1(elapsed);
            break;
        case STATE_GAME_LEVEL_2:
            UpdateGameLevel2(elapsed);
            break;
        case STATE_GAME_LEVEL_3:
            UpdateGameLevel3(elapsed);
            break;
        case STATE_WIN:
            break;
        case STATE_LOSE:
            break;
    }
}

void RenderGameLevel1 (ShaderProgram& program) {
    state.bob.Draw(program);
//    for (Doodlebob& d : state.doods) {
//        d.Draw(program);
//    }
    state.doods[0].living = false;
    state.doods[0].pos.x = -2000.0f;
    state.doods[1].Draw(program);
    state.doods[2].living = false;
    state.doods[2].pos.x = -2000.0f;
    for (Pencil& p : state.pencils) {
        p.Draw(program);
    }
    for (Bubble& b : state.bubbles) {
        b.Draw(program);
    }
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    program2.SetModelMatrix(modelMatrix);
    emit.Render();
}

void RenderGameLevel2 (ShaderProgram& program) {
    state.bob.Draw(program);
//    for (Doodlebob& d : state.doods) {
//        d.Draw(program);
//    }
    state.doods[0].Draw(program);
    state.doods[1].living = false;
    state.doods[1].pos.x = -2000.0f;
    state.doods[2].Draw(program);
    for (Pencil& p : state.pencils) {
        p.Draw(program);
    }
    for (Bubble& b : state.bubbles) {
        b.Draw(program);
    }
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    program2.SetModelMatrix(modelMatrix);
    emit.Render();
}

void RenderGameLevel3 (ShaderProgram& program) {
    state.bob.Draw(program);
    for (Doodlebob& d : state.doods) {
        d.Draw(program);
    }
    for (Pencil& p : state.pencils) {
        p.Draw(program);
    }
    for (Bubble& b : state.bubbles) {
        b.Draw(program);
    }
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    program2.SetModelMatrix(modelMatrix);
    emit.Render();
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
    modelMatrix = glm::translate(modelMatrix, glm::vec3(-2.7f, 0.3f, 0.0f));
    program.SetModelMatrix(modelMatrix);
    glUseProgram(program.programID);
    DrawText(program, fontTexture, "Bikini Bottom Dodgeball", 0.25f, 0.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(1.5f, -0.3f, 0.0f));
    program.SetModelMatrix(modelMatrix);
    glUseProgram(program.programID);
    DrawText(program, fontTexture, "Pick a Level", 0.2f, 0.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -0.2f, 0.0f));
    program.SetModelMatrix(modelMatrix);
    glUseProgram(program.programID);
    DrawText(program, fontTexture, "1) Beginner", 0.2f, 0.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -0.2f, 0.0f));
    program.SetModelMatrix(modelMatrix);
    glUseProgram(program.programID);
    DrawText(program, fontTexture, "2) Intermediate", 0.2f, 0.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -0.2f, 0.0f));
    program.SetModelMatrix(modelMatrix);
    glUseProgram(program.programID);
    DrawText(program, fontTexture, "3) Hard", 0.2f, 0.0f);
}

void RenderWin (ShaderProgram& program) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.7f, 0.3f, 0.0f));
    program.SetModelMatrix(modelMatrix);
    glUseProgram(program.programID);
    DrawText(program, fontTexture, "Congratulations", 0.25f, 0.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.3f, -0.4f, 0.0f));
    program.SetModelMatrix(modelMatrix);
    glUseProgram(program.programID);
    DrawText(program, fontTexture, "Press Enter to Close", 0.2f, 0.0f);
}

void RenderLose (ShaderProgram& program) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.0f, 0.3f, 0.0f));
    program.SetModelMatrix(modelMatrix);
    glUseProgram(program.programID);
    DrawText(program, fontTexture, "Game Over", 0.25f, 0.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.0f, -0.4f, 0.0f));
    program.SetModelMatrix(modelMatrix);
    glUseProgram(program.programID);
    DrawText(program, fontTexture, "Press Enter to Close", 0.2f, 0.0f);
}

void Render (ShaderProgram& program) {
    switch(mode) {
        case STATE_MAIN_MENU:
            RenderMainMenu(program);
            break;
        case STATE_GAME_LEVEL_1:
            RenderGameLevel1(program);
            break;
        case STATE_GAME_LEVEL_2:
            RenderGameLevel2(program);
            break;
        case STATE_GAME_LEVEL_3:
            RenderGameLevel3(program);
            break;
        case STATE_WIN:
            RenderWin(program);
            break;
        case STATE_LOSE:
            RenderLose(program);
            break;
    }
}

int main(int argc, char *argv[]) {
    Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 4096 );
    Mix_Music* backgroundmusic = Mix_LoadMUS("music.mp3");
    gamestart = Mix_LoadWAV("gamestart.wav");
    doodlesound = Mix_LoadWAV("doodlebob.wav");
    deadspongebob = Mix_LoadWAV("deadspongebob.wav");
    Mix_PlayMusic(backgroundmusic, -1);
    float width = 940;
    float height = 500;
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Bikini Bottom Dodgeball", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
    glViewport(0, 0, width, height);
    program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    program2.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");

    SDL_Event event;
    bool done = false;
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glClearColor(0.2745f, 0.5098f, 0.7058f, 1.0f); //background color set
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    float aspectRatio = (float)width / (float)height; //dimensions of window
    projectionWidth = projectionHeight * aspectRatio;
    float projectionDepth = 1.0f;
    projectionMatrix = glm::ortho(-projectionWidth, projectionWidth, -projectionHeight, projectionHeight, -projectionDepth, projectionDepth);
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
    program2.SetProjectionMatrix(projectionMatrix);
    program2.SetViewMatrix(viewMatrix);
    fontTexture = LoadTexture(RESOURCE_FOLDER"font1.png");


    GLuint spriteSheetTexture = LoadTexture(RESOURCE_FOLDER"spongebobsprite.png");
    SheetSprite spongebobsprite = SheetSprite(spriteSheetTexture, 0.0f/500.0f, 0.0f/500.0f, 74.0f/500.0f, 100.0f/
                           500.0f, 0.4f);
    SheetSprite doodlebobsprite = SheetSprite(spriteSheetTexture, 0.0f/500.0f, 100.0f/500.0f, 138.0f/500.0f, 100.0f/
                                              500.0f, 0.3f);
    SheetSprite pencilsprite = SheetSprite(spriteSheetTexture, 0.0f/500.0f, 200.0f/500.0f, 22.0f/500.0f, 22.0f/
                                              500.0f, 0.2f);
    GLuint bubbletexture = LoadTexture(RESOURCE_FOLDER"bubble.png");
    SheetSprite bubblesprite = SheetSprite(bubbletexture, 0.0f/640.0f, 0.0f/627.0f, 640.0f/640.0f, 627.0f/627.0f, 0.2f);
    emit.position.x = 0.0f;
    emit.position.y = 0.0f;
    
    glPointSize(4);
    
    Spongebob bob;
    bob.sprite = spongebobsprite;
    bob.pos.x = -2.0f;
    bob.pos.y = 0.0f;
    bob.size.x = 2.0f * spongebobsprite.size * (spongebobsprite.width / spongebobsprite.height) * 0.5f;
    bob.size.y = 2.0f * spongebobsprite.size * 0.5f;
    state.bob = bob;
    
    std::vector<Doodlebob> doods;
    for (int i = 0; i < 3; i++) {
        Doodlebob dood;
        dood.sprite = doodlebobsprite;
        dood.size.x = 2.0f * doodlebobsprite.size * (doodlebobsprite.width / doodlebobsprite.height) * 0.5f;
        dood.size.y = 2.0f * doodlebobsprite.size * 0.5f;
        if (i == 0) {
            dood.pos.y = -projectionHeight / 2;
        } else if (i == 1) {
            dood.pos.y = 0.0f;
        } else if (i == 2) {
            dood.pos.y = projectionHeight / 2;
        }
        dood.pos.x = 2.5f;
        dood.living = true;
        doods.push_back(dood);
    }
    state.doods = doods;

    std::vector<Pencil> bullets;
    for (int i=0; i < MAX_BULLETS; i++) {
        Pencil p;
        p.index = i;
        p.sprite = pencilsprite;
        p.pos.y = -projectionHeight / 2 + (projectionHeight / 2 * i);
        p.pos.x = -0.1f;
        p.size.x = 2.0f * pencilsprite.size * (pencilsprite.width / pencilsprite.height) * 0.5f;
        p.size.y = 2.0f * pencilsprite.size * 0.5f;
        bullets.push_back(p);
    }
    state.pencils = bullets;
    
    std::vector<Bubble> bubs;
    for (int i = 0; i < 10; i++) {
        Bubble b;
        b.sprite = bubblesprite;
        b.pos.x = 2000.0f;
        b.pos.y = 0.0f;
        b.size.x = 2.0f * bubblesprite.size * (bubblesprite.width / bubblesprite.height) * 0.5f;
        b.size.y = 2.0f * bubblesprite.size * 0.5f;
        bubs.push_back(b);
    }
    state.bubbles = bubs;
    
    float lastFrameTicks = 0.0f;
    
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            } else if (event.type == SDL_KEYDOWN) {
                if (mode == STATE_MAIN_MENU) {
                    if (event.key.keysym.scancode == SDL_SCANCODE_T) {
                        Mix_PlayChannel(-1, gamestart, 0);
                    }
                    if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                        done = true;
                    }
                    if (event.key.keysym.scancode == SDL_SCANCODE_1) {
                        Mix_PlayChannel(-1, gamestart, 0);
                        mode = STATE_GAME_LEVEL_1;
                    }
                    if (event.key.keysym.scancode == SDL_SCANCODE_2) {
                        Mix_PlayChannel(-1, gamestart, 0);
                        mode = STATE_GAME_LEVEL_2;
                    }
                    if (event.key.keysym.scancode == SDL_SCANCODE_3) {
                        Mix_PlayChannel(-1, gamestart, 0);
                        mode = STATE_GAME_LEVEL_3;
                    }
                } else if (mode == STATE_WIN || mode == STATE_LOSE) {
                    if (event.key.keysym.scancode == SDL_SCANCODE_RETURN) {
                        done = true;
                    }
                } else {
                    if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                        mode = STATE_MAIN_MENU;
                    }
                    if (event.key.keysym.scancode == SDL_SCANCODE_W) {
                        shoot();
                    }
                    if (event.key.keysym.scancode == SDL_SCANCODE_Q) {
                        shootup();
                    }
                    if (event.key.keysym.scancode == SDL_SCANCODE_E) {
                        shootdown();
                    }
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
    Mix_FreeMusic(backgroundmusic);
    Mix_FreeChunk(gamestart);
    Mix_FreeChunk(doodlesound);
    Mix_FreeChunk(deadspongebob);
    Mix_CloseAudio();
    SDL_Quit();
    return 0;
}
