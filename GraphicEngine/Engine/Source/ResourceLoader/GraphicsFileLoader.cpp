#include "GraphicsFileLoader.h"
#include <fstream>
#include <iostream>
#include <algorithm>

FileData::FileData(const std::vector<Vertex>& outVertices, const std::vector<uint32_t>& outIndices)
	: m_vertices(outVertices.size())
	, m_outIndices(outIndices.size())
{
	std::memcpy(m_vertices.data(), outVertices.data(), outVertices.size() * sizeof(Vertex));
	std::memcpy(m_outIndices.data(), outIndices.data(), outIndices.size() * sizeof(uint32_t));
}

void FileData::ExtractData(std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices)
{
	std::memcpy(outVertices.data(), m_vertices.data(), m_vertices.size() * sizeof(Vertex));
	std::memcpy(outIndices.data(), m_outIndices.data(), m_outIndices.size() * sizeof(uint32_t));
}

GraphicsFileLoader::GraphicsFileLoader()
	: m_fileDataCache()
{
}

GraphicsFileLoader::~GraphicsFileLoader()
{
	m_fileDataCache.clear();
}

bool GraphicsFileLoader::LoadOBJFile(const char* pFilename, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices)
{
	// if we already loaded it in the catch, retrieve it.
	if (m_fileDataCache.find(pFilename) != m_fileDataCache.end())
	{
		m_fileDataCache[pFilename].ExtractData(outVertices, outIndices);
		return true;
	}

	std::ifstream objFile(pFilename, std::ios::in | std::ios::binary);
	if (!objFile.is_open())
	{
		std::cout << "Fail to open " << pFilename << ".obj file for reading.";
		return false;
	}
	
	// Parsing Lambda
	auto parseVertexData = [](std::vector<Vertex>& verts, std::vector<glm::vec2>& vts, std::vector<glm::vec3>& vns, const std::string& line)
	{
		// Positions of spaces
		size_t s1 = line.find_first_of(' ') + 1;
		size_t s2 = line.find(' ', s1) + 1;
		size_t s3 = line.find_last_of(' ') + 1;
		float f1 = std::stof(line.substr(s1, s2 - s1 - 1));
		float f2 = std::stof(line.substr(s2, s3 - s2 - 1));
		float f3 = std::stof(line.substr(s3));

		if (line[1] == ' ')
		{
			verts.emplace_back(Vertex(glm::vec3{ f1, f2, f3 }));
		}
		else if (line[1] == 'n')
		{
			vns.emplace_back(glm::vec3{ f1, f2, f3 });
		}
		else if (line[1] == 't')
		{
			vts.emplace_back(glm::vec2{ f1, f2 });
		}
	};

	auto parseFaceData = [](std::vector<Face>& faces, const std::string& line)
	{
		// Positions of spaces
		size_t s1 = line.find_first_of(' ') + 1;
		size_t s2 = line.find(' ', s1) + 1;
		size_t s3 = line.find_last_of(' ') + 1;

		auto parseIndices = [](const std::string& indices) -> glm::ivec3
		{
			// Positions of slashes
			size_t s1 = indices.find_first_of('/');
			size_t s2 = indices.find_last_of('/');

			// All the index needs to subtract 1
			if (s1 == std::string::npos)
				return glm::ivec3{ std::stoi(indices) - 1, -1, -1 };
			else if (s1 == s2)
				return glm::ivec3{ std::stoi(indices.substr(0, s1)) - 1, std::stoi(indices.substr(s1 + 1)) - 1, -1 };
			else if (s1 + 1 == s2)
				return glm::ivec3{ std::stoi(indices.substr(0, s1)) - 1, -1, std::stoi(indices.substr(s2 + 1)) - 1 };
			else
				return glm::ivec3{
					std::stoi(indices.substr(0, s1)) - 1, 
					std::stoi(indices.substr(s1 + 1, s2 - s1 - 1)) - 1, 
					std::stoi(indices.substr(s2 + 1)) - 1 
				};
		};

		faces.emplace_back(
			Face(
				parseIndices(line.substr(s1, s2 - s1 - 1)), 
				parseIndices(line.substr(s2, s3 - s2 - 1)), 
				parseIndices(line.substr(s3))
			)
		);
	};

	// Parsing obj data line by line
	std::vector<glm::vec2> vts;
	std::vector<glm::vec3> vns;
	std::vector<Face> faces;
	std::string line;
	while (std::getline(objFile, line))
	{
		if (line[0] == 'v')
		{
			parseVertexData(outVertices, vts, vns, line);
		}
		else if (line[0] == 'f')
		{
			parseFaceData(faces, line);
		}
	}

	objFile.close();

	// update vertex uv and normal data, and update indices
	std::for_each(faces.begin(), faces.end(), [&outVertices, &outIndices, &vts, &vns](const Face& face)
	{
		for (size_t i = 0; i < face.s_kFaceIndicesSize; ++i)
		{
			if (face.indices[i].y != -1)
				//outVertices[face.indices[i].x].uv = vts[face.indices[i].y];

			if (face.indices[i].z != -1)
				outVertices[face.indices[i].x].normal = vns[face.indices[i].z];

			outIndices.emplace_back(face.indices[i].x);
		}
	});

	// store data in cache for next time use
	m_fileDataCache.insert_or_assign(pFilename, FileData(outVertices, outIndices));

	// ------------------------------ //
	// Testing Print Function Calls
	// ------------------------------ //

	//std::for_each(outVertices.begin(), outVertices.end(), [](const Vertex& v) {
	//	std::cout << "v " << v.m_pos.x << " " << v.m_pos.y << " " << v.m_pos.z << std::endl;
	//	//std::cout << "vt " << v.m_uv.x << " " << v.m_uv.y << std::endl;
	//	//std::cout << "vn " << v.m_normal.x << " " << v.m_normal.y << " " << v.m_normal.z << std::endl;
	//});

	//std::for_each(vts.begin(), vts.end(), [](const glm::vec2& v) {
	//	std::cout << "vt " << v.x << " " << v.y << std::endl;
	//});

	//std::for_each(vns.begin(), vns.end(), [](const glm::vec3& v) {
	//	std::cout << "vn " << v.x << " " << v.y << " " << v.z << std::endl;
	//});

	//std::for_each(faces.begin(), faces.end(), [](const Face& f) {
	//	std::cout << "f " << f.m_indices[0].x << "/" << f.m_indices[0].y << "/" << f.m_indices[0].z
	//			  << " " << f.m_indices[1].x << "/" << f.m_indices[1].y << "/" << f.m_indices[1].z
	//			  << " " << f.m_indices[2].x << "/" << f.m_indices[2].y << "/" << f.m_indices[2].z << std::endl;
	//});

	return true;
}