
/**
* Author: Kaitlyn Huynh
* Assignment: Simple 2D Scene
* Date due: 2024-09-28, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH  = 640 * 2,
              WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED     = 0.9765625f,
                BG_GREEN   = 0.97265625f,
                BG_BLUE    = 0.9609375f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X      = 0,
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
                LEVEL_OF_DETAIL    = 0, // mipmap reduction image level
                TEXTURE_BORDER     = 0; // this value MUST be zero

constexpr char MARIO_SPRITE_FILEPATH[]    = "assets/mario.png",
               GOOMBA_SPRITE_FILEPATH[] = "assets/goomba.png";

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
          g_mario_matrix,
          g_goomba_matrix,
          g_projection_matrix;

float g_previous_ticks = 0.0f;

GLuint g_mario_texture_id,
       g_goomba_texture_id;

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("MARIO WORLD!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_mario_matrix       = glm::mat4(1.0f);
    g_goomba_matrix     = glm::mat4(1.0f);
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(1.0f, 1.0f, 0.8f, 1.0f); // Pastel yellow (lighter version of yellow)


    g_mario_texture_id   = load_texture(MARIO_SPRITE_FILEPATH);
    g_goomba_texture_id = load_texture(GOOMBA_SPRITE_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}

// Initialize incremental variables
int ticks = 0;
int heartbeat_ticks = 0;
constexpr int   G_MAX_FRAME     = 40;
constexpr float G_GROWTH_FACTOR = 1.30f; // Grow by 30%
constexpr float G_SHRINK_FACTOR = 0.70f; // Shrink by 30%
constexpr float ROT_INCREMENT = 1.0f;
bool g_is_growing = true;
float ROT_ANGLE = 0.0f;
float prev_ticks = 0.0f;
float mario_angle = 0.0f;
float goomba_angle = 0.0f;
float mario_radius = 0.75f;
float goomba_radius = 1.5;
glm::vec3 mario_circular_translate = glm::vec3(0.0f);
glm::vec3 goomba_circular_translate = glm::vec3(0.0f);
glm::vec3 rotate_z_plane = glm::vec3(0.0f, 0.0f, 1.0f);

void update()
{
    /* Update incremental variables */
    float curr_ticks = (float) SDL_GetTicks() / 1000.0f;
    float delta_time = curr_ticks - prev_ticks;
    mario_angle += ROT_INCREMENT * delta_time;
    goomba_angle += ROT_INCREMENT * delta_time;

    prev_ticks = curr_ticks;
    ROT_ANGLE += ROT_INCREMENT * delta_time;
    
    /* Set transformation vectors */
    mario_circular_translate = glm::vec3(mario_radius * cos(mario_angle), mario_radius * sin(mario_angle), 0.0f);
    goomba_circular_translate = glm::vec3(goomba_radius * cos(goomba_angle), goomba_radius * sin(goomba_angle), 0.0f);
    
    /* Transformations */
    ticks++;
    heartbeat_ticks++;
    glm::vec3 scale_vector;
    if (heartbeat_ticks >= G_MAX_FRAME) {
        g_is_growing = !g_is_growing;
        heartbeat_ticks = 0;
    }
    scale_vector       = glm::vec3(
                                       g_is_growing ? G_GROWTH_FACTOR : G_SHRINK_FACTOR,
                                       g_is_growing ? G_GROWTH_FACTOR : G_SHRINK_FACTOR,
                                       1.0f
                                       );
    
    g_mario_matrix  = glm::translate(glm::mat4(1.0f), mario_circular_translate);
    g_goomba_matrix  = glm::translate(g_mario_matrix, goomba_circular_translate);
    g_goomba_matrix = glm::rotate(g_goomba_matrix, ROT_ANGLE, rotate_z_plane);
    g_goomba_matrix = glm::scale(g_goomba_matrix, scale_vector);
    
}


void draw_object(glm::mat4 &object_g_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // Object 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // Object 2
    };

    // Textures
    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // Object 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // Object 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
                          0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
                          false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Set object to render a texture
    draw_object(g_mario_matrix, g_mario_texture_id);
    draw_object(g_goomba_matrix, g_goomba_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


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
