#pragma once
#include <memory>

class GraphicObject;

class IComponent
{
protected:
	std::weak_ptr<GraphicObject> m_pOwner;

public:
	IComponent(std::weak_ptr<GraphicObject> pOwner) : m_pOwner(pOwner) {}

	virtual bool Update(float frameTime) = 0;
};

