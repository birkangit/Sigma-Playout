#pragma once

#include <memory>

#include "gl_check.h"

namespace caspar { namespace common { namespace gpu {
	
class texture
{
public:
	texture(size_t width, size_t height) : width_(width), height_(height)
	{	
		CASPAR_GL_CHECK(glGenTextures(1, &texture_));	

		CASPAR_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_));

		CASPAR_GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		CASPAR_GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		CASPAR_GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		CASPAR_GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

		CASPAR_GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL));
	}

	~texture()
	{		
		glDeleteTextures(1, &texture_);
		texture_ = 0;
	}
		
	void draw()
	{
		CASPAR_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_));
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f, -1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f,  1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f,  1.0f);
		glEnd();	
	}
	size_t width() const { return width_; }
	size_t height() const { return height_; }
	GLuint handle() { return texture_; }

private:
	GLuint texture_;
	const size_t width_;
	const size_t height_;
};
typedef std::shared_ptr<texture> texture_ptr;

}}}