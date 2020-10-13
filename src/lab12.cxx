#include <iostream>
#include <fstream>

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// Angel
#include "Angel.h"

// Std_image
#include "stb_image.h"

#include <vector>

#include <map>
#include <u_trace.h>

#include "defs.h"

//  Globally use our namespace in our example programs.
using namespace Angel;
using std::vector;
using std::map;

#define INLINE inline


const vec3 WHITE(1.0, 1.0, 1.0);
const vec3 BG(0.0, 0.0, 0.0);
const vec3 BLACK(0.0, 0.0, 0.0);
const vec3 RED(1.0, 0.0, 0.0);
const vec3 GREEN(0.0, 1.0, 0.0);
const vec3 BLUE(0.0, 0.0, 1.0);

const int CIRCLE_NUM_POINTS  = 100;
const int ELLIPSE_NUM_POINTS = 100;

const int TRIANGLE_NUM_POINTS = 3;
const int SQUARE_NUM          = 6;
const int SQUARE_NUM_POINTS = 4 * SQUARE_NUM;
const int SQUARE_START     = TRIANGLE_NUM_POINTS;
const int CIRCLE_START     = TRIANGLE_NUM_POINTS + SQUARE_NUM_POINTS;
const int ELLIPSE_START    = CIRCLE_START + CIRCLE_NUM_POINTS;
const int TOTAL_NUM_POINTS =
            TRIANGLE_NUM_POINTS + SQUARE_NUM_POINTS + CIRCLE_NUM_POINTS +
            ELLIPSE_NUM_POINTS;

const BOOL ENABLE_POLYGON_MODE = false ;

// Window dimensions
const GLuint WIDTH = 1024, HEIGHT = 1024;


// Default vertex array object
static UINT32 VAO;

// Default shader program, merely does nothing.
static INT32 shaderProgram;


// Function prototypes
void
key_callback(GLFWwindow *window, INT32 key, INT32 scancode, INT32 action,
             INT32 mode);


// 获得圆上的点
vec2 getEllipseVertex(const vec2 &center, double scale, double verticalScale,
                      double angle) {
  vec2 vertex(sin(angle), cos(angle));
  vertex *= scale;
  vertex.y *= verticalScale;
  vertex += center;
  return vertex;
}


// 根据角度生成颜色
float generateAngleColor(double angle) {
  return (float) (1.0 / (2 * M_PI) * (angle - (0.0 * M_PI)));
}

// 获得三角形的每个角度
double getTriangleAngle(int point) {
  return 2 * M_PI / 3 * point;
}

// 获得正方形的每个角度
double getSquareAngle(int point) {
  return M_PI / 4 + (M_PI / 2 * point);
}

void generateEllipsePoints(vec2 vertices[], vec3 colors[], int startVertexIndex,
                           int numPoints,
                           const vec2 &center, double scale,
                           double verticalScale) {
  double angleIncrement = (2 * M_PI) / numPoints;
  double currentAngle   = 0;

  for (int i = startVertexIndex; i < startVertexIndex + numPoints; ++i) {
    vertices[i] = getEllipseVertex(center, scale, verticalScale, currentAngle);
    if (verticalScale == 1.0) {
      colors[i] = vec3(generateAngleColor(currentAngle), 0.0, 0.0);
    } else {
      colors[i] = RED;
    }
    currentAngle += angleIncrement;
  }
}

void
generateTrianglePoints(vec2 vertices[], vec3 colors[], int startVertexIndex) {
  double scale = 0.25;
  vec2   center(0.0, 0.70);

  for (int i = 0; i < 3; ++i) {
    double currentAngle = getTriangleAngle(i);
    vertices[startVertexIndex + i] =
      vec2(sin(currentAngle), cos(currentAngle)) * scale + center;
  }

  colors[startVertexIndex]     = RED;
  colors[startVertexIndex + 1] = BLUE;
  colors[startVertexIndex + 2] = GREEN;
}

void generateSquarePoints(vec2 vertices[], vec3 colors[], int squareNumber,
                          int startVertexIndex) {
  double scale         = 0.90;
  double scaleDecrease = 0.15;
  vec2   center(0.0, -0.25);
  int    vertexIndex   = startVertexIndex;

  for (int i = 0; i < squareNumber; ++i) {
    vec3 currentColor;
    currentColor = (i % 2) ? BLACK : WHITE;
    for (int j = 0; j < 4; ++j) {
      double currentAngle = getSquareAngle(j);
      vertices[vertexIndex] =
        vec2(sin(currentAngle), cos(currentAngle)) * scale + center;
      colors[vertexIndex]   = currentColor;
      vertexIndex++;
    }
    scale -= scaleDecrease;
  }
}

