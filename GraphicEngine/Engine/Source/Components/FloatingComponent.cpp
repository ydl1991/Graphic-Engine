#include "FloatingComponent.h"
#include "Object/GraphicObject.h"

FloatingComponent::FloatingComponent(std::weak_ptr<GraphicObject> pOwner, float offset, float speed)
	: IComponent(pOwner)
	, m_offset(offset)
	, m_speed(speed)
	, m_currentOffset(0.f)
	, m_dir(1.f)
{

}

bool FloatingComponent::Update(float frameTime)
{
	if (m_pOwner.expired())
		return false;

	if (m_currentOffset > m_offset)
	{
		m_dir = -1.f;
	}
	else if (m_currentOffset < -m_offset)
	{
		m_dir = 1.f;
	}

	m_currentOffset += frameTime * m_speed * m_dir;
	m_pOwner.lock()->ChangePosition(glm::vec3(0, frameTime * m_speed * m_dir, 0));

	return true;
}
