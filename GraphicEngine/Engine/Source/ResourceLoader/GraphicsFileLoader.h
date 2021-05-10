#pragma once
#include <vector>
#include <unordered_map>
#include <string>

#include "GraphicsData.h"

struct FileData
{
	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_outIndices;

	FileData() : m_vertices(), m_outIndices() {}
	FileData(const std::vector<Vertex>& outVertices, const std::vector<uint32_t>& outIndices);
	void ExtractData(std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices);
};

class GraphicsFileLoader
{
	std::unordered_map<std::string, FileData> m_fileDataCache;

public:
	GraphicsFileLoader();
	~GraphicsFileLoader();
	GraphicsFileLoader(const GraphicsFileLoader&)			 = delete;
	GraphicsFileLoader& operator=(const GraphicsFileLoader&) = delete;
	GraphicsFileLoader(GraphicsFileLoader&&)				 = default;
	GraphicsFileLoader& operator=(GraphicsFileLoader&&)		 = default;

	bool LoadOBJFile(const char* pFilename, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices);
};