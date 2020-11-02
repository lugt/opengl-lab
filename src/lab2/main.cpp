// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// Angel
#include "Angel.h"

// Std_image
#include "stb_image.h"

#include <vector>
#include <string>
#include <fstream>

// Definitions of types...
#include "defs.h"

// Tracing
#include <u_trace.h>

#include <u_colors.h>
#include <sys/file.h>
#include <sys/stat.h>

// Shader
#include <shader_s.h>

// Imaging
#include <stb_image.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "unistd.h"

#define INLINE inline

// 三角面片中的顶点序列

// Window dimensions
const GLuint WIDTH = 1024, HEIGHT = 1024;

// Default vertex array object
static UINT32 VAO;

// Default shader program, merely does nothing.
static INT32 shaderProgram;

static UINT32 total_points_size = 0;


const int NUM_VERTICES = 8;

const vec3 basic_universal_colors[NUM_VERTICES] = {
	vec3(1.0, 1.0, 1.0),  // White
	vec3(1.0, 1.0, 0.0),  // Yellow
	vec3(0.0, 1.0, 0.0),  // Green
	vec3(0.0, 1.0, 1.0),  // Cyan
	vec3(1.0, 0.0, 1.0),  // Magenta
	vec3(1.0, 0.0, 0.0),  // Red
	vec3(0.0, 0.0, 0.0),  // Black
	vec3(0.0, 0.0, 1.0)   // Blue
};


using std::vector;


struct VEC4I_T {
  union {
    struct {
      UINT32 a;
      UINT32 b;
      UINT32 c;
      UINT32 color;
    };
    struct {
      UINT32 x,y,z;
    };
    UINT32 _val[4];
  };
};
typedef VEC4I_T VEC4I;

class VEC3_READER {
public:
  void Read(std::ifstream &input, vec3 & output) {
    FLOAT64 x, y, z;
    input >> x >> y >> z;
    output.x = x;
    output.y = y;
    output.z = z;
  }
};

class VEC4_READER {
  static UINT32 color_indicator;
public:
  void Read(std::ifstream &input, VEC4I &output) {
    UINT32 x, y, z, count;
    input >> count >>  x >> y >> z;
    Is_True(count == 3, ("Should be 3 points in a facet, instead of %d.", count));
    output.x = x;
    output.y = y;
    output.z = z;
    output.color = color_indicator % 8;
    color_indicator ++;
  }
};

UINT32 VEC4_READER::color_indicator = 0;


class OFFFile {
  vector<vec3>     _vertex;
  vector<VEC4I>    _faces;
  UINT32           _n_vertex;
  UINT32           _n_faces;
  UINT32           _n_lines;

  OFFFile(const OFFFile &) = delete;  // Copy not allowed
  OFFFile(const OFFFile &&) = delete; // Move not allowed

public:
  OFFFile():
    _vertex(0), _faces(0),
    _n_vertex(0), _n_faces(0), _n_lines(0) {};

  OFFFile(UINT32 n_v, UINT32 n_f,  UINT32 n_l) :
    _vertex(0), _faces(0),
    _n_vertex(n_v), _n_faces(n_f), _n_lines(n_l) {}

  vector<vec3> &getVertex() { return _vertex; }

  vector<VEC4I> &getFaces() { return _faces; }

  UINT32 getNVertex() const {
    return _n_vertex;
  }

  void setNVertex(UINT32 nVertex) {
    _n_vertex = nVertex;
  }

  UINT32 getNFaces() const {
    return _n_faces;
  }

  void setNFaces(UINT32 nFaces) {
    _n_faces = nFaces;
  }

  UINT32 getNLines() const {
    return _n_lines;
  }

  void setNLines(UINT32 nLines) {
    _n_lines = nLines;
  }

  template<typename T, typename LSTITEM, typename READER>
  void Read_list_from_istream(UINT32 size,
                              std::ifstream &input,
                              vector<LSTITEM> &output) {
    output.resize(size);
    READER r;
    for (UINT32 i = 0; i < size; ++i) {
      LSTITEM& cur_item = output.at(i);
      r.Read(input, cur_item);
    }
  }

  void Read_from_file(const std::string &off_file_name) {
    OFFFile &off = *this;
    if (off_file_name.empty()) {
      return;
    }

    Is_True((access(off_file_name.c_str(), F_OK) != -1), ("OFF file not existing"));

    UINT32 n_vertex = 0, n_face = 0, n_lines = 0;
    std::ifstream fin;
    std::string first_line;
    fin.open(off_file_name);

    // @TODO: 修改此函数读取OFF文件中三维模型的信息
    std::getline(fin, first_line);
    fin >> n_vertex >> n_face >> n_lines;
    off.setNVertex(n_vertex);
    off.setNFaces(n_face);
    off.setNLines(n_lines);
    off.getFaces().resize(n_face);
    Read_list_from_istream<FLOAT64, vec3, VEC3_READER>(n_vertex, fin, off.getVertex());
    Read_list_from_istream<UINT32, VEC4I, VEC4_READER>(n_face, fin, off.getFaces());
    fin.close();
  }

