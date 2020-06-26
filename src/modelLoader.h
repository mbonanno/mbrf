#pragma once

#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

namespace MBRF
{

// TODO: add to model/mesh file?
struct ModelVertex
{
	glm::vec3 m_pos;
	glm::vec3 m_normal;
	glm::vec4 m_color;
	glm::vec2 m_texcoord;
};

struct ModelData
{
	std::vector<ModelVertex> m_vertexBuffer;
	std::vector<uint32_t> m_indexBuffer;
};

class ModelLoader
{
public:

	static void LoadFromFile(const char* fileName, ModelData& modelData, float scale = 1.0f);
};

}

