/*
 *        Computer Graphics Course - Shenzhen University
 *    Mid-term Assignment - Tetris implementation sample code
 * ============================================================
 *
 * - 本代码仅仅是参考代码，具体要求请参考作业说明，按照顺序逐步完成。
 * - 关于配置OpenGL开发环境、编译运行，请参考第一周实验课程相关文档。
 *
 * - 已实现功能如下：
 * - 1) 绘制棋盘格和‘L’型方块
 * - 2) 键盘左/右/下键控制方块的移动，上键旋转方块
 *
 * - 未实现功能如下：
 * - 1) 随机生成方块并赋上不同的颜色
 * - 2) 方块的自动向下移动
 * - 3) 方块之间的碰撞检测
 * - 4) 棋盘格中每一行填充满之后自动消除
 * - 5) 其他
 *
 */

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// Angel
#include "Angel.h"

// Std_image
#include "stb_image.h"

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

// CLI
#include "unistd.h"


#include <cstdlib>
#include <iostream>

#include <chrono>
#include <algorithm>

using namespace std;

using std::chrono::steady_clock;
typedef std::chrono::steady_clock::time_point TIME_PT;

TIME_PT starttime;			// 控制方块向下移动时间

int rotation = 0;		// 控制当前窗口中的方块旋转
vec2 tile[4];			// 表示当前窗口中的方块
bool gameover = false;	// 游戏结束控制变量
int xsize = 400;		// 窗口大小（尽量不要变动窗口大小！）
int ysize = 720;

INT64 game_score = 0;

// 一个二维数组表示所有可能出现的方块和方向。
vec2 allRotationsLshape[4][4] =
							  {{vec2(0, 0), vec2(-1,0), vec2(1, 0), vec2(-1,-1)},	//   "L"
							   {vec2(0, 1), vec2(0, 0), vec2(0,-1), vec2(1, -1)},   //
							   {vec2(1, 1), vec2(-1,0), vec2(0, 0), vec2(1,  0)},   //
							   {vec2(-1,1), vec2(0, 1), vec2(0, 0), vec2(0, -1)}};

// 绘制窗口的颜色变量
vec4 orange = vec4(1.0, 0.5, 0.0, 1.0);
vec4 blue   = vec4(0.3, 0.2, 1.0, 1.0);
vec4 white  = vec4(1.0, 1.0, 1.0, 1.0);
vec4 black  = vec4(0.0, 0.0, 0.0, 1.0);
vec4 ENABLED_COLOR = vec4(1.0, 0.5, 0.0, 1.0);

// 当前方块的位置（以棋盘格的左下角为原点的坐标系）
vec2 tilepos = vec2(5, 19);

const UINT32 BOARD_WIDTH = 10, BOARD_HEIGHT = 20;

// 布尔数组表示棋盘格的某位置是否被方块填充，即board[x][y] = true表示(x,y)处格子被填充。
// （以棋盘格的左下角为原点的坐标系）
bool board[BOARD_WIDTH][BOARD_HEIGHT];


// 当棋盘格某些位置被方块填充之后，记录这些位置上被填充的颜色
vec4 boardcolours[1200];

GLuint locxsize;
GLuint locysize;

GLuint vaoIDs[3];
GLuint vboIDs[6];

//////////////////////////////////////////////////////////////////////////
// 修改棋盘格在pos位置的颜色为colour，并且更新对应的VBO

void changecellcolour(vec2 pos, vec4 colour)
{
	// 每个格子是个正方形，包含两个三角形，总共6个定点，并在特定的位置赋上适当的颜色
	for (int i = 0; i < 6; i++)
		boardcolours[(int)(6*(10*pos.y + pos.x) + i)] = colour;

	vec4 newcolours[6] = {colour, colour, colour, colour, colour, colour};

	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);

	// 计算偏移量，在适当的位置赋上颜色
	int offset = 6 * sizeof(vec4) * (int)(10*pos.y + pos.x);
	glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(newcolours), newcolours);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//////////////////////////////////////////////////////////////////////////
// 当前方块移动或者旋转时，更新VBO

void updatetile()
{
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[4]);

	// 每个方块包含四个格子
	for (int i = 0; i < 4; i++)
	{
		// 计算格子的坐标值
		GLfloat x = tilepos.x + tile[i].x;
		GLfloat y = tilepos.y + tile[i].y;

		const FLOAT32 HSIZ = 33.0, FSIZ = 66.0;

		vec4 p1 = vec4(HSIZ + (x * HSIZ), HSIZ + (y * HSIZ), .4, 1);
		vec4 p2 = vec4(HSIZ + (x * HSIZ), FSIZ + (y * HSIZ), .4, 1);
		vec4 p3 = vec4(FSIZ + (x * HSIZ), HSIZ + (y * HSIZ), .4, 1);
		vec4 p4 = vec4(FSIZ + (x * HSIZ), FSIZ + (y * HSIZ), .4, 1);

		// 每个格子包含两个三角形，所以有6个顶点坐标
		vec4 newpoints[6] = {p1, p2, p3, p2, p3, p4};
		glBufferSubData(GL_ARRAY_BUFFER, i*6*sizeof(vec4), 6*sizeof(vec4), newpoints);
	}
	glBindVertexArray(0);
}

