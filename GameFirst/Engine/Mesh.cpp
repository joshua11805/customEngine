#include "pch.h"
#include "Mesh.h"
#include "AssetManager.h"
#include "GltfImporter.h"
#include "JsonUtil.h"
#include "Material.h"
#include "Profiler.h"
#include "Renderer.h"
#include "VertexBuffer.h"

// Case-insensitive check for whether 'fileName' ends with 'ext'
static bool HasExtension(const char* fileName, const char* ext)
{
    std::string f(fileName);
    std::string e(ext);
    if (f.size() < e.size())
        return false;
    std::string tail = f.substr(f.size() - e.size());
    return SDL_strcasecmp(tail.c_str(), e.c_str()) == 0;
}


Mesh::Mesh(VertexBuffer* vertexBuffer, Material* material)
{
	m_vertexBuffer = vertexBuffer;
	m_material = material;
}

Mesh::~Mesh()
{
	delete m_vertexBuffer;
    for (Mesh* sub : m_subMeshes) delete sub;
}

void Mesh::Draw(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass)
{
    if (m_vertexBuffer)
    {
        m_material->SetActive(commandBuffer, renderPass);
        m_vertexBuffer->Draw(commandBuffer, renderPass);
    }
    for (Mesh* sub : m_subMeshes)
        sub->Draw(commandBuffer, renderPass);
}

void Mesh::AddSubMesh(Mesh* sub)
{
    m_subMeshes.push_back(sub);
}

/// This version of the Load function allows you to create a mesh from code by specifying the vertices and indices
/// rather than loading them from a file
/// @param vertexData the raw vertex data
/// @param vertexDataSize the size in bytes of the vertex data
/// @param indexData the raw index data
/// @param numIndex the number of indices
/// @param indexStride the size in bytes of a single index
/// @param material
/// @return true on success or false on failure (it always succeeds)
bool Mesh::Load(void* vertexData, uint32_t vertexDataSize, void* indexData, uint32_t numIndex, uint32_t indexStride, Material* material)
{
	m_vertexBuffer = new VertexBuffer(Renderer::Get(), vertexData, vertexDataSize, indexData, numIndex, indexStride);
    m_material = material;

    return true;
}

