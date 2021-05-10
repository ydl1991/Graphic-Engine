#include "Square.h"

Square::Square(const glm::vec3& pos, float sideLength, const glm::vec3 color)
	: GraphicObject(pos)
{
	enum
	{
		A, B, C, D
	};

	m_vertices = {
		{ { -0.5f, 0.0f, -0.5f }, color },
		{ { 0.5f, 0.0f, -0.5f }, color },
		{ { 0.5f, 0.0f, 0.5f }, color },
		{ { -0.5f, 0.0f, 0.5f }, color }
	};

	for (auto& vertex : m_vertices)
	{
		vertex.pos *= sideLength;
	}

	m_indices = {
		A, B, C,
		C, D, A
	};
}
