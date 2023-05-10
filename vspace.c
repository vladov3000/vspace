#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

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
  "layout (location = 0) in vec3 position;\n"
  "layout (location = 1) in vec2 vertex_uv;\n"
  "out vec2 uv;\n"
  "void main() {\n"
  "  gl_Position = vec4(position, 1);\n"
  "  uv = vertex_uv;\n"
  "}\n";

static GLchar fragment_shader_source[] =
  "#version 330 core\n"
  "in vec2 uv;"
  "out vec4 color;"
  "void main() {"
  "  color = vec4(1, uv.y, uv.x, 1);"
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
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
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

int main() {
  int exit_code = EXIT_SUCCESS;
  
  glfwSetErrorCallback(glfw_error_callback);
  
  assert(glfwInit());

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  GLFWwindow* window = glfwCreateWindow(
    640,
    480,
    "vspace",
    NULL,
    NULL
  );
  assert(window != NULL);

  glfwSetKeyCallback(window, glfw_key_callback);
  glfwMakeContextCurrent(window);

  void (*glGenVertexArrays)(GLsizei n, GLuint* arrays) =
    (void*) glfwGetProcAddress("glGenVertexArrays");
  void (*glBindVertexArray)(GLuint array) =
    (void*) glfwGetProcAddress("glBindVertexArray");

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
 
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(shader_program);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  glfwSwapBuffers(window);

  printf("Error: 0x%x\n", glGetError());

  while (!glfwWindowShouldClose(window)) {
    glfwWaitEvents();
  }

 CLEANUP_WINDOW:
  glfwDestroyWindow(window);
 CLEANUP_GLFW:
  glfwTerminate();
  return exit_code;
}

