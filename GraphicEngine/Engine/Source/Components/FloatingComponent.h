#pragma once

#include "../Interfaces/IComponent.h"
#include <memory>

class FloatingComponent : public IComponent
{
	float m_offset;
	float m_currentOffset;
	float m_speed;
	float m_dir;
public:
	FloatingComponent(std::weak_ptr<GraphicObject> pOwner, float offset, float speed);

	virtual bool Update(float frameTime) final override;
};

