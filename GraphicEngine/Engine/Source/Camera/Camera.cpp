#include "Camera.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

Camera::Camera()
	: m_cameraPosition()
	, m_cameraDirection()
	, m_viewMatrix(glm::identity<glm::mat4>())
	, m_projectionMatrix(glm::identity<glm::mat4>())
	, m_yaw(0)
	, m_pitch(0)
{

}

Camera::Camera(glm::vec3 initalPos, glm::vec3 initalDir)
	: m_cameraPosition(initalPos)
	, m_cameraDirection(initalDir)
	, m_viewMatrix(glm::identity<glm::mat4>())
	, m_projectionMatrix(glm::identity<glm::mat4>())
	, m_yaw(0)
	, m_pitch(0)
{
}

Camera::~Camera()
{
}

void Camera::Update()
{
	glm::mat4 cameraRotation = glm::eulerAngleYX(m_yaw, m_pitch);
	SetDirection(glm::vec3(cameraRotation * glm::vec4(0, 0, 1, 0)));
	UpdateViewMatrix();
}

void Camera::SetPerspectiveView(float fov, float width, float height, float near, float far)
{
	m_projectionMatrix = glm::perspectiveFov(fov, width, height, near, far);
}

void Camera::SetOrthoView(float left, float right, float top, float bot, float near, float far)
{
	//glm::ortho(0.0f, m_windowWidth * 1.0f, m_windowHeight * 1.0f, 0.0f);
}

void Camera::SetPosition(const glm::vec3& newPos)
{
	m_cameraPosition = newPos;
}

void Camera::SetDirection(const glm::vec3& newDir)
{
	m_cameraDirection = newDir;
}

void Camera::UpdateViewMatrix()
{
	m_viewMatrix = glm::lookAt(m_cameraPosition, m_cameraPosition + m_cameraDirection, glm::vec3(0, 1, 0));
}
