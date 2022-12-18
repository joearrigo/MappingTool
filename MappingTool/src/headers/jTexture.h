#ifndef JTEXTURE_H
#define JTEXTURE_H

#include <string>
#include <GL/glew.h>

class Texture {
	private:
		std::string filename;
		int width = 0, height = 0, channels = 0;
	public:
		unsigned int texture = 0;
		Texture(std::string filenameM);

		void getImageSize(int& widthM, int& heightM) {
			widthM = width;
			heightM = height;
		}

		void bind(int inp){
			glActiveTexture(inp);
			glBindTexture(GL_TEXTURE_2D, texture);
		}

		void unbind() {
			glBindTexture(GL_TEXTURE_2D, 0);
		}
};
#endif