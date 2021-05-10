#include "SpinningComponent.h"
#include "Object/GraphicObject.h"

SpinningComponent::SpinningComponent(std::weak_ptr<GraphicObject> pOwner, glm::vec3 axis, float speed)
	: IComponent(pOwner)
	, m_speed(speed)
	, m_axis(axis)
{

}

bool SpinningComponent::Update(float frameTime)
{
	if (m_pOwner.expired())
		return false;
	
	m_pOwner.lock()->Rotate(glm::two_pi<float>() * frameTime * m_speed, m_axis);

	return true;
}
