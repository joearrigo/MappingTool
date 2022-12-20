#ifndef JGL_H
#define JGL_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "headers/shader.hpp"
#include <stdio.h>
#include <iostream>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <algorithm>
#include "jbufferqueue.h"

//User defined. Runs before loop, at startup.
void Initialize();	
//User defined. Runs during loop
void Loop();
//User defined. Runs after loop, at shutdown.
void Deactivate();
//User defined. Runs whenever a key is pressed/repeated/released.
void KeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods);

//Functions related to jgl operation.
void WorldRenderPoll();
void windowSizeCallback(GLFWwindow* window, int width, int height);
void glRender();
bool glInit();
bool glLoop();
void glDeactivate();

/// 
/// All variables used are stored in structs so that any user
/// does not accidentally name their unrelated variable the 
/// same as one of these and cause an error.
/// 
/// Any user should be primarily concerned only with userVars
/// and camera. Modifying position,direction modifies the transform.
/// userVars contains copies of variables rather than exposing
/// them directly which could cause error if meddled with.
///
struct jglUserVars {
	char Window_Title[32];
	int limitFPS = 1;
	bool shouldClose = 0;
	jglUserVars(std::string title) {
		strcpy_s(Window_Title, 32, title.c_str());
	}
};

struct jglVariables {
	GLFWwindow* window = NULL;
	int XY_Resolution[2] = { 1024, 768 };
	GLuint programID, modelMatID, projCamMatID;
	glm::vec4 bgColor = glm::vec4(0, 0, 0.4, 0); //dark blue
	float lastTime = 0.0f, deltaTime = 0.0f, secondCt = 0.0f;
	int frames, msPerFrameAvg;
	float getAspectRatio();
};

class jglCamera {
	private: 
		glm::vec3 positionOld = glm::vec3(0, 0, 0);
		double horizOld = 0.0f, vertOld = 0.0f;
	public:
		float fov; //measured in degrees
		glm::vec3 position = glm::vec3(0,0,0);
		glm::mat4 View = glm::mat4(0.0f);
		glm::mat4 Model = glm::mat4(0.0f);
		glm::mat4 Projection = glm::mat4(0.0f);
		glm::mat4 VP = glm::mat4(0.0f);
		glm::vec3 direction, right, up;
		double horizAng = 3.14f;
		double vertAng = 0.0f;
		jglCamera(float fov, glm::vec3 posM, glm::vec3 lookAt);
		void updateView();
};

extern jglVariables* glWindow;
extern jglCamera camera;
extern jglUserVars* userVars;
extern float deltaTime;

void printVector(float* elementZero, int size);
void printMatrix(float* elementZero, int size);
void printMatrix(float* elementZero, int sizeRows, int sizeCols);
#endif

///
/// So what a user would have, ideally, is something like:
/// int main(void) {
///		if(!glInit())
///			return -1;
///		
///		while(glLoop());
///		glDeactivate();
/// }
/// 
/// void Init(){
///		//User defined stuff here
/// }
/// 
/// void Loop(){
///		//User defined stuff here
/// }
/// 
/// ... and then of course keyevent, deactivate, etc.