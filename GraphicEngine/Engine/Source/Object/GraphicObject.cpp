#include "GraphicObject.h"
#include "../Interfaces/IComponent.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

GraphicObject::GraphicObject()
	: m_objectUniform()
	, m_position()
	, m_vertices()
	, m_indices()
{
	m_components.reserve(100);
	m_delayComponentRemoveList.reserve(100);

	m_children.reserve(100);
	m_delayChildrenRemoveList.reserve(100);
}

GraphicObject::GraphicObject(const glm::vec3& pos)
	: m_position(pos)
	, m_objectUniform()
{
	m_components.reserve(100);
	m_delayComponentRemoveList.reserve(100);

	m_children.reserve(100);
	m_delayChildrenRemoveList.reserve(100);

	m_objectUniform.worldMatrix = glm::translate(glm::identity<glm::mat4>(), m_position);
}

GraphicObject::GraphicObject(const glm::vec3& pos, const std::vector<Vertex>& vertices)
	: m_vertices(vertices)
	, m_position(pos)
	, m_objectUniform()
{
	m_indices.reserve(1000);
	m_components.reserve(100);
	m_delayComponentRemoveList.reserve(100);

	m_children.reserve(100);
	m_delayChildrenRemoveList.reserve(100);

	m_objectUniform.worldMatrix = glm::translate(glm::identity<glm::mat4>(), m_position);
}

GraphicObject::GraphicObject(const glm::vec3& pos, std::vector<Vertex>&& vertices)
	: m_vertices(vertices)
	, m_position(pos)
	, m_objectUniform()
{
	m_indices.reserve(1000);
	m_components.reserve(100);
	m_delayComponentRemoveList.reserve(100);

	m_children.reserve(100);
	m_delayChildrenRemoveList.reserve(100);

	m_objectUniform.worldMatrix = glm::translate(glm::identity<glm::mat4>(), m_position);
}

GraphicObject::GraphicObject(const glm::vec3& pos, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
	: m_vertices(vertices)
	, m_indices(indices)
	, m_position(pos)
	, m_objectUniform()
{
	m_components.reserve(100);
	m_delayComponentRemoveList.reserve(100);

	m_children.reserve(100);
	m_delayChildrenRemoveList.reserve(100);

	m_objectUniform.worldMatrix = glm::translate(glm::identity<glm::mat4>(), m_position);
}

GraphicObject::GraphicObject(const glm::vec3& pos, std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices)
	: m_vertices(vertices)
	, m_indices(indices)
	, m_position(pos)
	, m_objectUniform()
{
	m_components.reserve(100);
	m_delayComponentRemoveList.reserve(100);

	m_children.reserve(100);
	m_delayChildrenRemoveList.reserve(100);

	m_objectUniform.worldMatrix = glm::translate(glm::identity<glm::mat4>(), m_position);
}

GraphicObject::~GraphicObject()
{
}

void GraphicObject::Update(float frameTime)
{
	for (size_t i = 0; i < m_components.size(); ++i)
	{
		if (!m_components[i]->Update(frameTime))
		{
			m_delayComponentRemoveList.emplace_back(i);
		}
	}

	RemoveExpiredComponentsAndChildren();
}

void GraphicObject::ChangePosition(const glm::vec3& delta)
{
	m_position += delta;
	m_objectUniform.worldMatrix = glm::translate(glm::identity<glm::mat4>(), m_position);

	for (int i = (int)m_children.size() - 1; i >= 0; --i)
	{
		if (m_children[i].expired())
		{
			m_delayChildrenRemoveList.emplace_back(i);
			continue;
		}

		m_children[i].lock()->ChangePosition(delta);
	}
}

void GraphicObject::SetPosition(const glm::vec3& pos)
{
	glm::vec3 delta = pos - m_position;

	m_position = pos;
	m_objectUniform.worldMatrix = glm::translate(glm::identity<glm::mat4>(), m_position);

	for (int i = (int)m_children.size() - 1; i >= 0; --i)
	{
		if (m_children[i].expired())
		{
			m_delayChildrenRemoveList.emplace_back(i);
			continue;
		}

		m_children[i].lock()->ChangePosition(delta);
	}
}

void GraphicObject::Rotate(float angle, glm::vec3 axis)
{
	m_objectUniform.worldMatrix = glm::rotate(m_objectUniform.worldMatrix, angle, axis);
}

void GraphicObject::Draw(vk::CommandBuffer& cb)
{
	cb.bindVertexBuffers(0, m_vertexBuffer, vk::DeviceSize(0));
	
	if (m_indices.size() > 0)
	{
		cb.bindIndexBuffer(m_indexBuffer, vk::DeviceSize(0), vk::IndexType::eUint32);
		cb.drawIndexed(static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
	}
	else
	{
		cb.draw(static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);
	}
}

void GraphicObject::AddComponent(std::unique_ptr<IComponent> comp)
{
	for (size_t i = 0; i < m_components.size(); ++i)
	{
		if (!m_components[i])
		{
			m_components[i] = std::move(comp);
			return;
		}
	}

	m_components.emplace_back(std::move(comp));
}

void GraphicObject::RemoveExpiredComponentsAndChildren()
{
	for (size_t index : m_delayComponentRemoveList)
	{
		m_components[index] = nullptr;
	}

	for (size_t index : m_delayChildrenRemoveList)
	{
		std::swap(m_children[index], m_children.back());
		m_children.pop_back();
	}
}

void GraphicObject::AddChild(std::weak_ptr<GraphicObject> child)
{
	if (child.expired())
		return;

	m_children.emplace_back(child);
}

void GraphicObject::SetMaterialDiffuse(const glm::vec4& diffuse)
{
	m_objectUniform.materialDiffuse = diffuse;
}

void GraphicObject::SetMaterialEmissive(const glm::vec4& emissive)
{
	m_objectUniform.materialEmissive = emissive;
}

void GraphicObject::SetMaterialSpecular(const glm::vec4& specular)
{
	m_objectUniform.materialSpecular = specular;
}

void GraphicObject::SetMaterialAmbient(const glm::vec4& ambient)
{
	m_objectUniform.materialAmbient = ambient;
}

void GraphicObject::SetMaterialShininess(float shine)
{
	m_objectUniform.materialShininess = shine;
}
