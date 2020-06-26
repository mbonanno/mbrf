#include "modelLoader.h"

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

namespace MBRF
{

void ModelLoader::LoadFromFile(const char* fileName, ModelData& modelData, float scale)
{
	// TODO: handle different vertex formats?

	const aiScene* scene;
	Assimp::Importer Importer;

	static const int assimpFlags = /*aiProcess_FlipWindingOrder |*/ aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenNormals;

	scene = Importer.ReadFile(fileName, assimpFlags);

	// Vertex Buffer

	std::vector<ModelVertex> vertexBuffer;

	for (uint32_t m = 0; m < scene->mNumMeshes; m++)
	{
		for (uint32_t v = 0; v < scene->mMeshes[m]->mNumVertices; v++)
		{
			ModelVertex vertex;

			vertex.m_pos = glm::make_vec3(&scene->mMeshes[m]->mVertices[v].x) * scale;
			vertex.m_normal = glm::make_vec3(&scene->mMeshes[m]->mNormals[v].x);
			// Texture coordinates and colors may have multiple channels, we only use the first [0] one
			vertex.m_texcoord = glm::make_vec2(&scene->mMeshes[m]->mTextureCoords[0][v].x);

			vertex.m_color = (scene->mMeshes[m]->HasVertexColors(0)) ? glm::make_vec4(&scene->mMeshes[m]->mColors[0][v].r) : glm::vec4(1.0f);

			modelData.m_vertexBuffer.push_back(vertex);
		}
	}

	// Index Buffer

	for (uint32_t m = 0; m < scene->mNumMeshes; m++)
	{
		uint32_t indexBase = static_cast<uint32_t>(modelData.m_indexBuffer.size());
		for (uint32_t f = 0; f < scene->mMeshes[m]->mNumFaces; f++)
		{
			for (uint32_t i = 0; i < 3; i++)
				modelData.m_indexBuffer.push_back(scene->mMeshes[m]->mFaces[f].mIndices[i] + indexBase);
		}
	}
}

}
