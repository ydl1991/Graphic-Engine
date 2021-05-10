#pragma once
#include "../GraphicObject.h"

class Square : public GraphicObject
{
public:
	Square(const glm::vec3& pos, float sideLength, const glm::vec3 color = { 1.0f, 1.0f, 1.0f });
};

