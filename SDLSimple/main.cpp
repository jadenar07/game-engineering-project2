/**
* Author: Jaden Ritchie
* Assignment: Pong Clone
* Date due: 2024-10-12, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>

enum AppStatus { RUNNING, TERMINATED };

constexpr float WINDOW_SIZE_MULT = 1.0f;

constexpr int WINDOW_WIDTH  = 1280 * WINDOW_SIZE_MULT,
              WINDOW_HEIGHT = 960 * WINDOW_SIZE_MULT;

constexpr float BG_RED     = 0.9765625f,
                BG_GREEN   = 0.97265625f,
                BG_BLUE    = 0.9609375f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr char PADDLE1_SPRITE_FILEPATH[] = "88226-200.png",
               PADDLE2_SPRITE_FILEPATH[]  = "88226-200.png",
BALL_SPRITE_FILEPATH[]  = "vecteezy_3d-icon-pillates-ball-isolated-on-transparent-background_23850321.png";

constexpr glm::vec3 INIT_SCALE_PADDLE1 = glm::vec3(1.0f, 1.3985f, 0.0f),
                    INIT_SCALE_PADDLE2 = glm::vec3(1.0f, 1.3985f, 0.0f),
                    INIT_POS_PADDLE1 = glm::vec3(-4.5f, 0.0f, 0.0f),
                    INIT_POS_PADDLE2 = glm::vec3(4.5f, 0.0f, 0.0f),
                    INIT_POS_BALL = glm::vec3(0.0f, 0.0f, 0.0f),
                    INIT_SCALE_BALL = glm::vec3(0.50f, 0.50f, 0.0f);

SDL_Window* g_display_window;

AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();
glm::mat4 g_view_matrix, g_paddle1_matrix, g_projection_matrix, g_paddle2_matrix, g_ball_matrix;

float g_previous_ticks = 0.0f;

GLuint g_paddle1_texture_id, g_paddle2_texture_id, g_ball_texture_id;

float paddle1_top, paddle1_bottom, paddle2_top, paddle2_bottom;

glm::vec3 g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_paddle1_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_paddle1_movement = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_paddle2_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_paddle2_movement = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_ball_movement = glm::vec3(1.0f, 0.2f, 0.0f);


float g_paddle_speed = 10.0f;
float g_ball_speed = 1.0f;

bool auto_mode = false;
float auto_direction = 1.0f;

void initialise();
void process_input();
void update();
void render();
void shutdown();

constexpr GLint NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL = 0;
constexpr GLint TEXTURE_BORDER = 0;

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
    
    stbi_image_free(image);
    
    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Pong Game!!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
    if (g_display_window == nullptr)
    {
        shutdown();
    }
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_ball_matrix = glm::mat4(1.0f);
    g_paddle1_matrix = glm::mat4(1.0f);
    g_paddle2_matrix = glm::mat4(1.0f);
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    
    glUseProgram(g_shader_program.get_program_id());
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    g_ball_texture_id = load_texture(BALL_SPRITE_FILEPATH);
    g_paddle1_texture_id = load_texture(PADDLE1_SPRITE_FILEPATH);
    g_paddle2_texture_id = load_texture(PADDLE2_SPRITE_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    g_paddle1_position = INIT_POS_PADDLE1;
    g_paddle2_position = INIT_POS_PADDLE2;
}

void process_input()
{
    g_paddle2_movement = glm::vec3(0.0f);
    g_paddle1_movement = glm::vec3(0.0f);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_q:
                        g_app_status = TERMINATED;
                        break;
                    case SDLK_t:
                        auto_mode = !auto_mode;
                        break;
                    default:
                        break;
                }
            default:
                break;
        }
    }
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    
    if (key_state[SDL_SCANCODE_W])
    {
        g_paddle1_movement.y = 1.0f;
    }
    else if (key_state[SDL_SCANCODE_S])
    {
        g_paddle1_movement.y = -1.0f;
    }
    
    if (!auto_mode) {
        if (key_state[SDL_SCANCODE_UP])
        {
            g_paddle2_movement.y = 1.0f;
        }
        else if (key_state[SDL_SCANCODE_DOWN])
        {
            g_paddle2_movement.y = -1.0f;
        }
    }
    
    if (glm::length(g_paddle2_movement) > 1.0f)
    {
        g_paddle2_movement = glm::normalize(g_paddle2_movement);
    }
    if (glm::length(g_paddle1_movement) > 1.0f)
    {
        g_paddle1_movement = glm::normalize(g_paddle1_movement);
    }
}

void update()
{
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    g_ball_position += g_ball_movement * g_ball_speed * delta_time;

    g_paddle1_position += g_paddle1_movement * g_paddle_speed * delta_time;

    if (g_paddle1_position.y + (INIT_SCALE_PADDLE1.y / 2.0f) > 3.75f) {
        g_paddle1_position.y = 3.75f - (INIT_SCALE_PADDLE1.y / 2.0f);
    } 
    
    else if (g_paddle1_position.y - (INIT_SCALE_PADDLE1.y / 2.0f) < -3.75f) {
        g_paddle1_position.y = -3.75f + (INIT_SCALE_PADDLE1.y / 2.0f);
    }

    if (auto_mode) {
        g_paddle2_position.y += auto_direction * g_paddle_speed * delta_time;

        if (g_paddle2_position.y + (INIT_SCALE_PADDLE2.y / 2.0f) > 3.75f) {
            g_paddle2_position.y = 3.75f - (INIT_SCALE_PADDLE2.y / 2.0f);
            auto_direction = -auto_direction;
        } else if (g_paddle2_position.y - (INIT_SCALE_PADDLE2.y / 2.0f) < -3.75f) {
            g_paddle2_position.y = -3.75f + (INIT_SCALE_PADDLE2.y / 2.0f);
            auto_direction = -auto_direction;
        }
    } else {

        g_paddle2_position += g_paddle2_movement * g_paddle_speed * delta_time;
    }


    if (g_paddle2_position.y + (INIT_SCALE_PADDLE2.y / 2.0f) > 3.75f) {
        g_paddle2_position.y = 3.75f - (INIT_SCALE_PADDLE2.y / 2.0f);
    } else if (g_paddle2_position.y - (INIT_SCALE_PADDLE2.y / 2.0f) < -3.75f) {
        g_paddle2_position.y = -3.75f + (INIT_SCALE_PADDLE2.y / 2.0f);
    }

    if (g_ball_position.y + 0.5f > 3.75f) {
        g_ball_movement.y = -g_ball_movement.y;
    }

    if (g_ball_position.y - 0.5f < -3.75f) {
        g_ball_movement.y = -g_ball_movement.y;
    }

    if (g_ball_position.x - 0.5f < (g_paddle1_position.x + INIT_SCALE_PADDLE1.x / 2.0f) &&
        g_ball_position.x + 0.5f > (g_paddle1_position.x - INIT_SCALE_PADDLE1.x / 2.0f) &&
        g_ball_position.y - 0.5f < (g_paddle1_position.y + INIT_SCALE_PADDLE1.y / 2.0f) &&
        g_ball_position.y + 0.5f > (g_paddle1_position.y - INIT_SCALE_PADDLE1.y / 2.0f)) {
        
        g_ball_movement.x = -g_ball_movement.x;
        float relative_intersect_y = g_ball_position.y - g_paddle1_position.y;
        float normalized_intersect_y = (relative_intersect_y / (INIT_SCALE_PADDLE1.y / 2.0f));
        g_ball_movement.y = normalized_intersect_y * 2.0f;
    }

    if (g_ball_position.x + 0.5f > (g_paddle2_position.x - INIT_SCALE_PADDLE2.x / 2.0f) &&
        g_ball_position.x - 0.5f < (g_paddle2_position.x + INIT_SCALE_PADDLE2.x / 2.0f) &&
        g_ball_position.y - 0.5f < (g_paddle2_position.y + INIT_SCALE_PADDLE2.y / 2.0f) &&
        g_ball_position.y + 0.5f > (g_paddle2_position.y - INIT_SCALE_PADDLE2.y / 2.0f)) {
        
        g_ball_movement.x = -g_ball_movement.x;
        float relative_intersect_y = g_ball_position.y - g_paddle2_position.y;
        float normalized_intersect_y = (relative_intersect_y / (INIT_SCALE_PADDLE2.y / 2.0f));
        g_ball_movement.y = normalized_intersect_y * 2.0f;
    }

    if (g_ball_position.x + 0.5f > 5.0f) {
        g_app_status = TERMINATED;
        LOG("Player 1 wins!");
    } else if (g_ball_position.x - 0.5f < -5.0f) {
        g_app_status = TERMINATED;
        LOG("Player 2 wins!");
    }

    g_ball_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::translate(g_ball_matrix, g_ball_position);
    g_ball_matrix = glm::scale(g_ball_matrix, INIT_SCALE_BALL);

    g_paddle2_matrix = glm::mat4(1.0f);
    g_paddle2_matrix = glm::translate(g_paddle2_matrix, g_paddle2_position);
    g_paddle2_matrix = glm::scale(g_paddle2_matrix, INIT_SCALE_PADDLE2);

    g_paddle1_matrix = glm::mat4(1.0f);
    g_paddle1_matrix = glm::translate(g_paddle1_matrix, g_paddle1_position);
    g_paddle1_matrix = glm::scale(g_paddle1_matrix, INIT_SCALE_PADDLE1);
}

void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };

    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    draw_object(g_paddle2_matrix, g_paddle2_texture_id);
    draw_object(g_paddle1_matrix, g_paddle1_texture_id);
    draw_object(g_ball_matrix, g_ball_texture_id);

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
