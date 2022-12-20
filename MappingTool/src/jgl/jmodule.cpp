#include "jmodule.h"
#include <fstream>
#include <assimp/postprocess.h>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#pragma region WorldObject:

WorldObject::WorldObject() {
	worldMatrix = glm::mat4(0.0f);
}

bool WorldObject::insertModule(Module* in) {
	for (Module* temp : modules) {
		if (temp->getType() == in->getType())
			return 0; //Module of that type already here.
	}
	modules.push_back(in);
	in->setParent(this);
	return 1;
}

bool WorldObject::removeModule(Module* in) {
	for (int i = 0; i < modules.size(); i++) {
		Module* temp = modules[i];
		if (temp->getType() == in->getType()) {
			modules.erase(modules.begin()+i);
			return 1;
		}
	}
	return 0; //Module of that type not here.
}

Module* WorldObject::findModule(std::string type) {
	for (Module* temp : modules) {
		if (temp->getType() == type)
			return temp;
	}
	return NULL; //If none found
}
#pragma endregion

#pragma region Model:
bool Model::loadModel(std::string modelPathM) {
	modelPath = modelPathM;
	std::ifstream file(modelPath);
	if (!file.good())
		return 0; //File can't be found

	scene = importer.ReadFile(modelPath, aiProcess_Triangulate |
		aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
		aiProcess_JoinIdenticalVertices);

	if (scene) {
		for (unsigned int j = 0; j < scene->mNumMeshes; j++) {
			Mesh* temp = new Mesh(scene->mMeshes[j]);
			vertexBuffers.push_back(temp->getVertexBuffer());
			indexBuffers.push_back(temp->getIndexBuffer());
			materialIndices.push_back(temp->getMaterialIndex());
			indexCts.push_back(temp->getIndexCt());
		}
		return 1;
	}
	else {
		return 0; //Scene couldn't be read.
	}
}

void Model::reset() {
	vertexBuffers.clear();
	indexBuffers.clear();
	materialIndices.clear();
	meshes.clear();
	textures.clear();
	scene = NULL;
	modelPath = "";

}

Model::Model() {
	reset();
	moduleType = MOD_MODEL;
}

#pragma endregion

#pragma region Material:
Material::Material() {
	moduleType = MOD_MATERIAL;
}

bool Material::loadModel(GLint progID) {
	Module* model = parent->findModule(MOD_MODEL);
	if (!model || model->getType() != MOD_MODEL) {
		std::cout << "Model module not found\n";
		return 0;
	} //Model component could not be found

	scene = model->getScene();

	filepath = model->getFilepath();
	indexCounts = model->getIndexCounts();
	vertexBuffers = model->getVertexBuffers();
	indexBuffers = model->getIndexBuffers();
	materialIndices = model->getMaterialIndices();

	int lastSlash = filepath.find_last_of('/');
	std::string dir;
	if (lastSlash == std::string::npos)
		dir = ".";
	else if (lastSlash == 0)
		dir = "/";
	else {
		dir = filepath.substr(0, lastSlash);
	}

	textures.resize(scene->mNumMaterials);

	for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
		const aiMaterial* material = scene->mMaterials[i];
		textures[i] = NULL;
		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString path;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
				std::string absoluteFilePath = dir + "/" + path.data;
				textures[i] = new Texture(absoluteFilePath);
			}
		}
	}

	render(progID);

	return 1;
}

// This macro will help us make the attribute pointers
// position, size, type, struct, element
// from: David Erbelding, Niko Procopi (see frag.shader).
#define SetupAttribute(index, size, type, structure, element) \
	glVertexAttribPointer(index, size, type, 0, sizeof(structure), (void*)offsetof(structure, element)); \

void Material::render(GLint progID) {

	for (unsigned int i = 0; i < vertexBuffers.size(); i++) {
		GLuint VAOid;
		glGenVertexArrays(1, &VAOid);
		glBindVertexArray(VAOid);

		for (int n = 0; n < 3; n++)
			glEnableVertexAttribArray(n);

		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[i]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffers[i]);
		
		//Try doing the attribute setup with the VertexArrayObject instead.
		SetupAttribute(0, 3, GL_FLOAT, Vertex, position);
		SetupAttribute(1, 3, GL_FLOAT, Vertex, normal);
		SetupAttribute(2, 2, GL_FLOAT, Vertex, uv);

		
		if (materialIndices[i] < textures.size() && textures[materialIndices[i]]) {
			bind(textures[materialIndices[i]], GL_TEXTURE0);
		}

		GLint uniformA = glGetUniformLocation(progID, "tex");
		std::cout << "uniformBefore   " << glGetError() << std::endl; // returns 0 (no error)
		glUniform1i(uniformA, 0);
		std::cout << "uniformAfter   " << glGetError() << std::endl; // returns 0 (no error)
		
		bVec.push_back(new BufferContainer(VAOid, indexCounts[i]));

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	for (int n = 0; n < 3; n++)
		glDisableVertexAttribArray(n);
}

void Material::pushVAO() {
	//I know this is janky, I'll work out something
	//more elegant later but there's a final I should
	//be studying for tonight.
	for (int i = 0; i < bVec.size(); i++) {
		glVAOs.push(bVec[i]);
	}
}

void Material::bind(Texture* t, GLuint inp) {
	glActiveTexture(inp);
	glBindTexture(GL_TEXTURE_2D, t->texture);
}

void Material::unbind(GLuint inp) {
	glActiveTexture(inp);
	glBindTexture(GL_TEXTURE_2D, 0);
}
#pragma endregion

#pragma region Mesh:
bool Mesh::makeVertexBuffer() {
	for (unsigned int t = 0; t < mesh->mNumVertices; ++t) {
		Vertex v;
		memcpy(&v.position, &mesh->mVertices[t], sizeof(glm::vec3));
		memcpy(&v.normal, &mesh->mNormals[t], sizeof(glm::vec3));
		memcpy(&v.uv, &mesh->mTextureCoords[0][t], sizeof(glm::vec2));
		vertices.push_back(v);

		glGenBuffers(1, &vertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	return 1;
}

bool Mesh::makeIndexBuffer() {
	for (unsigned int t = 0; t < mesh->mNumFaces; ++t) {
		const struct aiFace* face = &mesh->mFaces[t];
		for (int i = 0; i < 3; i++) {
			indices.push_back(face->mIndices[i]);
		}
		if (face->mNumIndices == 4) {
			indices.push_back(face->mIndices[0]);
			indices.push_back(face->mIndices[2]);
			indices.push_back(face->mIndices[3]);
		}
	}

	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	return 1;
}

#pragma endregion

#pragma region Texture:
Texture::Texture(std::string filenameM) {
	unsigned char* image = stbi_load(filenameM.c_str(), &width, &height, &channels, 0);
	if (image) {
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else {
		std::cout << "TEXTURE: Error loading texture\n";
	}
	stbi_image_free(image);
}
#pragma endregion
