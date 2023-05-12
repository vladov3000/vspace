#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <limits.h>

#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

typedef uint8_t u8;

typedef enum {
  PERLIN_MODE,
  COLOR_PERLIN_MODE,
} Mode;

typedef struct {
  float x;
  float y;
} Vec2;

typedef struct {
  float red;
  float green;
  float blue;
  float alpha;
} Color;

typedef struct {
  Mode  mode;
  float speed;
  float scale;
} WindowData;

static float vertices[] = {
   1,  1,  0,  1,  1,
   1, -1,  0,  1,  0,
  -1, -1,  0,  0,  0,
  -1,  1,  0,  0,  1,
};

static int indices[] = {
  0, 1, 3,
  1, 2, 3,
};

static GLchar vertex_shader_source[] =
  "#version 330 core\n"
  "layout (location = 0) in vec3 position;"
  "layout (location = 1) in vec2 vertex_uv;"
  "out vec2 uv;"
  "void main() {"
  "  gl_Position = vec4(position, 1);"
  "  uv = vertex_uv;"
  "}";

static GLchar fragment_shader_source[] =
  "#version 330 core\n"
  "in vec2 uv;"
  "out vec4 color;"
  "uniform sampler2D mesh_texture;"
  "void main() {"
  "  color = texture(mesh_texture, uv);"
  "}";

static void glfw_error_callback(int error, const char* description) {
  printf("GLFW ERROR: %d %s.\n", error, description);
}

static void glfw_key_callback(
  GLFWwindow* window,
  int key,
  int scancode,
  int action,
  int mods
) {
  WindowData* wd = glfwGetWindowUserPointer(window);
  
  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_ESCAPE)
      glfwSetWindowShouldClose(window, GL_TRUE);
    if (key == GLFW_KEY_M)
      wd->mode = wd->mode == PERLIN_MODE ? COLOR_PERLIN_MODE : PERLIN_MODE;
    if (key == GLFW_KEY_UP)
      wd->speed += 5;
    if (key == GLFW_KEY_DOWN)
      wd->speed -= 5;
    if (key == GLFW_KEY_EQUAL)
      wd->scale += 1;
    if (key == GLFW_KEY_MINUS)
      wd->scale -= 1;
  }
}

static void framebuffer_size_callback(
  GLFWwindow* window,
  int width,
  int height
) {
  glViewport(0, 0, width, height);
}

static GLuint compile_shader(
  GLenum type,
  const GLchar* source,
  GLint length
) {
  GLuint shader = glCreateShader(type);

  glShaderSource(shader, 1, &source, &length);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  
  if (success)
    return shader;

  GLint log_length;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
  
  GLchar* log = malloc(sizeof(GLchar) * log_length);
  glGetShaderInfoLog(shader, log_length, NULL, log);
  
  puts("Failed to compile shader:");
  puts(log);

  glDeleteShader(shader);
  free(log);
  return 0;
}

static GLuint link_shader_program(
  GLuint vertex_shader,
  GLuint fragment_shader
) {
  GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);

  GLint success;
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  
  if (success) {
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return shader_program;
  }
  
  GLint log_length;
  glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &log_length);
  
  GLchar* log = malloc(sizeof(GLchar) * log_length);
  glGetProgramInfoLog(shader_program, log_length, NULL, log);
  
  puts("Failed to compile shader:");
  puts(log);

  glDeleteProgram(shader_program);
  free(log);
  return 0;
}

float perlin_interpolate(float a, float b, float w) {
  if (0 > w) return a;
  if (1 < w) return b;
  return (b - a) * w + a;
  // return (b - a) * (3 - 2 * w) * w * w + a;
  // return (b - a) * ((w * (w * 6 - 15) + 10) * w * w * w) + a;
}

Vec2 perlin_random_gradient(int ix, int iy) {
  const unsigned w = CHAR_BIT * sizeof(unsigned);
  const unsigned s = w / 2;

  unsigned a = ix, b = iy;
  a *= 3284157443;
  b ^= a << s | a >> (w - s);
  b *= 1911520717;
  a ^= b << s | b >> (w - s);
  a *= 2048419325;
  
  float random = a * (3.14159265 / ~(~0u >> 1));
  return (Vec2) { .x = cos(random), .y = sin(random) };
}

float perlin_dot_gradient(int ix, int iy, float x, float y) {
  Vec2 gradient = perlin_random_gradient(ix, iy);

  float dx = x - (float) ix;
  float dy = y - (float) iy;

  return dx * gradient.x + dy * gradient.y;
}

// Perlin noise from:
// https://en.wikipedia.org/wiki/Perlin_noise#Implementation
float perlin(float x, float y) {
  int   x0  = floor(x), x1 = x0 + 1;
  int   y0  = floor(y), y1 = y0 + 1;
  float sx  = x - x0  , sy = y - y0;
  float n0  = perlin_dot_gradient(x0, y0, x, y);
  float n1  = perlin_dot_gradient(x1, y0, x, y);
  float ix0 = perlin_interpolate(n0, n1, sx);
  float n2  = perlin_dot_gradient(x0, y1, x, y);
  float n3  = perlin_dot_gradient(x1, y1, x, y);
  float ix1 = perlin_interpolate(n2, n3, sx);
  return perlin_interpolate(ix0, ix1, sy);
}

