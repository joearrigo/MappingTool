/// 
/// TODO:	| |  1) Render grid of ghost spheres every 1x1 tile unit (size TBD); as in, render white textures with alpha < 1 on spheres
///			| |  2) Allow movement throughout grid using wasd and mouse_look (toggled on and off using 'z' key)
///			| |  3) Make listener for mouse click, check if it hits any rendered face (choose the frontmost), if so, return a tag identifying that object.
///			| |  4) Allow creation of planes by clicking on ghost spheres and dragging to make shape (kind of like Fusion 360)
///			| |  5) Make GUI to contain GLFW window and display available textures on the side (selectable using mouse click).
///			| |  6) Allow application of square tile textures to planes (as in, textures tile across rectangles)
///			| |  7) Make map exportable to OBJ (with texture UVs and the whole everything else too)
///			| |  8) Face subdivision and deformation (like Source Engine displacements)
///			| |  9) Optimize conversion of square tiles (2 1x1 right triangles): groups of square tiles can be made into rectangle tiles (with properly mapped textures) such that #rectangles is reasonably low
///			| | 10) Customizable skybox (dome)
///

#pragma region includes

// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

//Others
#include <iostream>
#include <sstream>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

//Shaders
#include "shaders/shader.hpp"

#pragma endregion

GLFWwindow* window;
char Window_Title[32] = "Untitled Mapping Tool V0.0.1";
int XY_Resolution[2] = { 1024, 768 };
float Aspect_Ratio = XY_Resolution[0] / XY_Resolution[1];

//Booleans
int freeView = 0, debugMode = 1;

std::stringstream cout;
void cPrint() {
	std::string consoleLog = cout.str();
	if (consoleLog.length() > 0) {
		std::cout << consoleLog;
		cout.str("");
	}
}

//Debug print, only print when debug mode is on
std::stringstream dout;
void dPrint() {
	std::string debugLog = dout.str();
	if (debugLog.length() > 0 && debugMode == 1) {
		std::cout << debugLog;
		dout.str("");
	}
}

//Set up GLFW (+ window) and GLEW (from: http://www.opengl-tutorial.org/)
int initialize() {
	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(XY_Resolution[0], XY_Resolution[1], Window_Title, NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}
	cout << Window_Title << " Loaded. GLFW, GLEW initialized.\n";
	dout << "Debug Mode Enabled\n";
}

static void toggleFreeView() {
	freeView = (freeView == 0); //flip value of freeView
	dout << freeView << "\n";
	//Do more
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
		toggleFreeView();
}

static const GLfloat g_vertex_buffer_data[] = {
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	0.0f, 1.0f, 0.0f
};

int main(void) {
	if (initialize() == -1) {
		return -1;
	}

	glfwSetKeyCallback(window, key_callback);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	GLuint programID = LoadShaders("src/shaders/vert.shader", "src/shaders/frag.shader");

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);
		// Draw the triangle !
		glDrawArrays(GL_TRIANGLES, 0, 3); // Starting from vertex 0; 3 vertices total -> 1 triangle
		glDisableVertexAttribArray(0);

		glUseProgram(programID);

		glfwSwapBuffers(window);
		glfwPollEvents();

		cPrint();
		dPrint();
	}

	// Cleanup VBO
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteVertexArrays(1, &VertexArrayID);
	glDeleteProgram(programID);

	glfwDestroyWindow(window);
	glfwTerminate();
	
	return 0;
}