bool Mesh::Load(const char* fileName, AssetManager* pAssetManager)
{
    PROFILE_SCOPE(MeshLoad);

    // glTF/GLB files go through the standard-format importer (Phase 1: static meshes)
    if (HasExtension(fileName, ".glb") || HasExtension(fileName, ".gltf"))
    {
        return GltfImporter::Load(this, fileName, pAssetManager);
    }

	std::ifstream file(fileName);
	if (!file.is_open())
	{
		return false;
	}

	rapidjson::IStreamWrapper isw(file);
	rapidjson::Document doc;
	doc.ParseStream(isw);

	if (!doc.IsObject())
	{
		DbgAssert(false, "Unable to open Mesh file");
		return false;
	}

	std::string str = doc["metadata"]["type"].GetString();
	int ver = doc["metadata"]["version"].GetInt();

	// Check the metadata
	if (!doc["metadata"].IsObject() ||
		str != "itpmesh" ||
		ver != 3)
	{
		DbgAssert(false, "Mesh File Incorrect Version");
		return false;
	}

	// Check if it's a skinned mesh
	GetBoolFromJSON(doc, "skinned", m_isSkinned);

	// Load Material
	std::string matName;
	GetStringFromJSON(doc, "material", matName);
	m_material = pAssetManager->LoadMaterial(matName);

	// Read the vertex format
	const rapidjson::Value& vertFormat = doc["vertexformat"];
	if (!vertFormat.IsArray() || vertFormat.Size() < 1)
	{
		DbgAssert(false, "Mesh File Invalid Vertex Format");
		return false;
	}

	uint32_t vertSize = 0;
	uint32_t vertNumValues = 0;
	std::string inputLayoutName;
	for (rapidjson::SizeType i = 0; i < vertFormat.Size(); i++)
	{
		if (!vertFormat[i].IsObject())
		{
			DbgAssert(false, "Mesh File Invalid Vertex Format");
			return false;
		}

		inputLayoutName += vertFormat[i]["name"].GetString();

		std::string vertType = vertFormat[i]["type"].GetString();
		uint32_t elementSize = 0;
		if (vertType == "float")
			elementSize = 4;
		else
			elementSize = 1;

		vertNumValues += vertFormat[i]["count"].GetUint();
		vertSize += elementSize * vertFormat[i]["count"].GetUint();
	}

	if (vertNumValues < 3)
	{
		DbgAssert(false, "Mesh File Invalid Vertex Format");
		return false;
	}

	// Load in the vertices
	const rapidjson::Value& vertsJson = doc["vertices"];
	if (!vertsJson.IsArray() || vertsJson.Size() < 1)
	{
		DbgAssert(false, "Mesh File Invalid Vertex Format");
		return false;
	}

	union VertPacked
	{
		float f;
		uint8_t b[4];
	};
	uint32_t numVerts = vertsJson.Size();
	std::vector<VertPacked> vertices;
	vertices.reserve(numVerts * vertSize);
	for (rapidjson::SizeType i = 0; i < vertsJson.Size(); i++)
	{
		const rapidjson::Value& vert = vertsJson[i];
		if (!vert.IsArray() || vert.Size() != vertNumValues)
		{
			DbgAssert(false, "Mesh File Invalid Vertex Format");
			return false;
		}

		// Now stuff all the vertices into the vertex buffer
		if (m_isSkinned)
		{   //special case for skinned
			VertPacked temp;
			// Position/Normal
			for (rapidjson::SizeType j = 0; j < 6; j++)
			{
				temp.f = vert[j].GetDouble();
				vertices.emplace_back(temp);
			}

			// Uints for bones and weights
			for (rapidjson::SizeType j = 6; j < 14; j += 4)
			{
				temp.b[0] = vert[j].GetUint();
				temp.b[1] = vert[j + 1].GetUint();
				temp.b[2] = vert[j + 2].GetUint();
				temp.b[3] = vert[j + 3].GetUint();
				vertices.emplace_back(temp);
			}

			// Last two texture coordinates
			for (rapidjson::SizeType j = 14; j < vert.Size(); j++)
			{
				temp.f = vert[j].GetDouble();
				vertices.emplace_back(temp);
			}
		}
		else
		{
			for (rapidjson::SizeType j = 0; j < vert.Size(); j++)
			{
				VertPacked temp;
				temp.f = vert[j].GetDouble();
				vertices.emplace_back(temp);
			}
		}
	}

	// Load in the indices
	const rapidjson::Value& indJson = doc["indices"];
	if (!indJson.IsArray() || indJson.Size() < 1)
	{
		DbgAssert(false, "Mesh File Invalid Index Format");
		return false;
	}

	std::vector<uint16_t> indices;
	indices.reserve(indJson.Size() * 3);
	for (rapidjson::SizeType i = 0; i < indJson.Size(); i++)
	{
		const rapidjson::Value& ind = indJson[i];
		if (!ind.IsArray() || ind.Size() != 3)
		{
			DbgAssert(false, "Mesh File Invalid Index Format");
			return false;
		}

		indices.emplace_back(static_cast<uint16_t>(ind[0].GetUint()));
		indices.emplace_back(static_cast<uint16_t>(ind[1].GetUint()));
		indices.emplace_back(static_cast<uint16_t>(ind[2].GetUint()));
	}

	// Now create a vertex array
	return Load((void*)vertices.data(), numVerts * vertSize, (void*)
		indices.data(), (uint32_t)indices.size(), sizeof(uint16_t),
		m_material);
}

Mesh* Mesh::StaticLoad(const char* fileName, AssetManager* pAssetManager)
{
    Mesh* pMesh = new Mesh(nullptr, nullptr);
    if (false == pMesh->Load(fileName, pAssetManager))
    {
        delete pMesh;
        return nullptr;
    }
    return pMesh;
}


