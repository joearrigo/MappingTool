#include "model.h"
#include "headers/dstream.hpp"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <fstream>
#include <gl/GL.h>

void Mesh::makeVBuffer() {
	for (unsigned int t = 0; t < mesh->mNumVertices; ++t) {
		Vertex v;
		memcpy(&v.position, &mesh->mVertices[t], sizeof(glm::vec3));
		memcpy(&v.normal, &mesh->mNormals[t], sizeof(glm::vec3));
		memcpy(&v.texCoord, &mesh->mTextureCoords[0][t], sizeof(glm::vec2));
		vertices.push_back(v);
		
		printf("X: %f | Y: %f\n", v.texCoord[0], v.texCoord[1]);

		glGenBuffers(1, &vertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

void Mesh::makeIBuffer() {
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
}

jModel::jModel(const std::string filepathM) {
	std::ifstream file(filepathM);
	if (!file.good())
	{
		dout << "Error reading model file at: " << filepathM << "\n";
		return;
	}

	filepath = filepathM;

	loadModel();
}

bool jModel::loadModel() {
	scene = importer.ReadFile(filepath, aiProcess_Triangulate |
		aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
		aiProcess_JoinIdenticalVertices);

	if (scene) {
		for (unsigned int j = 0; j < scene->mNumMeshes; j++)
			meshes.push_back(Mesh(scene->mMeshes[j]));
		genTextures();
		return 1; //return 1 if success
	}
	else {
		return 0;
	}
}

void jModel::genTextures() {
	textures.resize(scene->mNumMaterials);

	/// 
	/// Get absolute dir for file
	/// 
	int lastSlash = filepath.find_last_of('/');
	std::string dir;
	if (lastSlash == std::string::npos)
		dir = ".";
	else if (lastSlash == 0)
		dir = "/";
	else {
		dir = filepath.substr(0, lastSlash);
	}

	for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
		const aiMaterial* material = scene->mMaterials[i];
		textures[i] = NULL;

		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString path;

			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
				std::string newFp = dir + "/" + path.data;
				textures[i] = new Texture(newFp);
			}
		}
	}
}

// This macro will help us make the attribute pointers
// position, size, type, struct, element
// from: David Erbelding, Niko Procopi (see frag.shader).
#define SetupAttribute(index, size, type, structure, element) \
	glVertexAttribPointer(index, size, type, 0, sizeof(structure), (void*)offsetof(structure, element)); \

void jModel::render(GLint progID) {

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	for (unsigned int i = 0; i < meshes.size(); i++) {
		glBindBuffer(GL_ARRAY_BUFFER, meshes[i].vertexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshes[i].indexBuffer);
		SetupAttribute(0, 3, GL_FLOAT, Vertex, position);
		SetupAttribute(1, 3, GL_FLOAT, Vertex, normal);
		SetupAttribute(2, 2, GL_FLOAT, Vertex, texCoord);
		
		const unsigned int materialIndex = meshes[i].materialIndex;
		if (materialIndex < textures.size() && textures[materialIndex]) {
			textures[materialIndex]->bind(GL_TEXTURE0);
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textures[meshes[i].materialIndex]->texture);

		GLint uniformA = glGetUniformLocation(progID, "tex");
		glUniform1i(uniformA, 0);
		
		glDrawElements(GL_TRIANGLES, (int) meshes[i].indices.size(), GL_UNSIGNED_SHORT, 0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}