  void Get_facet_points(vector<vec3> &points, vector<vec3> &colors) {
    for (UINT32 i = 0; i < getFaces().size(); i++) {
      // Dump the points in the triangle.
      VEC4I &triangle = getFaces().at(i);
      for (UINT32 j = 0; j < 3; j++) {
        Is_True(triangle.color < sizeof(basic_universal_colors) / sizeof(vec3), ("Not a valid color : %d", triangle.color));
        points.push_back(getVertex().at(triangle._val[j]));
        colors.push_back(basic_universal_colors[triangle.color]);
      }
    }
  }
};


// Transformations

const int X_AXIS = 0;
const int Y_AXIS = 1;
const int Z_AXIS = 2;

enum {
  TRANSFORM_SCALE = 0,
  TRANSFORM_ROTATE = 1,
  TRANSFORM_TRANSLATE = 2
};

const double DELTA_DELTA = 0.01;    // Delta的变化率
const double DEFAULT_DELTA = 0.03;    // 默认的Delta值

double scaleDelta = DEFAULT_DELTA;
double rotateDelta = DEFAULT_DELTA;
double translateDelta = DEFAULT_DELTA;

vec3 scaleTheta(1.0, 1.0, 1.0);    // 缩放控制变量
vec3 rotateTheta(0.0, 0.0, 0.0);    // 旋转控制变量
vec3 translateTheta(0.0, 0.0, 0.0);    // 平移控制变量

GLint matrixLocation;
int currentTransform = TRANSFORM_TRANSLATE;    // 设置当前变换


void storeFacesPoints(const char *file_path, vector<vec3> &points,
                      vector <vec3> &colors)
{
  // Define a struct to contain the OFF file contents.
  OFFFile off;

  // Read OFF file from scratch.
  off.Read_from_file(file_path);

  // @TODO: 修改此函数在points和colors容器中存储每个三角面片的各个点和颜色信息
  points.clear();
  colors.clear();

  // Transform off content to points and colors.
  off.Get_facet_points(points, colors);

  // Set to total size;
  total_points_size = points.size();
}

// Common shader
Shader *ourShader = nullptr;

void init()
{
  std::vector<vec3> points;   //传入着色器的绘制点
  std::vector<vec3> colors;   //传入着色器的颜色

  storeFacesPoints("/Users/xc5/CLionProjects/opengl/example2/Models/cow.off",
                   points, colors);

  // 创建顶点数组对象
	GLuint vao[1];
	glGenVertexArrays(1, vao);
	glBindVertexArray(vao[0]);

	Is_True(colors.size() > 0, ("Colors size is zero."));
  Is_True(points.size() > 0, ("Points size is zero."));


	// 创建并初始化顶点缓存对象
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(vec3) + colors.size() * sizeof(vec3), NULL, GL_STATIC_DRAW);

	// @TODO: 修改完成后再打开下面注释，否则程序会报错
	// 分别读取数据
	glBufferSubData(GL_ARRAY_BUFFER, 0, points.size() * sizeof(vec3), &points[0]);
	glBufferSubData(GL_ARRAY_BUFFER, points.size() * sizeof(vec3), colors.size() * sizeof(vec3), &colors[0]);


  // build and compile our shader zprogram
  // ------------------------------------
  ourShader = new Shader("/Users/xc5/CLionProjects/opengl/example2/src/lab2/vshader.glsl",
                         "/Users/xc5/CLionProjects/opengl/example2/src/lab2/fshader.glsl");
  ourShader->use();

	// 从顶点着色器中初始化顶点的位置
	GLuint pLocation = glGetAttribLocation(ourShader->ID, "vPosition");
	glEnableVertexAttribArray(pLocation);
	glVertexAttribPointer(pLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	// 从片元着色器中初始化顶点的颜色
	GLuint cLocation = glGetAttribLocation(ourShader->ID, "vColor");
	glEnableVertexAttribArray(cLocation);
	glVertexAttribPointer(cLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(points.size() * sizeof(vec3)));

	// 黑色背景
	glClearColor(0.0, 0.0, 0.0, 1.0);
}


enum FUNC_MODES {
  MODE_TRANSLATE_X,
  MODE_TRANSLATE_Y,
  MODE_TRANSLATE_Z,
  MODE_ROTATE_X,
  MODE_ROTATE_Y,
  MODE_ROTATE_Z,
};

// Functionality related var.
BOOL paused = false;
float lastPosition = 0.0f;
vec3 lastTranslate = 0;
FUNC_MODES tranform_mode;