// Initialize GL and the vector indicies
void init() {
  vector<vec2> *datas = new vector<vec2>;
  datas->resize(TOTAL_NUM_POINTS);

  vector<vec3> *color_datas = new vector<vec3>;
  color_datas->resize(TOTAL_NUM_POINTS);

  vec2 *vertices = datas->data();
  vec3 *colors   = color_datas->data();

  // 生成各种形状上的点
  generateTrianglePoints(vertices, colors, 0);
  INT32 total_num = TRIANGLE_NUM_POINTS;

  generateSquarePoints(vertices, colors, SQUARE_NUM, total_num);
  total_num += SQUARE_NUM_POINTS;

  vec2 center_circle(0.7, 0.75);
  vec2 center_ellipse(-0.7, 0.75);

  /*生成圆形和椭圆上的点和颜色*/
  Is_True(total_num == CIRCLE_START, ("Incorrect CIRCLE_START = %d", CIRCLE_START));
  generateEllipsePoints(vertices, colors, total_num, CIRCLE_NUM_POINTS,
                        center_circle, 0.2, 1.0);
  total_num += CIRCLE_NUM_POINTS;

  Is_True(total_num == ELLIPSE_START, ("Incorrect ELLIPSE_START = %d", CIRCLE_START));
  generateEllipsePoints(vertices, colors, total_num, ELLIPSE_NUM_POINTS,
                        center_ellipse, 0.2, 0.7);
  total_num += ELLIPSE_NUM_POINTS;

  // 创建顶点数组对象
  GLuint vao[1];
  glGenVertexArrays(1, vao);
  glBindVertexArray(vao[0]);
  VAO = vao[0];

  INT32 verticies_count_size = TOTAL_NUM_POINTS * sizeof(vec2);
  INT32 colors_count_size    = TOTAL_NUM_POINTS * sizeof(vec3);

  // 创建并初始化顶点缓存对象
  GLuint buffer;
  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, verticies_count_size + colors_count_size, NULL,
               GL_DYNAMIC_DRAW);

  // 分别读取数据
  glBufferSubData(GL_ARRAY_BUFFER, 0, verticies_count_size, vertices);
  glBufferSubData(GL_ARRAY_BUFFER, verticies_count_size, colors_count_size, colors);


  // 读取着色器并使用
  GLuint program = InitShader(
    "/Users/xc5/CLionProjects/opengl/example2/src/vshader.glsl",
    "/Users/xc5/CLionProjects/opengl/example2/src/fshader.glsl");
  glUseProgram(program);
  shaderProgram = program;


  // 从顶点着色器中初始化顶点的位置
  GLuint pLocation = glGetAttribLocation(program, "vPosition");
  glEnableVertexAttribArray(pLocation);
  glVertexAttribPointer(pLocation, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

  // 从片元着色器中初始化顶点的颜色
  GLuint cLocation = glGetAttribLocation(program, "vColor");
  glEnableVertexAttribArray(cLocation);
  glVertexAttribPointer(cLocation, 3, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(verticies_count_size));

  // 黑色背景
  glClearColor(0.7, 0.3, 0.3, 1.0);

  // uncomment this call to draw in wireframe polygons.
  if (ENABLE_POLYGON_MODE) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  }
}



// Resize callback
void framebuffer_size_callback(GLFWwindow *window, INT32 width, INT32 height) {
  glViewport(0, 0, width, height);
}

const char *Read_file_as_string(const char *filename) {
  std::ifstream ifs(filename);
  std::string   content((std::istreambuf_iterator<char>(ifs)),
                        (std::istreambuf_iterator<char>()));
  char          *array = new char[content.size() + 2];
  memcpy(array, content.c_str(), content.size());
  array[content.size() + 1] = '\0';
  return array;
}

// process user input.
void processInput(GLFWwindow *window, INT32 shaderProgram, UINT32 VAO) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
}

unsigned int texture;
INT32 Get_new_texture() {
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
// 为当前绑定的纹理对象设置环绕、过滤方式
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
// 加载并生成纹理
  int           width, height, nrChannels;
  unsigned char *data = stbi_load("/Users/xc5/CLionProjects/opengl/example2/wall.jpg", &width, &height, &nrChannels,
                                  0);
  if (data) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    std::cout << "Failed to load texture" << std::endl;
  }
  stbi_image_free(data);
  return texture;
}


INLINE void refreshFrame(GLFWwindow *window, INT32 shaderProgram, INT32 VAO) {
  // 清理窗口
  glClearColor(BG.x, BG.y, BG.z, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(BG.x, BG.y, BG.z, 1.0);

  glBindVertexArray(VAO);

  // 绘制三角形
  glDrawArrays(GL_TRIANGLES, 0, TRIANGLE_NUM_POINTS);

  // 绘制多个正方形
  for (int i = 0; i < SQUARE_NUM; ++i) {
    glDrawArrays(GL_TRIANGLE_FAN, SQUARE_START + (i * 4), 4);
  }

  // 绘制圆
  glDrawArrays(GL_TRIANGLE_FAN, CIRCLE_START, CIRCLE_NUM_POINTS);

  // 绘制椭圆
  glDrawArrays(GL_TRIANGLE_FAN, ELLIPSE_START, ELLIPSE_NUM_POINTS);

}

// The MAIN function, from here we start the application and run the game loop
int main() {
  std::cout << "Starting GLFW context, Comapt version 3.3" << std::endl;
  // Init GLFW
  glfwInit();
  // Set all the required options for GLFW
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

  // Create a GLFWwindow object that we can use for GLFW's functions
  GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "2017152003_GuantingLu_Shiyan1", NULL,
                                        NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);

  // Set the required callback functions
  glfwSetKeyCallback(window, key_callback);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);


  if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
    std::cout << "Failed to initialize OpenGL context" << std::endl;
    return -1;
  }

  // Define the viewport dimensions
  glViewport(0, 0, WIDTH, HEIGHT);

  const GLubyte *n1 = glGetString(GL_VERSION);
  std::cout << "openGL version string : " << n1 << std::endl;

  // Initialize shaders
  init();

  Get_new_texture();


  // Game loop
  while (!glfwWindowShouldClose(window)) {

    // process input info
    processInput(window, shaderProgram, VAO);

    refreshFrame(window, shaderProgram, VAO);

    // Check if any events have been activated (key pressed, mouse moved etc.) and call corresponding response functions
    glfwPollEvents();

    // Swap the screen buffers
    glfwSwapBuffers(window);
  }

  // Terminates GLFW, clearing any resources allocated by GLFW.
  glfwTerminate();
  return 0;
}

// Is called whenever a key is pressed/released via GLFW
void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
  std::cout << key << std::endl;
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GL_TRUE);
  }
}
