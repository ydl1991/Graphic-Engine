#pragma once
#include <glm/glm.hpp>

class Camera
{
	glm::vec3 m_cameraPosition;
	glm::vec3 m_cameraDirection;

	glm::mat4 m_viewMatrix;
	glm::mat4 m_projectionMatrix;

	float m_yaw;
	float m_pitch;

public:
	Camera();
	Camera(glm::vec3 initalPos, glm::vec3 initalDir);
	~Camera();
	Camera(const Camera&) = delete;
	Camera& operator=(const Camera&) = delete;
	Camera(Camera&&) = default;
	Camera& operator=(Camera&&) = default;

	void Update();

	void SetPerspectiveView(float fov, float width, float height, float near, float far);
	void SetOrthoView(float left, float right, float top, float bot, float near, float far);

	glm::vec3 Position() const { return m_cameraPosition; }
	glm::vec3 Direction() const { return m_cameraDirection; }
	void SetPosition(const glm::vec3& newPos);
	void SetDirection(const glm::vec3& newDir);
	float Yaw() const { return m_yaw; }
	void SetYaw(float nYaw) { m_yaw = nYaw; }
	float Pitch() const { return m_pitch; }
	void SetPitch(float nPitch) { m_pitch = nPitch; }

	glm::mat4 ViewMatrix() const { return m_viewMatrix; }
	void UpdateViewMatrix();

	glm::mat4 ProjMatrix() const { return m_projectionMatrix; }

};