//////////////////////////////////////////////////////////////////////////
// 设置当前方块为下一个即将出现的方块。在游戏开始的时候调用来创建一个初始的方块，
// 在游戏结束的时候判断，没有足够的空间来生成新的方块。

void newtile()
{
	// 将新方块放于棋盘格的最上行中间位置并设置默认的旋转方向
	tilepos = vec2(5 , 19);
	rotation = 0;

	for (int i = 0; i < 4; i++)
	{
		tile[i] = allRotationsLshape[0][i];
	}

	updatetile();

	// 给新方块赋上颜色
	vec4 newcolours[24];
	for (int i = 0; i < 24; i++) {
    newcolours[i] = blue;
  }

	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(newcolours), newcolours);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

//////////////////////////////////////////////////////////////////////////
// 游戏和OpenGL初始化

void init()
{
	// 初始化棋盘格，包含64个顶点坐标（总共32条线），并且每个顶点一个颜色值
	vec4 gridpoints[64];
	vec4 gridcolours[64];

	// 纵向线
	for (int i = 0; i < 11; i++)
	{
		gridpoints[2*i] = vec4((33.0 + (33.0 * i)), 33.0, 0, 1);
		gridpoints[2*i + 1] = vec4((33.0 + (33.0 * i)), 693.0, 0, 1);

	}

	// 水平线
	for (int i = 0; i < 21; i++)
	{
		gridpoints[22 + 2*i] = vec4(33.0, (33.0 + (33.0 * i)), 0, 1);
		gridpoints[22 + 2*i + 1] = vec4(363.0, (33.0 + (33.0 * i)), 0, 1);
	}

	// 将所有线赋成白色
	for (int i = 0; i < 64; i++)
		gridcolours[i] = white;

	// 初始化棋盘格，并将没有被填充的格子设置成黑色
	UINT32 pts = BOARD_WIDTH * BOARD_HEIGHT * 6;

	vec4 boardpoints[pts];
	for (int i = 0; i < pts; i++)
		boardcolours[i] = black;

	// 对每个格子，初始化6个顶点，表示两个三角形，绘制一个正方形格子
	for (int i = 0; i < BOARD_HEIGHT; i++)
		for (int j = 0; j < BOARD_WIDTH; j++)
		{
			vec4 p1 = vec4(33.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p2 = vec4(33.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);
			vec4 p3 = vec4(66.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p4 = vec4(66.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);

			boardpoints[6*(BOARD_WIDTH*i + j)    ] = p1;
			boardpoints[6*(BOARD_WIDTH*i + j) + 1] = p2;
			boardpoints[6*(BOARD_WIDTH*i + j) + 2] = p3;
			boardpoints[6*(BOARD_WIDTH*i + j) + 3] = p2;
			boardpoints[6*(BOARD_WIDTH*i + j) + 4] = p3;
			boardpoints[6*(BOARD_WIDTH*i + j) + 5] = p4;
		}

	// 将棋盘格所有位置的填充与否都设置为false（没有被填充）
	for (int i = 0; i < BOARD_WIDTH; i++) {
    for (int j = 0; j < BOARD_HEIGHT; j++) {
      board[i][j] = false;
    }
  }

	// 载入着色器
	GLuint program = InitShader("/Users/xc5/CLionProjects/opengl/example2/src/tetris/vshader.glsl",
                             "/Users/xc5/CLionProjects/opengl/example2/src/tetris/fshader.glsl");
	glUseProgram(program);

	locxsize = glGetUniformLocation(program, "xsize");
	locysize = glGetUniformLocation(program, "ysize");

	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	GLuint vColor = glGetAttribLocation(program, "vColor");

	glGenVertexArrays(3, &vaoIDs[0]);

	// 棋盘格顶点
	glBindVertexArray(vaoIDs[0]);
	glGenBuffers(2, vboIDs);

	// 棋盘格顶点位置
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[0]);
	glBufferData(GL_ARRAY_BUFFER, 64*sizeof(vec4), gridpoints, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// 棋盘格顶点颜色
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[1]);
	glBufferData(GL_ARRAY_BUFFER, 64*sizeof(vec4), gridcolours, GL_STATIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);

	// 棋盘格每个格子
	glBindVertexArray(vaoIDs[1]);
	glGenBuffers(2, &vboIDs[2]);

	// 棋盘格每个格子顶点位置
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[2]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardpoints, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// 棋盘格每个格子顶点颜色
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardcolours, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);

	// 当前方块
	glBindVertexArray(vaoIDs[2]);
	glGenBuffers(2, &vboIDs[4]);

	// 当前方块顶点位置
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[4]);
	glBufferData(GL_ARRAY_BUFFER, 24*sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// 当前方块顶点颜色
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]);
	glBufferData(GL_ARRAY_BUFFER, 24*sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);

	glBindVertexArray(0);
	glClearColor(0, 0, 0, 0);

	// 游戏初始化
	newtile();
	starttime = std::chrono::steady_clock::now();
}

