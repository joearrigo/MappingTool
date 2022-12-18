/// 
/// TODO:	| |  1) Render grid of ghost spheres every 1x1 tile unit (size TBD); as in, render white textures with alpha < 1 on spheres
///			|X|  2) Allow movement throughout grid using wasd and mouse_look (toggled on and off using 'z' key)
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
#include <algorithm>

// Include GLEW
#include <GL/glew.h> 

// Include GLFW
#include <GLFW/glfw3.h>

//Others
#include <map>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//Shaders
#include "headers/shader.hpp"

//Other header includes
#include "headers/dstream.hpp"
#include "headers/model.h"

#include <filesystem>
namespace fs = std::filesystem;
#pragma endregion

#define CTRL_EXIT		002
#define CTRL_FVIEW		003
#define CTRL_FWD		004
#define CTRL_BACK		005
#define CTRL_LEFT		006
#define CTRL_RIGHT 		007
#define DEBUG_POSITION	010

GLFWwindow* window;
char Window_Title[32] = "Untitled Mapping Tool V0.0.2";
int XY_Resolution[2] = { 1024, 768 };
float Aspect_Ratio = XY_Resolution[0] / XY_Resolution[1];
GLuint programID, modelMatID, projCamMatID;

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
	{keyType(GLFW_KEY_D, GLFW_PRESS), CTRL_RIGHT},
	{keyType(GLFW_KEY_P, GLFW_PRESS), DEBUG_POSITION}
};

//Booleans
int freeView = 0, limitFPS = 1;

//Freeview stuff
// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
glm::mat4 Camera = glm::lookAt(
	glm::vec3(4, 3, 3),
	glm::vec3(0, 0, 0),
	glm::vec3(0, 1, 0)
);
glm::mat4 Model = glm::mat4(1.0f); //eye(4)
glm::mat4 VP;

glm::vec3 position = glm::vec3(4, 3, 3); //Initial position, worldspace. TODO: Make 0,0,0
glm::vec3 direction, right, up;

double horizontalAngle = 3.14f;
double verticalAngle = 0.0f;
float initialFOV = 45.0f;

float moveSpeed = 5.0f;
float mouseSpeed = 0.1f;

double lastTime = 0.0;
float deltaTime = 0.0f;


float secondCt = 0.0f;
int frames;
int msPerFrameAvg;

double toggleTime = 0.0;

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
	
	programID = LoadShaders("src/shaders/vert.glsl", "src/shaders/frag.glsl");
	std::cout << "loadshaders   " << glGetError() << std::endl; // returns 0 (no error)
	modelMatID = glGetUniformLocation(programID, "M");
	projCamMatID = glGetUniformLocation(programID, "VP");
	lout << Window_Title << " Loaded. GLFW, GLEW initialized.\n";
	dout << "Debug Mode Enabled\n";
	updConsole();
	return 1;
}

static void toggleFreeView() {
	if (glfwGetTime() < 1 + toggleTime)
		return;
	freeView = (freeView == 0); //flip value of freeView
	toggleTime = glfwGetTime();
	dout << freeView << "\n";

	if(freeView)
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	else
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

static void moveCamera(int directionIn) {
	if (!freeView)
		return;
	switch (directionIn) {
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
	horizontalAngle += mouseSpeed * deltaTime * (double(XY_Resolution[0]) / 2 - xpos);
	verticalAngle += mouseSpeed * deltaTime * (double(XY_Resolution[1])/2 - ypos);

	verticalAngle = std::clamp(verticalAngle, (double) - 3.14f / 2.0f, (double) 3.14f / 2.0f);
	
	if (verticalAngle > 3.14f / 2.0f)
		verticalAngle = 3.14f / 2.0f;
	if (verticalAngle < -3.14f / 2.0f)
		verticalAngle = -3.14f / 2.0f;

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
		case CTRL_EXIT:
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			break;
		case CTRL_FVIEW:
			toggleFreeView();
			break;
		case DEBUG_POSITION:
			dout << "X: " << position[0] << " Y: " << position[1] << " Z: " << position[2] << "\n";
			break;
		default:
			break;
	}
}

int main(void) {
	if (initialize() == -1) {
		return -1;
	}

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	//Make our model:
	jModel *mModel = new jModel("D:/SOFTWARE DEV/MappingTool/MappingTool/src/assets/box.obj");

	glfwSetKeyCallback(window, key_callback);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		///
		/// Calculating length of frame for movement math.
		/// 
		if (secondCt >= 1.0f) {
			msPerFrameAvg = (int)((1000 * secondCt) / frames);
			dout << "FPS: " << frames << " | mspf: " << msPerFrameAvg << "\n";
			frames = 0;
			secondCt = 0.0f;
		}

		deltaTime = float(glfwGetTime() - lastTime);
		lastTime = glfwGetTime();

		/// 
		/// Move/Look Calculations:
		/// 
		glfwGetCursorPos(window, &xpos, &ypos);

		if (freeView)
			lookCamera();

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			moveCamera(CTRL_FWD);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			moveCamera(CTRL_LEFT);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			moveCamera(CTRL_BACK);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			moveCamera(CTRL_RIGHT);


		/// 
		/// Matrix calculation before sending to GPU
		///
		VP = Projection * Camera;

		glm::vec4 worldPos = Model * glm::vec4(mModel->meshes[0].vertices[0].position, 1);
		glm::vec4 viewPos = VP * worldPos;
		glUseProgram(programID);
		glUniformMatrix4fv(modelMatID, 1, GL_FALSE, &Model[0][0]);
		glUniformMatrix4fv(projCamMatID, 1, GL_FALSE, &VP[0][0]);
		
		///
		/// Render whatever has to be rendered here:::
		/// 

		mModel->render(programID);

		/// 
		/// End of Frame's Work
		/// 

		glfwSwapBuffers(window);
		glfwPollEvents();
		updConsole();

		///
		/// Framerate limiting (MAX: 60fps)
		///
		while(limitFPS == 1 && float(glfwGetTime() - lastTime) < 1.0f / 60.0f);
		frames++;
		secondCt += glfwGetTime() - lastTime;
	}

	// Cleanup VBO
	glDeleteProgram(programID);

	glfwDestroyWindow(window);
	glfwTerminate();
	
	return 0;
}

