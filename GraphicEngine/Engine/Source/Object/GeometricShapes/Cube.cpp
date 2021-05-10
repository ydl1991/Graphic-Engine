#include "Cube.h"

Cube::Cube(const glm::vec3& pos, float sideLength, const glm::vec3 color)
	: GraphicObject(pos)
{
	// -.5  +.5
	//    E---F
	//   /|  /|
	//  A---B |  +.5
	//  | | | |
	//  | H-|-G
	//  |/  |/
	//  D---C    -.5

	enum
	{
		A, B, C, D, E, F, G, H
	};

	m_vertices = {
		{ { -0.5,  0.5, -0.5 }, color }, // A
		{ {  0.5,  0.5, -0.5 }, color }, // B
		{ {  0.5, -0.5, -0.5 }, color }, // C
		{ { -0.5, -0.5, -0.5 }, color }, // D
		{ { -0.5,  0.5, 0.5 }, color },	 // E
		{ {  0.5,  0.5, 0.5 }, color },	 // F
		{ {  0.5, -0.5, 0.5 }, color },	 // G
		{ { -0.5, -0.5, 0.5 }, color },	 // H
	};									

	for (auto& vertex : m_vertices)
	{
		vertex.pos *= sideLength;
	}

	m_indices = {
		A, C, D,
		A, B, C,
		B, F, G,
		B, G, C,
		G, F, E,
		H, G, E,
		E, A, H,
		A, D, H,
		E, F, B,
		E, B, A,
		C, G, H,
		D, C, H,
	};
}
