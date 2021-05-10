#pragma once

#include "../Interfaces/IComponent.h"
#include <memory>
#include <glm/ext/matrix_transform.hpp>

class SatelliteComponent : public IComponent
{
	float m_speed;
	std::weak_ptr<GraphicObject> m_pSurrounding;

public:
	SatelliteComponent(std::weak_ptr<GraphicObject> pOwner, std::weak_ptr<GraphicObject> pSurrounding, float speed);

	virtual bool Update(float frameTime) final override;
};

