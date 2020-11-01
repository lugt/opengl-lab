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

#define INLINE inline

// 三角面片中的顶点序列
typedef struct vIndex {
	unsigned int a, b, c;
	vIndex(int ia, int ib, int ic) : a(ia), b(ib), c(ic) {}
} vec3i;

std::string filename;
std::vector<vec3> vertices;
std::vector<vec3i> faces;

int nVertices = 0;
int nFaces = 0;
int nEdges = 0;


// Window dimensions
const GLuint WIDTH = 1024, HEIGHT = 1024;

// Default vertex array object
static UINT32 VAO;

// Default shader program, merely does nothing.
static INT32 shaderProgram;


std::vector<vec3> points;   //传入着色器的绘制点
std::vector<vec3> colors;   //传入着色器的颜色

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


struct INT3LIST {
  union {
    struct {
      UINT32 x;
      UINT32 y;
      UINT32 z;
    };
    UINT32 _val[3];
  };
};

typedef INT3LIST face3;

class OFFFile {
  vector<vec3>     _vertex;
  vector<face3>    _faces;
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

  vector<face3> &getFaces() { return _faces; }

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
};

template<typename T, typename LIST>
void Read_list_from_istream(UINT32 size,
                            std::ifstream &input,
                            vector<LIST> &output) {
  output.resize(size);
  for (UINT32 i = 0; i < size; ++i) {
    T x, y, z;
    input >> x >> y >> z;
    LIST& cur_vertex = output.at(i);
    cur_vertex.x = x;
    cur_vertex.y = y;
    cur_vertex.z = z;
  }
}


void read_off(const std::string filename, OFFFile &off)
{
	if (filename.empty()) {
		return;
	}

	UINT32 n_vertex = 0, n_face = 0, n_lines = 0;
	std::ifstream fin;
	std::string first_line;
	fin.open(filename);

	// @TODO: 修改此函数读取OFF文件中三维模型的信息
	std::getline(fin, first_line);
	fin >> n_vertex >> n_face >> n_lines;
	off.setNVertex(n_vertex);
	off.setNFaces(n_face);
	off.setNLines(n_lines);
  off.getFaces().resize(n_face);
  Read_list_from_istream<FLOAT64, vec3>(n_vertex, fin, off.getVertex());
  Read_list_from_istream<UINT32, face3>(n_face, fin, off.getFaces());
  fin.close();
}


void storeFacesPoints()
{
	points.clear();
	colors.clear();

	// @TODO: 修改此函数在points和colors容器中存储每个三角面片的各个点和颜色信息
}

void init()
{
	storeFacesPoints();

	// 创建顶点数组对象
	GLuint vao[1];
	glGenVertexArrays(1, vao);
	glBindVertexArray(vao[0]);

	// 创建并初始化顶点缓存对象
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(vec3) + colors.size() * sizeof(vec3), NULL, GL_STATIC_DRAW);

	// @TODO: 修改完成后再打开下面注释，否则程序会报错
	// 分别读取数据
	//glBufferSubData(GL_ARRAY_BUFFER, 0, points.size() * sizeof(vec3), &points[0]);
	//glBufferSubData(GL_ARRAY_BUFFER, points.size() * sizeof(vec3), colors.size() * sizeof(vec3), &colors[0]);

	// 读取着色器并使用
	GLuint program = InitShader("/Users/xc5/CLionProjects/opengl/example2/src/lab2/vshader.glsl",
                             "/Users/xc5/CLionProjects/opengl/example2/src/lab2/fshader.glsl");
	glUseProgram(program);

	// 从顶点着色器中初始化顶点的位置
	GLuint pLocation = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(pLocation);
	glVertexAttribPointer(pLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	// 从片元着色器中初始化顶点的颜色
	GLuint cLocation = glGetAttribLocation(program, "vColor");
	glEnableVertexAttribArray(cLocation);
	glVertexAttribPointer(cLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(points.size() * sizeof(vec3)));

	// 黑色背景
	glClearColor(0.0, 0.0, 0.0, 1.0);
}


//int main(int argc, char **argv)
//{
//	glutInit(&argc, argv);
//	// @TODO: 窗口显示模式支持深度测试
//	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
//	glutInitWindowSize(600, 600);
//	glutCreateWindow("3D OFF Model");
//
//	glewExperimental = GL_TRUE;
//	glewInit();
//
//	init();
//	glutDisplayFunc(display);
//
//	// @TODO: 启用深度测试
//
//	glutMainLoop();
//	return 0;
//}


INLINE void refreshFrame(GLFWwindow *window, INT32 shaderProgram, INT32 VAO) {
  // 清理窗口
  glClearColor(BG.x, BG.y, BG.z, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(BG.x, BG.y, BG.z, 1.0);

  // @TODO: 清理窗口，包括颜色缓存和深度缓存
  glClear(GL_COLOR_BUFFER_BIT);

  // 绘制边
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  // 消除背面光照
  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);

  glDrawArrays(GL_TRIANGLES, 0, points.size());
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

// Is called whenever a key is pressed/released via GLFW
void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
  std::cout << key << std::endl;
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GL_TRUE);
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

  OFFFile off_core;

  // 读取off模型文件
  read_off("cube.off", off_core);

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