int main() {
  int exit_code = EXIT_SUCCESS;
  
  glfwSetErrorCallback(glfw_error_callback);
  
  assert(glfwInit());

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  GLFWwindow* window = glfwCreateWindow(
    600,
    600,
    "vspace",
    NULL,
    NULL
  );
  assert(window != NULL);

  WindowData wd = {
    .mode  = COLOR_PERLIN_MODE,
    .speed = 0,
    .scale = 10,
  };

  glfwSetKeyCallback(window, glfw_key_callback);
  glfwSetWindowUserPointer(window, &wd);
  glfwMakeContextCurrent(window);

  GLuint vertex_shader = compile_shader(
    GL_VERTEX_SHADER,
    vertex_shader_source,
    sizeof vertex_shader_source
  );
  
  GLuint fragment_shader = compile_shader(
    GL_FRAGMENT_SHADER,
    fragment_shader_source,
    sizeof fragment_shader_source
  );
  
  if (vertex_shader == 0 || fragment_shader == 0)
    goto CLEANUP_WINDOW;

  GLuint shader_program = link_shader_program(
    vertex_shader,
    fragment_shader
  );

  if (shader_program == 0)
    goto CLEANUP_WINDOW;

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  glViewport(0, 0, width, height);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  GLuint vbo, ebo, vao, buffers[2];
  glGenBuffers(2, buffers);
  glGenVertexArrays(1, &vao);
  vbo = buffers[0], ebo = buffers[1];

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

  glBufferData(
    GL_ARRAY_BUFFER,
    sizeof vertices,
    vertices,
    GL_STATIC_DRAW
  );
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER,
    sizeof indices,
    indices,
    GL_STATIC_DRAW
  );
  
  glVertexAttribPointer(
    0,
    3,
    GL_FLOAT,
    GL_FALSE,
    5 * sizeof(float),
    (void*) 0
  );
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(
    1,
    2,
    GL_FLOAT,
    GL_FALSE,
    5 * sizeof(float),
    (void*) (3 * sizeof(float))
  );
  glEnableVertexAttribArray(1);

  #define TEXTURE_WIDTH  300
  #define TEXTURE_HEIGHT 300

  int   octaves = 4;
  float persistence = 0.5;
  float lacunarity = 2;

  int octave_offsets[4];

  static u8 texture_data[TEXTURE_HEIGHT][TEXTURE_WIDTH][4];

  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 

  //  printf("Error: 0x%x\n", glGetError());

  float offset_x = 0;
  float offset_y = 0;
  double last_time = glfwGetTime();

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    double time = glfwGetTime();
    double dt = time - last_time;
    last_time = time;
    
    offset_x += dt * wd.speed;
    offset_y += dt * wd.speed;

    for (int y = 0; y < TEXTURE_HEIGHT; y++) {
      for (int x = 0; x < TEXTURE_WIDTH; x++) {

        float amplitude = 1;
        float frequency = 1;
        float noise = 0;
        float max_noise = 0;

        for (int i = 0; i < octaves; i++) {
          float sample_x = (x - offset_x) / wd.scale * frequency;
          float sample_y = (y - offset_y) / wd.scale * frequency;
          float perlin_noise = perlin(sample_x, sample_y) / 2 + 0.5;
          noise += perlin_noise * amplitude;
          max_noise += amplitude;
          amplitude *= persistence;
          frequency *= lacunarity;
        }

        noise /= max_noise;

        if (wd.mode == PERLIN_MODE) {
          texture_data[y][x][0] = 255 * noise;
          texture_data[y][x][1] = 255 * noise;
          texture_data[y][x][2] = 255 * noise;
        } else if (wd.mode == COLOR_PERLIN_MODE) {
          if (noise < 0.49) {
            texture_data[y][x][0] = 50;
            texture_data[y][x][1] = 50;
            texture_data[y][x][2] = 255;
          } else if (noise < 0.52) {
            texture_data[y][x][0] = 225;
            texture_data[y][x][1] = 225;
            texture_data[y][x][2] = 100;
          } else if (noise < 0.56) {
            texture_data[y][x][0] = 50;
            texture_data[y][x][1] = 175;
            texture_data[y][x][2] = 50;
          } else if (noise < 0.58) {
            texture_data[y][x][0] = 0;
            texture_data[y][x][1] = 150;
            texture_data[y][x][2] = 0;
          } else if (noise < 0.65) {
            texture_data[y][x][0] = 100;
            texture_data[y][x][1] = 90;
            texture_data[y][x][2] = 90;
          } else {
            texture_data[y][x][0] = 255;
            texture_data[y][x][1] = 255;
            texture_data[y][x][2] = 255;
          }
        }

        texture_data[y][x][3] = 255;
      }
    }

    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA,
      TEXTURE_WIDTH,
      TEXTURE_HEIGHT,
      0,
      GL_RGBA,
      GL_UNSIGNED_BYTE,
      texture_data
    );
    
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shader_program);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glfwSwapBuffers(window);
  }

 CLEANUP_WINDOW:
  glfwDestroyWindow(window);
 CLEANUP_GLFW:
  glfwTerminate();
  return exit_code;
}

