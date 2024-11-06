/**
* Author: Kaitlyn Huynh
* Assignment: Rise of the AI
* Date due: 2024-11-9, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 3
#define ENEMY_COUNT 3

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"
#include "Map.h"

// ––––– STRUCTS AND ENUMS ––––– //
struct GameState
{
    Entity *player;
//    Entity *platforms;
    Entity *enemies;
    
    Map* map;
    
    Mix_Music *bgm;
    Mix_Chunk *jump_sfx;
};
// Map details
constexpr int MAP_WIDTH = 14,
        MAP_HEIGHT = 5;
unsigned int LEVEL_DATA[] =
{
    23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23,
    23, 0, 0, 0, 0, 0, 0, 0, 0, 23, 23, 23, 0, 23,
    23, 0, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 23,
    23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23
};


enum AppStatus { RUNNING, TERMINATED };

// ––––– CONSTANTS ––––– //
constexpr int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

constexpr float BG_RED     = 61.0f / 255.0f,
            BG_BLUE    = 59.0f / 255.0f,
            BG_GREEN   = 64.0f / 255.0f,
            BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr char SPRITESHEET_FILEPATH[] = "assets/peach.png",
                    MARIO_FILEPATH[] = "assets/mario4.png",
                    ENEMY_FILEPATH[] = "assets/goomba.png",
                    LUIGI_FILEPATH[] = "assets/luigi.png",
                    PLATFORM_FILEPATH[] = "assets/greenPipe.png",
                    THWOMP_FILEPATH[] = "assets/thwomp.png",
                    TILESHEET_FILEPATH[] = "assets/tiles.png",
                    FONT_FILEPATH[] = "assets/font1.png";
        

constexpr char BGM_FILEPATH[] = "assets/crypto.mp3",
           SFX_FILEPATH[] = "assets/bounce.wav";

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL  = 0;
constexpr GLint TEXTURE_BORDER   = 0;

constexpr float PLATFORM_OFFSET = 5.0f;

// ––––– VARIABLES ––––– //
GameState g_game_state;

SDL_Window* g_display_window;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

AppStatus g_app_status = RUNNING;

GLuint load_texture(const char* filepath);

void initialise();
void process_input();
void update();
void render();
void shutdown();


// ––––– GENERAL FUNCTIONS ––––– //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    stbi_image_free(image);
    
    return textureID;
}

void initialise()
{
    // ––––– GENERAL STUFF ––––– //
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("AI PROJ",
                                  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  WINDOW_WIDTH, WINDOW_HEIGHT,
                                  SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (context == nullptr)
    {
        LOG("ERROR: Could not create OpenGL context.\n");
        shutdown();
    }

    #ifdef _WINDOWS
    glewInit();
    #endif
    // ––––– VIDEO STUFF ––––– //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_view_matrix = glm::mat4(1.0f);

    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glm::vec3 acceleration = glm::vec3(0.0f,-4.905f, 0.0f); // Shared acceleration

    glClearColor(0.68f, 0.85f, 0.90f, 1.0f); // Pastel blue background color
    
    //PEACH//
    GLuint player_texture_id = load_texture(SPRITESHEET_FILEPATH);
    //Create player entity

    g_game_state.player = new Entity(player_texture_id, 5.0f, 0.2f, 1.3f, PLAYER); // sprite hitbox (center of pos)
    g_game_state.player->set_sprite_size(glm::vec3(2.0f, 4.0f, 0.0f)); // change size of sprite
    g_game_state.player->set_position(glm::vec3(8.0f, 8.0f, 0.0f));
    g_game_state.player->set_acceleration(acceleration);
    g_game_state.player->set_jumping_power(7.0f);
    
    // Map Set up //
    GLuint map_texture_id = load_texture(TILESHEET_FILEPATH);
    g_game_state.map = new Map(MAP_WIDTH, MAP_HEIGHT, LEVEL_DATA, map_texture_id, 1.0f, 8, 8); // 1.0f, 4, 1

    // ––––– GOOMBA ––––– Render enemies //
    GLuint enemy_texture_id = load_texture(ENEMY_FILEPATH);

    g_game_state.enemies = new Entity[ENEMY_COUNT];
    
    AIType ait;
    for (int i = 0; i < ENEMY_COUNT; i++)
    {
        switch (i) {
            case 0:
                ait = GUARD;
            case 1:
                ait = WALKER;
            case 2:
                ait = JUMPER;
           
        }
        g_game_state.enemies[i] = Entity(enemy_texture_id, 0.5f, 1.0f, 1.0f, ENEMY, (AIType) i, IDLE); // 0 walker 1 guard 2 jumper
        if (i == 2) { // Jumper position
            g_game_state.enemies[i].set_position(glm::vec3(7.0f, 1.0f, 0.0f));
        } else {
            g_game_state.enemies[i].set_position(glm::vec3(i + 2.0f, 1.0f, 0.0f));
        }
        
        g_game_state.enemies[i].set_sprite_size(glm::vec3(1.0f, 1.0f, 0.0f));
        g_game_state.enemies[i].set_acceleration(acceleration);
        g_game_state.enemies[i].set_jumping_power(2.0f);
    }
    // Fonts
    GLuint file_texture_id = load_texture(FONT_FILEPATH);
    // ––––– PLATFORM ––––– //
    GLuint platform_texture_id = load_texture(PLATFORM_FILEPATH);
    // Render platform obstacles
//    g_game_state.platforms = new Entity[PLATFORM_COUNT];
    
//     Starting platform
//    for (int i = 0; i < PLATFORM_COUNT; i++)
//    {
//        g_game_state.platforms[i] = Entity(platform_texture_id, 0.0f, 1.0f, 1.0f, PLATFORM);
//        g_game_state.platforms[i].set_position(glm::vec3(i + 4.0f, 0.7f, 0.0f));
//        g_game_state.platforms[i].set_sprite_size(glm::vec3(1.0f, 1.0f, 0.0f));
//        g_game_state.platforms[i].update(0.0f,
//                                    g_game_state.player,
//                                    g_game_state.platforms,
//                                    PLATFORM_COUNT,
//                                    g_game_state.map
//                                    );
//    }
    
    // ––––– AUDIO STUFF ––––– //
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

    g_game_state.bgm = Mix_LoadMUS(BGM_FILEPATH);
    Mix_PlayMusic(g_game_state.bgm, -1);
    Mix_VolumeMusic(MIX_MAX_VOLUME / 4.0f);

    g_game_state.jump_sfx = Mix_LoadWAV(SFX_FILEPATH);

    // ––––– GENERAL STUFF ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_game_state.player->set_movement(glm::vec3(0.0f));
 
 SDL_Event event;
 while (SDL_PollEvent(&event))
 {
     switch (event.type) {
         // End game
         case SDL_QUIT:
         case SDL_WINDOWEVENT_CLOSE:
             g_app_status = TERMINATED;
             break;
             
         case SDL_KEYDOWN:
             switch (event.key.keysym.sym) {
                 case SDLK_q:
                     // Quit the game with a keystroke
                     g_app_status = TERMINATED;
                     break;
                     
                 case SDLK_SPACE:
                     // Jump
                     if (g_game_state.player->get_collided_bottom())
                     {
                         g_game_state.player->jump();
                         Mix_PlayChannel(-1, g_game_state.jump_sfx, 0);
                     }
                     break;
                     
                 default:
                     break;
             }
             
         default:
             break;
     }
 }
 
 const Uint8 *key_state = SDL_GetKeyboardState(NULL);

 if (key_state[SDL_SCANCODE_LEFT])       g_game_state.player->move_left();
 else if (key_state[SDL_SCANCODE_RIGHT]) g_game_state.player->move_right();
     
 if (glm::length(g_game_state.player->get_movement()) > 1.0f)
    g_game_state.player->normalise_movement();
 
 
}
bool lose_game = false;
void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }
    
    while (delta_time >= FIXED_TIMESTEP)
    {
        
//        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.player, g_game_state.platforms, PLATFORM_COUNT, g_game_state.map);
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.player, g_game_state.enemies, ENEMY_COUNT, g_game_state.map);
        
        for (int i = 0; i < ENEMY_COUNT; i++) {

                g_game_state.enemies[i].update(FIXED_TIMESTEP,
                                               g_game_state.player,
                                               g_game_state.player,
                                               1,
                                               g_game_state.map
                                               );
                
            if (g_game_state.enemies[i].get_ai_type() == JUMPER) {
                g_game_state.enemies[i].ai_jump();
            }
            
            g_game_state.player->check_collision_x(g_game_state.enemies, ENEMY_COUNT);
            g_game_state.player->check_collision_y(g_game_state.enemies, ENEMY_COUNT);
            if ((g_game_state.player->get_collided_left() || g_game_state.player->get_collided_right() && g_game_state.player->get_collided_with() == &g_game_state.enemies[i])) {
                LOG("Hit 2");
                lose_game = true;
            }
            else if (g_game_state.player->get_collided_bottom() && g_game_state.player->get_collided_with() == &g_game_state.enemies[i]) {
                LOG("Hit1");
                g_game_state.enemies[i].deactivate();
            }
        }
        

        
        delta_time -= FIXED_TIMESTEP;
    }

    g_accumulator = delta_time;
    
    // Camera follows player
    g_view_matrix = glm::mat4(1.0f);
    g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-g_game_state.player->get_position().x, 0.0f, 0.0f));
}
constexpr int FONTBANK_SIZE = 16;
void draw_text(ShaderProgram* shader_program, GLuint font_texture_id, std::string text, float font_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;
    
    // Instead of having a single pair of arrays, we'll have a series of pairs—one for
    // each character. Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;
    
    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their
        //    position relative to the whole sentence)
        int spritesheet_index = (int)text[i];  // ascii value of character
        float offset = (font_size + spacing) * i;
        
        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float)(spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float)(spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;
        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
        });
        
        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
        });
    }
    
    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);
    
    shader_program->set_model_matrix(model_matrix);
    glUseProgram(shader_program->get_program_id());
    glVertexAttribPointer(shader_program->get_position_attribute(), 2, GL_FLOAT, false, 0,
                          vertices.data());
    glEnableVertexAttribArray(shader_program->get_position_attribute());
    
    glVertexAttribPointer(shader_program->get_tex_coordinate_attribute(), 2, GL_FLOAT,
                          false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(shader_program->get_tex_coordinate_attribute());
    
    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));
    
    glDisableVertexAttribArray(shader_program->get_position_attribute());
    glDisableVertexAttribArray(shader_program->get_tex_coordinate_attribute());
}

void render()
{
    GLuint file_texture_id = load_texture(FONT_FILEPATH);
    glClear(GL_COLOR_BUFFER_BIT);

    g_game_state.player->render(&g_shader_program);
    g_game_state.map->render(&g_shader_program);
    // Camera follows player
    g_shader_program.set_view_matrix(g_view_matrix);
    int inactive_count = 0;
    for (int i = 0; i < ENEMY_COUNT; i++) {
        if (g_game_state.enemies[i].get_is_active()) {
            g_game_state.enemies[i].render(&g_shader_program);
        } else {
            inactive_count++;
            g_game_state.enemies[i].set_position(glm::vec3(-10.0f,-10.0f,0.0f));
        }
    }
    if (lose_game == true) {
        draw_text(&g_shader_program, file_texture_id, "You lose!", 1.0f, 0.0001f, glm::vec3(1.0f, 1.0f, 0.0f));
    }
    else if (inactive_count == ENEMY_COUNT) {
        draw_text(&g_shader_program, file_texture_id, "You win!", 1.0f, 0.0001f, glm::vec3(1.0f, 1.0f, 0.0f));
    }

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();

//    delete [] g_game_state.platforms;
    delete [] g_game_state.enemies;
    delete    g_game_state.player;
    Mix_FreeChunk(g_game_state.jump_sfx);
    Mix_FreeMusic(g_game_state.bgm);
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
