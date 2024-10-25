#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 5

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_mixer.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include <cstdlib>
#include "Entity.h"
#include <iostream>
#include <string>

// ––––– STRUCTS AND ENUMS ––––– //
enum AppStatus { RUNNING, TERMINATED };

struct GameState
{
    Entity* player;
    Entity* platforms;
};


// ––––– CONSTANTS ––––– //
constexpr int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

constexpr float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;
constexpr char SPRITESHEET_FILEPATH[] = "assets/george_0.png",
                    MARIO_FILEPATH[] = "assets/mario4.png",
                    GOOMBA_FILEPATH[] = "assets/goomba.png",
                    LUIGI_FILEPATH[] = "assets/luigi.png",
                    GREENPIPE_FILEPATH[] = "assets/greenPipe.png",
                    THWOMP_FILEPATH[] = "assets/thwomp.png",
                    FONT_FILEPATH[] = "assets/font1.png";

constexpr char PLATFORM_FILEPATH[]    = "assets/platformPack_tile027.png";

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL  = 0;
constexpr GLint TEXTURE_BORDER   = 0;


// ––––– GLOBAL VARIABLES ––––– //
GameState g_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_program;
glm::mat4 g_view_matrix, g_projection_matrix;
// Fuel Juice
float juice = 15.0f;
constexpr float juice_decrement = 0.01f;
// Fuel Boost and Decay
constexpr float boost = 2.0f;
constexpr float decay = -0.25f;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;
constexpr float GRAVITY = -1.0f;

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
constexpr int FONTBANK_SIZE = 16;

