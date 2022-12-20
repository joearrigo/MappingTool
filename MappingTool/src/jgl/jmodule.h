#ifndef JMODULE_H
#define JMODULE_H

#include <string>
#include <vector>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include "jbufferqueue.h"

const std::string MOD_MODEL		= "mod_model"		;
const std::string MOD_MATERIAL	= "mod_material"	;

//Declaring these classes and structs so they can be used regardless of definition order:
class WorldObject;
class Module;
class Model;
class Brush;
class Material;
struct Mesh;
struct Vertex;
struct Texture;

// Classes first: 

/// <summary>
/// WorldObject(). Contains modules. Think of it as a Unity 
/// GameObject. Also contains the worldspace transform of
/// the object. Defaults to (0,0,0,0)
/// </summary>
class WorldObject {
	private:
		std::vector<Module*> modules;
	public:
		glm::mat4 worldMatrix;
		bool insertModule(Module* mIn);
		bool removeModule(Module* mIn);
		Module* findModule(std::string type);

		WorldObject();
};

/// <summary>
/// A Module is a component of a WorldObject that performs
/// a specific function. Examples include Model, Material.
/// Model associates a 3D model with the GameObject, and
/// Material associates a texture and allows the 
/// GameObject to render graphically.
/// </summary>
class Module {
protected:
	std::string moduleType = "";
	WorldObject* parent;
public:
	Module() {}
	void setParent(WorldObject* parentIn) { parent = parentIn; }
	std::string getType() { return moduleType; }
	
	//For polymorphism:
	virtual void reset() = 0;
	virtual bool loadModel(std::string) = 0;				//Model
	virtual std::string getFilepath() = 0;					//Model
	virtual std::vector<GLuint> getVertexBuffers() = 0;		//Model
	virtual std::vector<GLuint> getIndexBuffers() = 0;		//Model
	virtual std::vector<GLuint> getMaterialIndices() = 0;	//Model
	virtual std::vector<GLuint> getIndexCounts() = 0;		//Model
	virtual const aiScene* getScene() = 0;					//Model
	virtual bool loadModel(GLint progID) = 0;				//Material
	virtual void render(GLint progID) = 0;					//Material
	virtual void bind(Texture* t, GLuint inp) = 0;			//Material
	virtual void unbind(GLuint inp) = 0;					//Material
	virtual void pushVAO() = 0;								//Material
};

/// <summary>
/// Model, derived from Module. This takes care of model 
/// file loading, mesh getting, UV generation, etc.
/// </summary>
class Model : public Module {
	private:
		std::vector<GLuint> vertexBuffers, indexBuffers, materialIndices, indexCts				;
		std::vector<Mesh> meshes;
		std::vector<Texture*> textures;
		Assimp::Importer importer;
		const aiScene* scene;
		std::string modelPath;

	public:
		Model();
		void reset();
		bool loadModel(std::string modelPathM);
		std::vector<GLuint> getVertexBuffers() { return vertexBuffers; }
		std::vector<GLuint> getIndexBuffers() { return indexBuffers; }
		std::vector<GLuint> getMaterialIndices() { return materialIndices; }
		std::vector<GLuint> getIndexCounts() { return indexCts; }
		std::string getFilepath() { return modelPath; }
		const aiScene* getScene() { return scene; }

		bool loadModel(GLint progID) { return 0; }
		void render(GLint progID) { }
		void bind(Texture* t, GLuint inp) { }
		void unbind(GLuint inp) { }
		void pushVAO() { }							
};

/// <summary>
/// Brush, derived from Module, an alternative to Model.
/// Like a BSP Brush. Defined by an vertex array and an
/// index array.
/// </summary>
class Brush : public Module {

};

/// <summary>
/// Material, derived from Module. This takes should be
/// the only module that ever interfaces directly with
/// OpenGL or any other sort of graphics displays.
/// </summary>
class Material : public  Module {
	private:
		std::vector<GLuint> vertexBuffers, indexBuffers, materialIndices, indexCounts;
		std::vector<Texture*> textures;
		std::string filepath = "";
		const aiScene* scene;
		void bind(Texture* t, GLuint inp);
		void unbind(GLuint inp);
		std::vector<BufferContainer*> bVec;
	public:
		Material();
		bool loadModel(GLint progID);
		void render(GLint progID); //returns number of vertices (indices) rendered.
		void pushVAO();

		void reset() { }
		bool loadModel(std::string strIn) { return 0; }
		std::string getFilepath() { return filepath; }
		std::vector<GLuint> getVertexBuffers() { 
			std::vector<GLuint> temp;
			return temp; 
		}
		std::vector<GLuint> getIndexBuffers() {
			std::vector<GLuint> temp;
			return temp;
		}
		std::vector<GLuint> getMaterialIndices() {
			std::vector<GLuint> temp;
			return temp;
		}
		std::vector<GLuint> getIndexCounts() {
			std::vector<GLuint> temp;
			return temp;
		}
		const aiScene* getScene() { return NULL; }
};

// Now onto the supplementaries:

class Mesh {
	private:
		aiMesh* mesh;
		GLuint vertexBuffer, indexBuffer, materialIndex;
		bool makeVertexBuffer();
		bool makeIndexBuffer();
		std::vector<Vertex> vertices;
		std::vector<unsigned short> indices;
	public:
		Mesh(aiMesh* meshM) {
			mesh = meshM;
			materialIndex = mesh->mMaterialIndex;
			makeVertexBuffer();
			makeIndexBuffer();
		}
		bool setMesh(aiMesh* meshM) {
			mesh = meshM;
			materialIndex = mesh->mMaterialIndex;
			bool ret = makeVertexBuffer();
			return ret * makeIndexBuffer();
		}
		int getIndexCt() { return indices.size(); }
		GLuint getVertexBuffer() { return vertexBuffer; }
		GLuint getIndexBuffer() { return indexBuffer; }
		GLuint getMaterialIndex() { return materialIndex; }
};

struct Vertex {
	glm::vec3 position, normal;
	glm::vec2 uv;
};

struct Texture {
	std::string filename;
	int width = 0, height = 0, channels = 0;
	unsigned int texture = 0;

	Texture(std::string filenameM);
	void getImageSize(int& widthM, int& heightM) {
		widthM = width; heightM = height;
	}
};

#endif
