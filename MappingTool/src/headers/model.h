#ifndef MODEL_H
#define MODEL_H

#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <vector>
#include <headers/jTexture.h>

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;

	Vertex(glm::vec3 positionM, glm::vec2 texCoordM, glm::vec3 normalM) {
		position = positionM;
		normal = normalM;
		texCoord = texCoordM;
	}
	Vertex() {
		position = glm::vec3(0.0);
		normal = glm::vec3(0.0);
		texCoord = glm::vec2(0.0);
	}
};

class Mesh {
	private:
		aiMesh* mesh;
		void makeVBuffer();
		void makeIBuffer();

	public:
		std::vector<Vertex> vertices;
		std::vector<unsigned short> indices;
		GLuint vertexBuffer;
		GLuint indexBuffer;
		unsigned int materialIndex;
		Mesh (aiMesh* meshM) {
			setMesh(meshM);
		}

		void setMesh(aiMesh* meshM) {
			mesh = meshM;
			materialIndex = mesh->mMaterialIndex;
			makeVBuffer();
			makeIBuffer();
			printf("%d %d\n", (int)vertices.size(), (int)indices.size());
		}
};

class jModel {
	public:
		jModel(const std::string);
		bool loadModel();
		std::string getFilepath() {
			return filepath;
		}
		void setFilepath(std::string fIn) {
			filepath = fIn;
		}

		void render(GLint progID);
		std::vector<Mesh> meshes;

	private:
		std::string filepath, texture;
		Assimp::Importer importer;
		const aiScene *scene;
		std::vector<Texture*> textures;
		void genTextures();
};

#endif