//////////////////////////////////////////////////////////////////////////
// 检查在cellpos位置的格子是否被填充或者是否在棋盘格的边界范围内。

bool checkvalid(vec2 cellpos)
{
	if((cellpos.x >=0) && (cellpos.x < BOARD_WIDTH) && (cellpos.y >= 0) && (cellpos.y < BOARD_HEIGHT) ) {
    if (board[(INT32)(cellpos.x)][(INT32)(cellpos.y)]) {
      return false;
    }
	  return true;
  } else
		return false;
}

//////////////////////////////////////////////////////////////////////////
// 在棋盘上有足够空间的情况下旋转当前方块

void rotate()
{
	// 计算得到下一个旋转方向
	int nextrotation = (rotation + 1) % 4;

	// 检查当前旋转之后的位置的有效性
	if (checkvalid((allRotationsLshape[nextrotation][0]) + tilepos)
		&& checkvalid((allRotationsLshape[nextrotation][1]) + tilepos)
		&& checkvalid((allRotationsLshape[nextrotation][2]) + tilepos)
		&& checkvalid((allRotationsLshape[nextrotation][3]) + tilepos))
	{
		// 更新旋转，将当前方块设置为旋转之后的方块
		rotation = nextrotation;
		for (int i = 0; i < 4; i++)
			tile[i] = allRotationsLshape[rotation][i];

		updatetile();
	}
}

//////////////////////////////////////////////////////////////////////////
// 检查棋盘格在row行有没有被填充满

