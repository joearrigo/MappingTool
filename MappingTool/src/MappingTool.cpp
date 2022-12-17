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
#include <map>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

//Shaders
#include "headers/shader.hpp"

//Other header includes
#include "headers/dstream.hpp"

#pragma endregion

#define CTRL_EXIT		000
#define CTRL_FVIEW		001
#define CTRL_FWD		002
#define CTRL_BACK		003
#define CTRL_LEFT		004
#define CTRL_RIGHT 		005

GLFWwindow* window;
char Window_Title[32] = "Untitled Mapping Tool V0.0.1";
int XY_Resolution[2] = { 1024, 768 };
float Aspect_Ratio = XY_Resolution[0] / XY_Resolution[1];
GLuint programID, MatrixID;

//A keyType is basically just a vector that stores a keyID int and an actionID int.
struct keyType {
	int key, action; 
	keyType(int keyI, int actionI) { 
		key = keyI; 
		action = actionI; 
	}

	//Needed so that it can be input into the map (for sorting).
	bool operator<(const keyType& rhs) const noexcept
	{
		return (key < rhs.key);
	}
};

//This is the keyMap, the idea is that whenever a key is pressed (or any other action is done to it), a keyType made
//from that key and action combination is searched in the keyMap to figure out what action to do (e.g. pause menu, fire, etc.)
std::map<keyType, int> keyBindings = {
	{keyType(GLFW_KEY_ESCAPE, GLFW_PRESS), CTRL_EXIT},
	{keyType(GLFW_KEY_Z, GLFW_PRESS), CTRL_FVIEW},
	{keyType(GLFW_KEY_W, GLFW_PRESS), CTRL_FWD},
	{keyType(GLFW_KEY_S, GLFW_PRESS), CTRL_BACK},
	{keyType(GLFW_KEY_A, GLFW_PRESS), CTRL_LEFT},
	{keyType(GLFW_KEY_D, GLFW_PRESS), CTRL_RIGHT}
};

//Booleans
int freeView = 0;

//Freeview stuff
// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
glm::mat4 Camera = glm::lookAt(
	glm::vec3(4, 3, 3),
	glm::vec3(0, 0, 0),
	glm::vec3(0, 1, 0)
);
glm::mat4 Model = glm::mat4(1.0f); //eye(4)
glm::mat4 MVP;

glm::vec3 position = glm::vec3(4, 3, 3); //Initial position, worldspace. TODO: Make 0,0,0
glm::vec3 direction, right, up;

float horizontalAngle = 3.14f;
float verticalAngle = 0.0f;
float initialFOV = 45.0f;

float moveSpeed = 0.5f;
float mouseSpeed = 0.005f;

float lastTime = 0.0f;
float deltaTime = 0.0f;

float toggleTime = 0.0f;

double xpos, ypos;

//Set up GLFW (+ window) and GLEW (from: http://www.opengl-tutorial.org/)
int initialize() {
	debugStatus = 1;
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

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	programID = LoadShaders("src/shaders/vert.shader", "src/shaders/frag.shader");
	MatrixID = glGetUniformLocation(programID, "MVP");
	lout << Window_Title << " Loaded. GLFW, GLEW initialized.\n";
	dout << "Debug Mode Enabled\n";
}

static void toggleFreeView() {
	if (glfwGetTime() < 1 + toggleTime)
		return;
	freeView = (freeView == 0); //flip value of freeView
	toggleTime = glfwGetTime();
	dout << freeView << "\n";
}

static void moveCamera(int direction) {
	if (!freeView)
		return;
	switch (direction) {
		case CTRL_FWD:
			position += direction * deltaTime * moveSpeed;
			break;
		case CTRL_BACK:
			position -= direction * deltaTime * moveSpeed;
			break;
		case CTRL_LEFT:
			position -= right * deltaTime * moveSpeed;
			break;
		case CTRL_RIGHT:
			position += right * deltaTime * moveSpeed;
			break;
		default:
			break;
	}
}

static void lookCamera() {
	if (!freeView)
		return;
	glfwSetCursorPos(window, ((double)XY_Resolution[0]) / 2, ((double)XY_Resolution[1]) / 2);
	horizontalAngle += mouseSpeed * deltaTime * float(XY_Resolution[0] / 2 - xpos);
	horizontalAngle += mouseSpeed * deltaTime * float(XY_Resolution[1] / 2 - ypos);

	direction = glm::vec3(
		cos(verticalAngle) * sin(horizontalAngle),
		sin(verticalAngle),
		cos(verticalAngle) * cos(horizontalAngle)
	);

	right = glm::vec3(
		sin(horizontalAngle - 3.14f / 2.0f),
		0,
		cos(horizontalAngle - 3.14f / 2.0f)
	);

	glm::vec3 up = glm::cross(right, direction);

	Camera = glm::lookAt(
		position,
		position + direction,
		up
	);
}

