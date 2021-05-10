#pragma once

#include "../Interfaces/IComponent.h"
#include <memory>
#include <glm/ext/matrix_transform.hpp>

class SpinningComponent : public IComponent
{
	float m_speed;
	glm::vec3 m_axis;

public:
	SpinningComponent(std::weak_ptr<GraphicObject> pOwner, glm::vec3 axis, float speed);

	virtual bool Update(float frameTime) final override;
};

