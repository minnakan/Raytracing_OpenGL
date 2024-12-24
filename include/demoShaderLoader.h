#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>

struct Shader
{
	GLuint id = 0;

	bool loadShaderProgramFromData(const char *vertexShaderData, const char *fragmentShaderData);
	bool loadShaderProgramFromData(const char *vertexShaderData,
		const char *geometryShaderData, const char *fragmentShaderData);

	bool loadShaderProgramFromFile(const char *vertexShader, const char *fragmentShader);
	bool loadShaderProgramFromFile(const char *vertexShader,
		const char *geometryShader, const char *fragmentShader);

	void use();

	void clear();

	GLint getUniform(const char *name);


	void setBool(const std::string& name, bool value) const;
	void setInt(const std::string& name, int value) const;
	void setFloat(const std::string& name, float value) const;
	void setVec4f(const std::string& name, float x, float y, float z, float w) const;
	void setVec3(const std::string& name, float x, float y, float z)const;
	void setMat4(const std::string& name, const glm::mat4& mat)const;
};

GLint getUniform(GLuint shaderId, const char *name);