//Check which key is keyed and what action is actioned and respond accordingly.
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	int command = keyBindings[keyType(key, action)];
	switch (command) {
		case(CTRL_EXIT):
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			break;
		case(CTRL_FVIEW):
			toggleFreeView();
			break;
		default:
			if (command == CTRL_FWD || command == CTRL_BACK || command == CTRL_LEFT || command == CTRL_RIGHT) {
				moveCamera(command);
				dout << "DELTATIME: " << deltaTime << "\n";
				dout << "POS: " << position[0] << " " << position[1] << " " << position[2] << "\n";
			}
			break;
	}
}

// Our vertices. Three consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
// A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
static const GLfloat g_vertex_buffer_data[] = {
	-1.0f,-1.0f,-1.0f, // triangle 1 : begin
	-1.0f,-1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f, // triangle 1 : end
	1.0f, 1.0f,-1.0f, // triangle 2 : begin
	-1.0f,-1.0f,-1.0f,
	-1.0f, 1.0f,-1.0f, // triangle 2 : end
	1.0f,-1.0f, 1.0f,
	-1.0f,-1.0f,-1.0f,
	1.0f,-1.0f,-1.0f,
	1.0f, 1.0f,-1.0f,
	1.0f,-1.0f,-1.0f,
	-1.0f,-1.0f,-1.0f,
	-1.0f,-1.0f,-1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f,-1.0f,
	1.0f,-1.0f, 1.0f,
	-1.0f,-1.0f, 1.0f,
	-1.0f,-1.0f,-1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f,-1.0f, 1.0f,
	1.0f,-1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f,-1.0f,-1.0f,
	1.0f, 1.0f,-1.0f,
	1.0f,-1.0f,-1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f,-1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f,-1.0f,
	-1.0f, 1.0f,-1.0f,
	1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f,-1.0f,
	-1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f,
	1.0f,-1.0f, 1.0f
};
static const GLfloat g_color_buffer_data[] = {
	0.583f,  0.771f,  0.014f,
	0.609f,  0.115f,  0.436f,
	0.327f,  0.483f,  0.844f,
	0.822f,  0.569f,  0.201f,
	0.435f,  0.602f,  0.223f,
	0.310f,  0.747f,  0.185f,
	0.597f,  0.770f,  0.761f,
	0.559f,  0.436f,  0.730f,
	0.359f,  0.583f,  0.152f,
	0.483f,  0.596f,  0.789f,
	0.559f,  0.861f,  0.639f,
	0.195f,  0.548f,  0.859f,
	0.014f,  0.184f,  0.576f,
	0.771f,  0.328f,  0.970f,
	0.406f,  0.615f,  0.116f,
	0.676f,  0.977f,  0.133f,
	0.971f,  0.572f,  0.833f,
	0.140f,  0.616f,  0.489f,
	0.997f,  0.513f,  0.064f,
	0.945f,  0.719f,  0.592f,
	0.543f,  0.021f,  0.978f,
	0.279f,  0.317f,  0.505f,
	0.167f,  0.620f,  0.077f,
	0.347f,  0.857f,  0.137f,
	0.055f,  0.953f,  0.042f,
	0.714f,  0.505f,  0.345f,
	0.783f,  0.290f,  0.734f,
	0.722f,  0.645f,  0.174f,
	0.302f,  0.455f,  0.848f,
	0.225f,  0.587f,  0.040f,
	0.517f,  0.713f,  0.338f,
	0.053f,  0.959f,  0.120f,
	0.393f,  0.621f,  0.362f,
	0.673f,  0.211f,  0.457f,
	0.820f,  0.883f,  0.371f,
	0.982f,  0.099f,  0.879f
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

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	GLuint colorbuffer;
	glGenBuffers(1, &colorbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		deltaTime = float(glfwGetTime() - lastTime);
		lastTime = deltaTime;

		glfwGetCursorPos(window, &xpos, &ypos);

		if (freeView)
			lookCamera();

		MVP = Projection * Camera * Model;

		glUseProgram(programID);
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

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

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glVertexAttribPointer(
			1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// Draw the triangle !
		glDrawArrays(GL_TRIANGLES, 0, 12*3); // Starting from vertex 0; 3 vertices total -> 1 triangle
		
		glDisableVertexAttribArray(0);

		glfwSwapBuffers(window);
		glfwPollEvents();
		updConsole();
	}

	// Cleanup VBO
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteVertexArrays(1, &VertexArrayID);
	glDeleteProgram(programID);

	glfwDestroyWindow(window);
	glfwTerminate();
	
	return 0;
}

