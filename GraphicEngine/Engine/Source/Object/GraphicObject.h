#pragma once
#include "ResourceLoader/GraphicsData.h"
#include "Framework/Framework.h"

#include <vector>
#include <memory>

class IComponent;

class GraphicObject
{
protected:
	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;

	std::vector<std::unique_ptr<IComponent>> m_components;
	std::vector<size_t> m_delayComponentRemoveList;

	std::vector<std::weak_ptr<GraphicObject>> m_children;
	std::vector<size_t> m_delayChildrenRemoveList;

	vk::Buffer m_vertexBuffer;
	vk::DeviceMemory m_vertexBufferMemory;
	vk::Buffer m_indexBuffer;
	vk::DeviceMemory m_indexBufferMemory;

	glm::vec3 m_position;

public:
	ObjectUniforms m_objectUniform;

public:
	GraphicObject();
	GraphicObject(const glm::vec3& pos);
	GraphicObject(const glm::vec3& pos, const std::vector<Vertex>& vertices);
	GraphicObject(const glm::vec3& pos, std::vector<Vertex>&& vertices);
	GraphicObject(const glm::vec3& pos, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
	GraphicObject(const glm::vec3& pos, std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices);

	~GraphicObject();
	GraphicObject(const GraphicObject&) = default;
	GraphicObject& operator=(const GraphicObject&) = default;
	GraphicObject(GraphicObject&&) = default;
	GraphicObject& operator=(GraphicObject&&) = default;

	void Update(float frameTime);
	void ChangePosition(const glm::vec3& delta);
	void SetPosition(const glm::vec3& pos);
	void Rotate(float angle, glm::vec3 axis);
	void Draw(vk::CommandBuffer& cb);

	void AddComponent(std::unique_ptr<IComponent> comp);
	void RemoveExpiredComponentsAndChildren();
	void AddChild(std::weak_ptr<GraphicObject> child);

	void EnableLighting() { m_objectUniform.enableLighting = true; }
	void DisableLighting() { m_objectUniform.enableLighting = false; }
	void SetMaterialDiffuse(const glm::vec4& diffuse);
	void SetMaterialEmissive(const glm::vec4& emissive);
	void SetMaterialSpecular(const glm::vec4& specular);
	void SetMaterialAmbient(const glm::vec4& ambient);
	void SetMaterialShininess(float shine);

	std::vector<Vertex>& Vertices() { return m_vertices; }
	std::vector<uint32_t>& Indices() { return m_indices; }
	vk::Buffer& VertexBuffer() { return m_vertexBuffer; }
	vk::DeviceMemory& VertexBufferMemory() { return m_vertexBufferMemory; }
	vk::Buffer& IndexBuffer() { return m_indexBuffer; }
	vk::DeviceMemory& IndexBufferMemory() { return m_indexBufferMemory; }
	//size_t UniformSize() const { return sizeof(m_objectUniform); }
	//ObjectUniforms& Uniform() { return m_objectUniform; }
	glm::vec3 Position() const { return m_position; }
};