void draw_text(ShaderProgram* shader_program, GLuint font_texture_id, std::string text,
    float font_size, float spacing, glm::vec3 position)
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
glm::vec3 START_POSITION = glm::vec3(3.0f, 3.0f, 0.0f);
void initialise()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("My lunar lander",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    // ––––– VIDEO ––––– //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_program.set_projection_matrix(g_projection_matrix);
    g_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    // ––––– PLATFORMS ––––– //
    GLuint platform_texture_id = load_texture(PLATFORM_FILEPATH);
    GLuint goomba_texture_id = load_texture(GOOMBA_FILEPATH);
    GLuint luigi_texture_id = load_texture(LUIGI_FILEPATH);
    GLuint greenPipe_texture_id = load_texture(GREENPIPE_FILEPATH);
    GLuint thwomp_texture_id = load_texture(THWOMP_FILEPATH);
    g_state.platforms = new Entity[PLATFORM_COUNT];

    // Set the type of every platform entity to PLATFORM
    for (int i = 1; i < PLATFORM_COUNT - 1; i++)
    {
        float height = (i % 2 == 0 ? 0.5f : -2.5f);
        g_state.platforms[i].set_texture_id(thwomp_texture_id);
        g_state.platforms[i].set_position(glm::vec3(i - PLATFORM_COUNT / 2.0f, height, 0.0f));
        g_state.platforms[i].set_width(0.5f);
        g_state.platforms[i].set_height(0.5f);
        g_state.platforms[i].update(0.0f, NULL, 0.0f);
    }
    // Starting platform //
    g_state.platforms[0].set_texture_id(greenPipe_texture_id);
    g_state.platforms[0].set_position(glm::vec3(3.0f, 1.0f, 0.0f));
    g_state.platforms[0].set_width(0.5f);
    g_state.platforms[0].set_height(0.5f);
    g_state.platforms[0].update(0.0f, NULL, 0.0f);
    
    // Goal platform Luigi
    g_state.platforms[PLATFORM_COUNT- 1].set_texture_id(luigi_texture_id);
    g_state.platforms[PLATFORM_COUNT - 1].set_position(glm::vec3(-2.0f, 0.5f, 0.0f));
    g_state.platforms[PLATFORM_COUNT - 1].set_width(0.5f);
    g_state.platforms[PLATFORM_COUNT - 1].set_height(0.5f);
    g_state.platforms[PLATFORM_COUNT - 1].update(0.0f, NULL, 0.0f);
    
    // ––––– PLAYER (MARIO) ––––– //
    GLuint player_texture_id = load_texture(MARIO_FILEPATH);

    glm::vec3 acceleration = glm::vec3(0.0f, GRAVITY, 0.0f);
    g_state.player = new Entity();
    g_state.player->set_texture_id(player_texture_id);
    g_state.player->set_acceleration(acceleration);
    g_state.player->set_position(START_POSITION);
    g_state.player->set_width(1.0f);
    g_state.player->set_height(1.5f);
    

    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_game_is_running = false;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_game_is_running = false;
                        break;
                    default:
                        break;
                }

            default:
                break;
        }
    }

    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    if (juice <= 0) { // Ran out of juice to use
        juice = 0.0f;
        return;
    }
    if (key_state[SDL_SCANCODE_UP] && key_state[SDL_SCANCODE_LEFT]) {
        g_state.player->move_up();
        g_state.player->move_left();
        juice -= juice_decrement;
    }
    else if (key_state[SDL_SCANCODE_UP] && key_state[SDL_SCANCODE_RIGHT]) {
        g_state.player->move_up();
        g_state.player->move_right();
        juice -= juice_decrement;
    }
    if (key_state[SDL_SCANCODE_LEFT])
    {
        g_state.player->move_left();
        juice -= juice_decrement;
    }
    else if (key_state[SDL_SCANCODE_RIGHT])
    {
        g_state.player->move_right();
        juice -= juice_decrement;
    }
    else if (key_state[SDL_SCANCODE_UP])
    {
        g_state.player->move_up();
        juice -= juice_decrement;
    }
    
    if (glm::length(g_state.player->get_movement()) > 1.0f)
    {
        g_state.player->normalise_movement();
    }
}
bool goalTouched = false;
bool platformTouched = false;
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
        glm::vec3 boost_acceleration = g_state.player->get_movement() * boost;
        float slow_down = g_state.player->get_velocity().x * decay;
        
        glm::vec3 player_acceleration = boost_acceleration + glm::vec3(slow_down, GRAVITY, 0.0f);
         
        if (!platformTouched && !goalTouched) { // If game is still running, render changes
            g_state.player->set_acceleration(player_acceleration);
            g_state.player->update(FIXED_TIMESTEP, g_state.platforms, PLATFORM_COUNT);
        }
        
        
        // Check for collision with GOAL or PLATFORM, set booleans to render end game message
        for (int i = 1; i < PLATFORM_COUNT - 1; i++) {
            if (g_state.player->get_collided_with() == &g_state.platforms[i]) {
                platformTouched = true;
            }
        }
        if (g_state.player->get_collided_with() == &g_state.platforms[PLATFORM_COUNT - 1]) {
            goalTouched = true;
        }
        
        
        
        delta_time -= FIXED_TIMESTEP;
    }
    
    

    g_accumulator = delta_time;
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    g_state.player->render(&g_program);
    GLuint font_texture_id = load_texture(FONT_FILEPATH);
    std::string msg = "Fuel: " + std::to_string(juice);
    draw_text(&g_program, font_texture_id, msg, 0.5f, 0.001f, glm::vec3(-2.0f, 3.0f, 0.0f));
    if (platformTouched) {
        draw_text(&g_program, font_texture_id, "You lose", 0.5f, 0.001f, glm::vec3(-2.0f, 2.0f, 0.0f));
    } else if (goalTouched) {
        draw_text(&g_program, font_texture_id, "You win", 0.5f, 0.001f, glm::vec3(-2.0f, 2.0f, 0.0f));
    }
    for (int i = 0; i < PLATFORM_COUNT; i++) g_state.platforms[i].render(&g_program);
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();

    delete [] g_state.platforms;
    delete g_state.player;
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
