#pragma once
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

struct Vertex
{
	glm::vec3 pos;
	//glm::vec2 uv;
	glm::vec3 normal;

	Vertex() : pos(), normal() {}
	Vertex(glm::vec3 pos) : pos(pos), normal() {}
	//Vertex(glm::vec3 pos, glm::vec2 uv) : pos(pos), uv(uv), normal() {}
	Vertex(glm::vec3 pos, glm::vec3 nor) : pos(pos), normal(nor) {}
};

struct Face
{
	static constexpr size_t s_kFaceIndicesSize = 3;
	glm::ivec3 indices[s_kFaceIndicesSize];		// Pos, UV, Normal indices

	Face(glm::ivec3 v1, glm::ivec3 v2, glm::ivec3 v3)
		: indices()
	{
		indices[0] = v1;
		indices[1] = v2;
		indices[2] = v3;
	}
};

struct Uniforms
{
	glm::mat4 viewMatrix;
	glm::mat4 projMatrix;
	glm::vec4 lightPosition;
	glm::vec4 lightColor;
	glm::vec4 cameraPosition;
};

struct ObjectUniforms
{
	glm::mat4 worldMatrix;
	glm::vec4 materialDiffuse;
	glm::vec4 materialEmissive;
	glm::vec4 materialAmbient;
	glm::vec4 materialSpecular;
	float materialShininess;
	bool enableLighting;

	ObjectUniforms() 
		: worldMatrix(glm::identity<glm::mat4>())
		, materialDiffuse()
		, materialEmissive()
		, materialAmbient()
		, materialSpecular()
		, materialShininess(32)
		, enableLighting(false) 
	{

	}
};