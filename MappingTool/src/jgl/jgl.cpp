#include "jgl.h"

void printVector(float* elementZero, int size) {
	printMatrix(elementZero, 1, size);
}
void printMatrix(float* elementZero, int size) {
	printMatrix(elementZero, size, size);
}
void printMatrix(float* elementZero, int sizeRows, int sizeCols) {
	printf("\n");
	for (int i = 0; i < sizeRows; i++) {
		for (int j = 0; j < sizeCols; j++) {
			printf("%f\t", *elementZero);
			elementZero = elementZero + 1;
		}
		printf("\n");
	}
	printf("\n");
}

float jglVariables::getAspectRatio() {
	return XY_Resolution[0] / XY_Resolution[1];
}

jglCamera::jglCamera(float fov, glm::vec3 posM, glm::vec3 lookAt) {
	position = posM;
	View = glm::lookAt(position, lookAt, glm::vec3(0, 1, 0));
	Model = glm::mat4(1.0f); //eye(4)
	Projection = glm::perspective(glm::radians(fov), 4.0f / 3.0f, 0.1f, 100.0f);
	VP = View * Projection;
	updateView();
}

void jglCamera::updateView() {
	if (positionOld == position && horizOld == horizAng && vertOld == vertAng)
		return; //If no change, there's no point in recalculating

	//Prevent camera from flipping upside down in either direction.
	vertAng = std::clamp(vertAng, (double)-3.14f / 2.0f, (double)3.14f / 2.0f);

	direction = glm::vec3(
		cos(vertAng) * sin(horizAng),
		sin(vertAng),
		cos(vertAng) * cos(horizAng)
	);

	right = glm::vec3(
		sin(horizAng - 3.14f / 2.0f),
		0,
		cos(horizAng - 3.14f / 2.0f)
	);

	up = glm::cross(right, direction);

	View = glm::lookAt(
		position,
		position + direction,
		up
	);

	VP = Projection * View;

	positionOld = position;
	horizOld = horizAng;
	vertOld = vertAng;
}

void windowSizeCallback(GLFWwindow* window, int width, int height) {
	glWindow->XY_Resolution[0] = width;
	glWindow->XY_Resolution[0] = height;
	glViewport(0, 0, width, height);
	camera.Projection = glm::perspective(glm::radians(camera.fov), glWindow->getAspectRatio(), 0.1f, 100.0f);
}

// This macro will help us make the attribute pointers
// position, size, type, struct, element
// from: David Erbelding, Niko Procopi (see frag.shader).
#define SetupAttribute(index, size, type, structure, element) \
	glVertexAttribPointer(index, size, type, 0, sizeof(structure), (void*)offsetof(structure, element)); \

void glRender() {
	while (!glVAOs.empty()) {
		glBindVertexArray(glVAOs.front()->VAO);

		glDrawElements(GL_TRIANGLES, glVAOs.front()->numIndices, GL_UNSIGNED_SHORT, 0);
		//glDrawArrays(GL_TRIANGLES, 0, glVAOs.front()->numIndices);
		glBindVertexArray(0);
		glVAOs.pop();
	}
}

bool glInit() {
	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return 0;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	glWindow->window = glfwCreateWindow(glWindow->XY_Resolution[0], glWindow->XY_Resolution[1], userVars->Window_Title, NULL, NULL);
	if (glWindow->window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		getchar();
		glfwTerminate();
		return 0;
	}
	glfwMakeContextCurrent(glWindow->window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return 0;
	}

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glWindow->programID = LoadShaders("src/shaders/vert.glsl", "src/shaders/frag.glsl");
	std::cout << "loadshaders   " << glGetError() << std::endl; // returns 0 (no error)
	glWindow->modelMatID = glGetUniformLocation(glWindow->programID, "M");
	glWindow->projCamMatID = glGetUniformLocation(glWindow->programID, "VP");

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	glfwSetKeyCallback(glWindow->window, KeyEvent);
	glfwSetWindowSizeCallback(glWindow->window, windowSizeCallback);
	glClearColor(glWindow->bgColor[0], glWindow->bgColor[1], glWindow->bgColor[2], glWindow->bgColor[3]);

	std::cout << userVars->Window_Title << " Loaded. GLFW, GLEW initialized.\n";

	//TODO: Init all possible textures and assign them GLuint texture values from GL.
	//Put into a map with (string absoluteFilePath, GLuint texture)
	//Whenever a texture is needed, poll the map for its filepath and use that texture
	//This avoids loading the same texture multiple times.
	//Will need to adjust jmodule textures[] vectors to be strings instead of GLuints

	Initialize();

	return 1;
}

bool glLoop() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	///
	/// Calculating length of frame for movement math.
	/// 
	if (glWindow->secondCt >= 1.0f) {
		glWindow->msPerFrameAvg = (int)((1000 * glWindow->secondCt) / glWindow->frames);
		std::cout << "FPS: " << glWindow->frames << " | mspf: " << glWindow->msPerFrameAvg << "\n";
		glWindow->frames = 0;
		glWindow->secondCt = 0.0f;
	}

	glWindow->deltaTime = float(glfwGetTime() - glWindow->lastTime);
	deltaTime = glWindow->deltaTime;
	glWindow->lastTime = glfwGetTime();

	///
	/// User loop code happens here:
	/// 

	Loop();
	camera.updateView();

	/// 
	/// Matrix calculation before sending to GPU
	///
	
	glUseProgram(glWindow->programID);

	//I'm just commenting this here so some lone
	//reader might know my struggle.
	//I spent AN ENTIRE DAY trying to fix 
	//an access violation issue regarding
	//&camera.Model[0][0]. And you know what
	//fixed it? I had the std::string type inside
	//of Module default to NULL, which is an issue
	//of its own, but that manifested itself as
	//an access violation here?!?!?!? WHY?
	//Lesson be learned, fellas: what you think
	//is the issue and what you're told is the issue
	//might not actually be the issue.

	glUniformMatrix4fv(glWindow->modelMatID, 1, GL_FALSE, &camera.Model[0][0]);
	glUniformMatrix4fv(glWindow->projCamMatID, 1, GL_FALSE, &camera.VP[0][0]);

	///
	/// Render whatever has to be rendered here:::
	/// 

	//TODO: Poll through every WorldObject and find renderable ones, then call
	//render() on their material modules
	WorldRenderPoll(); //This should resolve that todo.
	glRender();

	/// 
	/// End of Frame's Work
	/// 

	glfwSwapBuffers(glWindow->window);
	glfwPollEvents();

	///
	/// Framerate limiting (MAX: 60fps)
	///
	while (userVars->limitFPS == 1 && float(glfwGetTime() - glWindow->lastTime) < 1.0f / 60.0f);
	glWindow->frames++;
	glWindow->secondCt += glfwGetTime() - glWindow->lastTime;

	return !(glfwWindowShouldClose(glWindow->window) || userVars->shouldClose);
}

void glDeactivate() {
	glDeleteProgram(glWindow->programID);

	glfwDestroyWindow(glWindow->window);
	glfwTerminate();

	Deactivate();
}