INLINE void refreshFrame(GLFWwindow *window, INT32 shaderProgram, INT32 VAO) {
  // 清理窗口
  Is_True(total_points_size > 0, ("No points to draw"));

  // @TODO: 清理窗口，包括颜色缓存和深度缓存
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // 绘制边
  // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

//  // 生成变换矩阵
//  mat4 m(1.0, 0.0, 0.0, 0.0,
//         0.0, 1.0, 0.0, 0.0,
//         0.0, 0.0, 1.0, 0.0,
//         0.0, 0.0, 0.0, 1.0);
//
//  // @TODO: 在此处修改函数，根据scaleTheta，rotateTheta，translateTheta，计算变换矩阵
//  // 可使用Scale(),Translate(),RotateX(),RotateY(),RotateZ()等函数。函数定义在mat.h
//  // 从指定位置matrixLocation中传入变换矩阵m
//  glUniformMatrix4fv(matrixLocation, 1, GL_TRUE, m);

  // create transformations
  if (!paused) {
    // lastRotation = (float) glfwGetTime();
  }

  glm::mat4 transform = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
  transform = glm::translate(transform, glm::vec3(translateTheta.x, translateTheta.y, translateTheta.z));
  transform = glm::rotate(transform, rotateTheta.x, glm::vec3(1.0, 0.0, 0.0));
  transform = glm::rotate(transform, rotateTheta.y, glm::vec3(0.0, 1.0, 0.0));
  transform = glm::rotate(transform, rotateTheta.z, glm::vec3(0.0, 0.0, 1.0));
  transform = glm::scale(transform, glm::vec3(scaleTheta.x, scaleTheta.y, scaleTheta.z));

  // transform = glm::rotate(transform, (float)lastPosition, glm::vec3(0.0f, 1.0f, 0.0f));

  // get matrix's uniform location and set matrix
  ourShader->use();
  unsigned int transformLoc = glGetUniformLocation(ourShader->ID, "transform");
  glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

  // 消除背面光照
  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);

  glDrawArrays(GL_TRIANGLES, 0, total_points_size);
  // sleep(1);
}

// 通过Delta值更新Theta
void updateTheta(int axis, int sign) {
  switch (currentTransform) {
    case TRANSFORM_SCALE:
      scaleTheta[axis] += sign * scaleDelta;
      break;
    case TRANSFORM_ROTATE:
      rotateTheta[axis] += sign * rotateDelta;
      break;
    case TRANSFORM_TRANSLATE:
      translateTheta[axis] += sign * translateDelta;
      break;
  }
}

// 复原Theta和Delta
void resetTheta()
{
  scaleTheta = vec3(1.0, 1.0, 1.0);
  rotateTheta = vec3(0.0, 0.0, 0.0);
  translateTheta = vec3(0.0, 0.0, 0.0);
  scaleDelta = DEFAULT_DELTA;
  rotateDelta = DEFAULT_DELTA;
  translateDelta = DEFAULT_DELTA;
}

// 更新变化Delta值
void updateDelta(int sign)
{
  switch (currentTransform) {
    case TRANSFORM_SCALE:
      scaleDelta += sign * DELTA_DELTA;
      break;
    case TRANSFORM_ROTATE:
      rotateDelta += sign * DELTA_DELTA;
      break;
    case TRANSFORM_TRANSLATE:
      translateDelta += sign * DELTA_DELTA;
      break;
  }
}

void keyboard(UINT32 key)
{
  switch (key) {
    case GLFW_KEY_Q:
      updateTheta(X_AXIS, 1);
      break;
    case GLFW_KEY_A:
      updateTheta(X_AXIS, -1);
      break;
    case GLFW_KEY_W:
      updateTheta(Y_AXIS, 1);
      break;
    case GLFW_KEY_S:
      updateTheta(Y_AXIS, -1);
      break;
    case GLFW_KEY_E:
      updateTheta(Z_AXIS, 1);
      break;
    case GLFW_KEY_D:
      updateTheta(Z_AXIS, -1);
      break;
    case GLFW_KEY_R:
      updateDelta(1);
      break;
    case GLFW_KEY_F:
      updateDelta(-1);
      break;
    case GLFW_KEY_T:
      resetTheta();
      break;
    case GLFW_KEY_SPACE:
      currentTransform = (currentTransform + 1) % 3;
      break;
    case GLFW_KEY_ESCAPE:
      // Esc按键
      exit(EXIT_SUCCESS);
      break;
  }
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
//  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
//    glfwSetWindowShouldClose(window, true);
//  } else if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
//    paused = !paused;
//  }
}

// Is called whenever a key is pressed/released via GLFW
void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
  std::cout << key << std::endl;
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GL_TRUE);
  } else if (action == GLFW_PRESS) {
    keyboard(key);
  }
}

// Resize callback
void framebuffer_size_callback(GLFWwindow *window, INT32 width, INT32 height) {
  glViewport(0, 0, width, height);
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
  GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "2017152003_3D_OFF", NULL,
                                        NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);

  // Ensure we can capture the escape key being pressed below
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
  // Hide the mouse and enable unlimited mouvement
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

  // Set the mouse at the center of the screen
  glfwPollEvents();
  glfwSetCursorPos(window, WIDTH/2, HEIGHT/2);

  // Enable depth test
  // glEnable(GL_DEPTH_TEST);

  // Accept fragment if it closer to the camera than the former one
  // glDepthFunc(GL_LESS);

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

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  // Game loop
  while (!glfwWindowShouldClose(window)) {

    // process input info
    processInput(window);

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
