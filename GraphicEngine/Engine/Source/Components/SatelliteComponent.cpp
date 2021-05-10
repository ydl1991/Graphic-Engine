#include "SatelliteComponent.h"
#include "Object/GraphicObject.h"
#include <glm/gtx/euler_angles.hpp>
#include <iostream>

SatelliteComponent::SatelliteComponent(std::weak_ptr<GraphicObject> pOwner, std::weak_ptr<GraphicObject> pSurrounding, float speed)
	: IComponent(pOwner)
	, m_speed(speed)
	, m_pSurrounding(pSurrounding)
{

}

bool SatelliteComponent::Update(float frameTime)
{
	if (m_pOwner.expired() || m_pSurrounding.expired())
		return false;
	
	double deltaAngle = (double)(frameTime * glm::two_pi<double>() * m_speed);
	double cosTheta = cos(deltaAngle);
	double sinTheta = sin(deltaAngle);

	glm::vec3 curPos = m_pOwner.lock()->Position();
	glm::vec3 center = m_pSurrounding.lock()->Position();
	glm::vec3 newPos;
	newPos.x = (float)(cosTheta * (curPos.x - center.x) - sinTheta * (curPos.z - center.z) + center.x);
	newPos.z = (float)(sinTheta * (curPos.x - center.x) + cosTheta * (curPos.z - center.z) + center.z);
	newPos.y = curPos.y;

	m_pOwner.lock()->SetPosition(newPos);
	
	return true;
}