BOOL checkfullrow(int row_id)
{
  UINT32 row_count = 0;
  for(UINT32 x = 0; x < BOARD_WIDTH; x++) {
    row_count += (board[x][row_id]) ? 1 : 0;
  }

  // full row found.
  if (row_count == BOARD_WIDTH) {
    // cancel this row and move everything above down for 1
    // X X O O
    // O O X O, Look above and check if need to change
    for (INT32 cur_row = row_id; cur_row < BOARD_HEIGHT - 1; cur_row++) {
      Is_True(cur_row < BOARD_HEIGHT - 1, ("Incorrect cur_row : %d", cur_row));
      for (UINT32 x = 0; x < BOARD_WIDTH; x++) {
        if (board[x][cur_row] != board[x][cur_row + 1]) {
          board[x][cur_row] = board[x][cur_row + 1];
          changecellcolour(vec2(x, cur_row), board[x][cur_row] ? ENABLED_COLOR : black);
        }
      }
    }
    // Remove first row.
    for (UINT32 x = 0; x < BOARD_WIDTH; x++) {
      if (board[x][BOARD_HEIGHT - 1]) {
        board[x][BOARD_HEIGHT - 1] = false;
        changecellcolour(vec2(x, BOARD_HEIGHT - 1), black);
      }
    }
    return true;
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////
// 放置当前方块，并且更新棋盘格对应位置顶点的颜色VBO

void settile()
{
	// 每个格子
	for (int i = 0; i < 4; i++)
	{
		// 获取格子在棋盘格上的坐标
		int x = (tile[i] + tilepos).x;
		int y = (tile[i] + tilepos).y;
		// 将格子对应在棋盘格上的位置设置为填充
		board[x][y] = true;
		// 并将相应位置的颜色修改
		changecellcolour(vec2(x,y), ENABLED_COLOR);
	}
}

//////////////////////////////////////////////////////////////////////////
// 给定位置(x,y)，移动方块。有效的移动值为(-1,0)，(1,0)，(0,-1)，分别对应于向
// 左，向下和向右移动。如果移动成功，返回值为true，反之为false。

bool movetile(vec2 direction)
{
  // 4 tiles in a graphic element.
  INT32 current_tile_size_count = 4;

	// 计算移动之后的方块的位置坐标
	vec2 newtilepos[current_tile_size_count];
	for (int i = 0; i < current_tile_size_count; i++)
		newtilepos[i] = tile[i] + tilepos + direction;

	BOOL stablized = false;
	while (!stablized) {
	  stablized = true;
    for (int i = 0; i < BOARD_HEIGHT; i++) {
      if (checkfullrow(i)) {
        stablized = false;
      }
    }
  }

	// 检查移动之后的有效性
	if (checkvalid(newtilepos[0])
		&& checkvalid(newtilepos[1])
		&& checkvalid(newtilepos[2])
		&& checkvalid(newtilepos[3]))
		{
			// 有效：移动该方块
			tilepos.x = tilepos.x + direction.x;
			tilepos.y = tilepos.y + direction.y;

			updatetile();

			return true;
		}

	return false;
}

//////////////////////////////////////////////////////////////////////////
// 重新启动游戏

void restart()
{
  // Clear all the known boxes and restart.
  for (INT32 i = 0; i < BOARD_WIDTH; ++i){
    for (INT32 j = 0; j < BOARD_HEIGHT; ++j) {
      board[i][j] = false;
      changecellcolour(vec2(i,j), black);
    }
  }
}

//////////////////////////////////////////////////////////////////////////
// 游戏渲染部分

void display()
{
	glClear(GL_COLOR_BUFFER_BIT);

	glUniform1i(locxsize, xsize);
	glUniform1i(locysize, ysize);

	glBindVertexArray(vaoIDs[1]);
	glDrawArrays(GL_TRIANGLES, 0, 1200); // 绘制棋盘格 (10*20*2 = 400 个三角形)

	glBindVertexArray(vaoIDs[2]);
	glDrawArrays(GL_TRIANGLES, 0, 24);	 // 绘制当前方块 (8 个三角形)

	glBindVertexArray(vaoIDs[0]);
	glDrawArrays(GL_LINES, 0, 64);		 // 绘制棋盘格的线

}

//////////////////////////////////////////////////////////////////////////
// 在窗口被拉伸的时候，控制棋盘格的大小，使之保持固定的比例。

void reshape(GLsizei w, GLsizei h)
{
	xsize = w;
	ysize = h;
	glViewport(0, 0, w, h);
}

void test_func_1() {
  for(UINT32 y = 3; y < 5; y++) {
    for(UINT32 x = 0; x < BOARD_WIDTH; x++) {
      board[x][y] = true;
      // 并将相应位置的颜色修改
      changecellcolour(vec2(x,y), ENABLED_COLOR);
    }
  }
}

//////////////////////////////////////////////////////////////////////////
// 键盘响应事件中的特殊按键响应

void special(UINT32 key, int x, int y)
{
	if(!gameover)
	{
		switch(key)
		{
			case GLFW_KEY_UP:	// 向上按键旋转方块
				rotate();
				break;
		  case GLFW_KEY_S: {
        Is_Trace(true, (stdout, "Your score : %lld \n", game_score));
        break;
		  }
		  case GLFW_KEY_T: {
        test_func_1();
		    break;
		  }
			case GLFW_KEY_DOWN: // 向下按键移动方块
        game_score ++;
				if (!movetile(vec2(0, -1)))
				{
					settile();
					newtile();
				}
				break;
			case GLFW_KEY_LEFT:  // 向左按键移动方块
				movetile(vec2(-1, 0));
				break;
			case GLFW_KEY_RIGHT: // 向右按键移动方块
				movetile(vec2(1, 0));
				break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// 键盘响应时间中的普通按键响应

void keyboard(UINT32 key)
{
	switch(key)
	{
		case GLFW_KEY_ESCAPE: // ESC键 和 'q' 键退出游戏
			exit(EXIT_SUCCESS);
			break;
		case GLFW_KEY_Q:
			exit (EXIT_SUCCESS);
			break;
		case GLFW_KEY_R: // 'r' 键重启游戏
			restart();
			break;
	  default:
      special(key, 0, 0);
      break;
	}
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

//////////////////////////////////////////////////////////////////////////

void idle(void)
{
	//
}

//////////////////////////////////////////////////////////////////////////

// Resize callback
void framebuffer_size_callback(GLFWwindow *window, INT32 width, INT32 height) {
  glViewport(0, 0, width, height);
  reshape(width, height);
}

// The MAIN function, from here we start the application and run the game loop
INT32 main(INT32 argc, char **argv) {

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
  GLFWwindow *window = glfwCreateWindow(xsize, ysize, "Mid-Term-2017152003", NULL,
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
  glfwSetCursorPos(window, xsize/2, ysize/2);

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
  glViewport(0, 0, xsize, ysize);

  const GLubyte *n1 = glGetString(GL_VERSION);
  std::cout << "openGL version string : " << n1 << std::endl;

  // Initialize shaders
  init();

//  glEnable(GL_DEPTH_TEST);
//  glDepthFunc(GL_LESS);

  // Game loop
  while (!glfwWindowShouldClose(window)) {

    display();

    // Check if any events have been activated (key pressed, mouse moved etc.) and call corresponding response functions
    glfwPollEvents();

    // Swap the screen buffers
    glfwSwapBuffers(window);
  }

  // Terminates GLFW, clearing any resources allocated by GLFW.
  glfwTerminate();
  return 0;
}

