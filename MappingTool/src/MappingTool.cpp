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
#include <jgl/global.h>
#include <jgl/jbufferqueue.h>
#include <jgl/jgl.h>
#include <jgl/jmodule.h>
#include <map>


jglVariables* glWindow = new jglVariables();
jglCamera camera = jglCamera(45, glm::vec3(4, 3, 3), glm::vec3(0, 0, 0));
jglUserVars* userVars = new jglUserVars("Untitled Mapping Tool V0.0.4");
float deltaTime;

#pragma endregion

#define CTRL_EXIT		002
#define CTRL_FVIEW		003
#define CTRL_FWD		004
#define CTRL_BACK		005
#define CTRL_LEFT		006
#define CTRL_RIGHT 		007
#define DEBUG_POSITION	010

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
int freeView = 0;
float mouseSpeed = 0.1f, moveSpeed = 5.0f;
double xpos, ypos;

float toggleTime;
void toggleFreeView() {
	if (glfwGetTime() < 1 + toggleTime)
		return;
	freeView = (freeView == 0); //flip value of freeView
	toggleTime = glfwGetTime();
	
	if(freeView)
		glfwSetInputMode(glWindow->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	else
		glfwSetInputMode(glWindow->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void moveCamera(int directionIn) {
	if (!freeView)
		return;
	switch (directionIn) {
		case CTRL_FWD:
			camera.position += camera.direction * deltaTime * moveSpeed;
			break;
		case CTRL_BACK:
			camera.position -= camera.direction * deltaTime * moveSpeed;
			break;
		case CTRL_LEFT:
			camera.position -= camera.right * deltaTime * moveSpeed;
			break;
		case CTRL_RIGHT:
			camera.position += camera.right * deltaTime * moveSpeed;
			break;
		default:
			break;
	}
}

void lookCamera() {
	if (!freeView)
		return;
	glfwSetCursorPos(glWindow->window, ((double)glWindow->XY_Resolution[0]) / 2, ((double)glWindow->XY_Resolution[1]) / 2);
	camera.horizAng += mouseSpeed * deltaTime * (double(glWindow->XY_Resolution[0]) / 2 - xpos);
	camera.vertAng += mouseSpeed * deltaTime * (double(glWindow->XY_Resolution[1])/2 - ypos);
	camera.updateView();
}

//Check which key is keyed and what action is actioned and respond accordingly.
void KeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods)
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
			std::cout << "X: " << camera.position[0] << " Y: " << camera.position[1] << " Z: " << camera.position[2] << "\n";
			break;
		default:
			break;
	}
}
/*
	virtual void reset() = 0;
	virtual bool loadModel(std::string) = 0;				//Model
	virtual std::string getFilepath() = 0;					//Model
	virtual std::vector<GLuint> getVertexBuffers() = 0;		//Model
	virtual std::vector<GLuint> getIndexBuffers() = 0;		//Model
	virtual std::vector<GLuint> getMaterialIndices() = 0;	//Model
	virtual std::vector<GLuint> getIndexCounts() = 0;		//Model
	virtual const aiScene* getScene() = 0;					//Model
	virtual bool loadModel() = 0;							//Material
	virtual void render(GLint progID) = 0;					//Material
	virtual void bind(Texture* t, GLuint inp) = 0;			//Material
	virtual void unbind(GLuint inp) = 0;					//Material
*/
bool t = 1;
int main(void) {
	if (!glInit())
		return -1;

	std::cout << t << " glInited\n";

	while (glLoop());
	glDeactivate();
}

WorldObject cube;

void Initialize() {
	t = 0;

	Model* model = new Model();
	Material* mat = new Material();

	cube.insertModule(model);
	cube.insertModule(mat);
	std::cout << "Modules inserted\n";
	
	model->loadModel("D:/SOFTWARE DEV/MappingTool/MappingTool/src/assets/box.obj");
	std::cout << "Model loaded\n";
	mat->loadModel(glWindow->programID);
	std::cout << "Textures gotten\n";
}

void Loop() {
	glfwGetCursorPos(glWindow->window, &xpos, &ypos);

	if (freeView)
		lookCamera();

	if (glfwGetKey(glWindow->window, GLFW_KEY_W) == GLFW_PRESS)
		moveCamera(CTRL_FWD);
	if (glfwGetKey(glWindow->window, GLFW_KEY_A) == GLFW_PRESS)
		moveCamera(CTRL_LEFT);
	if (glfwGetKey(glWindow->window, GLFW_KEY_S) == GLFW_PRESS)
		moveCamera(CTRL_BACK);
	if (glfwGetKey(glWindow->window, GLFW_KEY_D) == GLFW_PRESS)
		moveCamera(CTRL_RIGHT);
}

void WorldRenderPoll() {

	cube.findModule(MOD_MATERIAL)->pushVAO();
}

void Deactivate() {

}