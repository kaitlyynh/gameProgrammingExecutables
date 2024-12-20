/**
* Author: Kaitlyn Huynh
* Assignment: Animal Collector
* Date due: Dec-11-2024, 2:00pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define LEVEL1_WIDTH 14
#define LEVEL1_HEIGHT 5
#define LEVEL1_LEFT_EDGE 5.0f
#define LOG(argument) std::cout << argument << '\n'
#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"
#include "Map.h"
#include "Utility.h"
#include "Scene.h"
#include "Menu.h"
#include "LevelA.h"
#include "LevelB.h"
#include "LevelC.h"

// ————— CONSTANTS ————— //
constexpr int WINDOW_WIDTH  = 640 * 2,
            WINDOW_HEIGHT = 600; // 480
    

constexpr float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

// No spotlight
constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
                F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

// Spotlight
constexpr char V_SHADER_PATH_2[] = "shaders/vertex_exercise.glsl",
                F_SHADER_PATH_2[] = "shaders/fragment_exercise.glsl";

constexpr char FONT_FILEPATH[] = "assets/font1.png",
                HEART_FILEPATH[] = "assets/heart.png";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;


enum AppStatus { RUNNING, TERMINATED };

// ————— GLOBAL VARIABLES ————— //
Scene* scenes[4];
Scene *g_current_scene;
Menu* g_menu;
LevelA *g_level_a;
LevelB *g_level_b;
LevelC *g_level_c;

SDL_Window* g_display_window;

AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program;
ShaderProgram g_gray_aura_program;
glm::mat4 g_view_matrix, g_projection_matrix;
float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

void switch_to_scene(Scene *scene)
{
    g_current_scene = scene;
    g_current_scene->initialise();
}

void initialise();
void process_input();
void update();
void render();
void shutdown();


void initialise()
{
    // ————— VIDEO ————— //
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("Proj",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    if (context == nullptr)
    {
        shutdown();
    }
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    // ————— GENERAL ————— //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    
//    // Other shader program
    g_gray_aura_program.load(V_SHADER_PATH_2, F_SHADER_PATH_2);
    g_gray_aura_program.set_projection_matrix(g_projection_matrix);
    g_gray_aura_program.set_view_matrix(g_view_matrix);
//    // Other shader prgoram

    glUseProgram(g_shader_program.get_program_id());
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    // Menu setup //
    g_menu = new Menu();
    
    // ————— LEVELS SETUP ————— //
    g_level_a = new LevelA();
    g_level_b = new LevelB();
    g_level_c = new LevelC();
    
    scenes[0] = g_menu;
    scenes[1] = g_level_a;
    scenes[2] = g_level_b;
    scenes[3] = g_level_c;
    
    switch_to_scene(scenes[0]);
    
   
    // ————— BLENDING ————— //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_current_scene->get_state().player->set_movement(glm::vec3(0.0f));
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        // ————— KEYSTROKES ————— //
        switch (event.type) {
            // ————— END GAME ————— //
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
                        // ————— JUMPING ————— //
                        break; // No jumping supported
                        if (g_current_scene->get_state().player->get_collided_bottom())
                        {
                            g_current_scene->get_state().player->jump();
                            
                        }
                         
                    case SDLK_RETURN:
                        //                            g_current_scene = g_level_b;
                        //                            g_current_scene = g_level_c;
                        //                            g_current_scene.get_state().next_scene_id = 1;
                        // Menu scene & enter was pressed,
                        if (g_current_scene->get_state().map == nullptr) {
                            g_current_scene = g_level_a;
//                            g_current_scene = g_level_b;
//                            g_current_scene = g_level_c;
                            switch_to_scene(g_current_scene);
                        }
                        break;
                    case SDLK_w:
                        if (g_current_scene == g_level_a || g_current_scene == g_level_b) {
                            g_current_scene->get_state().player->shoot_bullet(g_current_scene, glm::vec3(0.0f, 1.0f, 0.0f)); // Only shoot up
                            if (g_current_scene->get_state().player->get_bullet_curr() > 0) {
                                Mix_Chunk *shoot_sfx = Mix_LoadWAV("assets/attack_hit.mp3");
                                Mix_PlayChannel(-1,  shoot_sfx, 0);
                            }
                            break;
                        }
                        
                        
                    case SDLK_a:
                        if (g_current_scene == g_level_a || g_current_scene == g_level_b) {
                            g_current_scene->get_state().player->shoot_bullet(g_current_scene, glm::vec3(-1.0f, 0.0f, 0.0f)); // Only shoot left
                            if (g_current_scene->get_state().player->get_bullet_curr() > 0) {
                                Mix_Chunk *shoot_sfx = Mix_LoadWAV("assets/attack_hit.mp3");
                                Mix_PlayChannel(-1,  shoot_sfx, 0);
                            }
                            break;
                        }
                    case SDLK_d:
                        if (g_current_scene == g_level_a || g_current_scene == g_level_b) {
                            g_current_scene->get_state().player->shoot_bullet(g_current_scene, glm::vec3(1.0f, 0.0f, 0.0f)); // Only shoot right
                            if (g_current_scene->get_state().player->get_bullet_curr() > 0) {
                                Mix_Chunk *shoot_sfx = Mix_LoadWAV("assets/attack_hit.mp3");
                                Mix_PlayChannel(-1,  shoot_sfx, 0);
                            }
                            break;
                        }
                    case SDLK_s:
                        if (g_current_scene == g_level_a || g_current_scene == g_level_b) {
                            g_current_scene->get_state().player->shoot_bullet(g_current_scene, glm::vec3(0.0f, -1.0f, 0.0f)); // Only shoot down
                            if (g_current_scene->get_state().player->get_bullet_curr() > 0) {
                                Mix_Chunk *shoot_sfx = Mix_LoadWAV("assets/attack_hit.mp3");
                                Mix_PlayChannel(-1,  shoot_sfx, 0);
                            }
                            break;
                        }
                    case SDLK_1:
                        if (g_current_scene == g_level_c) {
                            g_level_c->spawn_animal("WEINER_DOG_FILEPATH");
                            Mix_Chunk *place_sfx = Mix_LoadWAV("assets/p_1.ogg");
                            Mix_VolumeChunk(place_sfx, MIX_MAX_VOLUME);  // MAX  128
                            Mix_PlayChannel(-1,  place_sfx, 0);
                        }
                        
                        break;
                    case SDLK_2:
                        if (g_current_scene == g_level_c) {
                            g_level_c->spawn_animal("MONKEY_BANANA_FILEPATH");
                            Mix_Chunk *place_sfx = Mix_LoadWAV("assets/p_1.ogg");
                            Mix_VolumeChunk(place_sfx, MIX_MAX_VOLUME);
                            Mix_PlayChannel(-1,  place_sfx, 0);
                        }
                        break;
                    case SDLK_z:
                        g_current_scene->complete = true; // Use to quick test
                        break;
                    default:
                        break;
                }
                
            default:
                break;
        }
    }
    
    // ————— KEY HOLD ————— //
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT])        { g_current_scene->get_state().player->move_left(); }
    else if (key_state[SDL_SCANCODE_RIGHT])  { g_current_scene->get_state().player->move_right(); }
    else if (key_state[SDL_SCANCODE_UP])  { g_current_scene->get_state().player->move_up(); }
    else if (key_state[SDL_SCANCODE_DOWN])  { g_current_scene->get_state().player->move_down(); }
    if (glm::length( g_current_scene->get_state().player->get_movement()) > 1.0f)
        g_current_scene->get_state().player->normalise_movement();
 
}

void update()
{
    // ————— DELTA TIME / FIXED TIME STEP CALCULATION ————— //
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    delta_time += g_accumulator;
    
    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }
    
    while (delta_time >= FIXED_TIMESTEP) {
        // ————— UPDATING THE SCENE (i.e. map, character, enemies...) ————— //
        g_current_scene->update(FIXED_TIMESTEP);
        
        delta_time -= FIXED_TIMESTEP;
    }
    
    g_accumulator = delta_time;
    
    
    // ————— PLAYER CAMERA ————— //
    g_view_matrix = glm::mat4(1.0f);
    g_view_matrix = glm::translate(g_view_matrix, glm::vec3(
                                                -g_current_scene->get_state().player->get_position().x,
                                                -g_current_scene->get_state().player->get_position().y,
                                                0
                                                            ));
    
    g_gray_aura_program.set_light_position_matrix(g_current_scene->get_state().player->get_position());
    
}

void render()
{
    GLuint file_texture_id = Utility::load_texture(FONT_FILEPATH);
//    g_shader_program.set_view_matrix(g_view_matrix);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // ————— RENDERING THE SCENE (i.e. map, character, enemies...) ————— //
    if (g_current_scene == g_level_c) {
        g_shader_program.set_view_matrix(g_view_matrix);
        g_current_scene->render(&g_shader_program);
    }
    else if (g_current_scene->get_state().player->get_curr_health() <= 0.0f) {
        g_gray_aura_program.set_view_matrix(g_view_matrix);
        g_current_scene->render(&g_gray_aura_program);
    } else {
        g_shader_program.set_view_matrix(g_view_matrix);
        g_current_scene->render(&g_shader_program);
    }
    
    
//    g_current_scene->render(&g_shader_program);
    
    
    SDL_GL_SwapWindow(g_display_window);

    
}

void shutdown()
{
    SDL_Quit();
    
    // ————— DELETING LEVEL A DATA (i.e. map, character, enemies...) ————— //
    delete g_menu;
    delete g_level_a;
    delete g_level_b;
    delete g_level_c;
}

// ————— GAME LOOP ————— //
int main(int argc, char* argv[])
{
    initialise();
    
    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        if (g_current_scene->complete) {
            if (g_current_scene->get_state().next_scene_id <= 3) {
                switch_to_scene(scenes[g_current_scene->get_state().next_scene_id]);
            }
            
        }
        render();
    }
    
    shutdown();
    return 0;
}
