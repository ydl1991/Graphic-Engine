#pragma once
#include "../GraphicObject.h"

class Cube : public GraphicObject
{
public:
	Cube(const glm::vec3& pos, float sideLength, const glm::vec3 color = {1.0f, 1.0f, 1.0f});
};

