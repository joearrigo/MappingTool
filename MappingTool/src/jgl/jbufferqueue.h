#ifndef JBUFFERQUEUE_H
#define JBUFFERQUEUE_H

#include <GL/glew.h>
#include <vector>
#include <string>
#include <queue>
#include <GLFW/glfw3.h>

struct BufferContainer {
	GLuint VAO;
	int numIndices;

	BufferContainer(GLuint vaoIn, int numIndicesIn) {
		VAO = vaoIn;
		numIndices = numIndicesIn;
	}

};

extern std::queue<BufferContainer*> glVAOs;



#endif