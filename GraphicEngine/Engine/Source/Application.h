#pragma once

#include "ResourceLoader/GraphicsData.h"
#include "ResourceLoader/GraphicsFileLoader.h"
#include "Camera/Camera.h"
#include "Framework/Framework.h"
#include <vector>
#include <queue>
#include <memory>

class GraphicObject;

class Application : public GAP311::VulkanApp
{
	// Static
	static Application* s_pApp;

	float m_theta;

	std::vector<GAP311::PipelineObjects> m_pipelines;
	std::vector<std::shared_ptr<GraphicObject>> m_objects;
	std::vector<int> m_renderingPriority;	// value == index of graphic object

	GraphicsFileLoader m_graphicLoader;
	Camera m_camera;
	Uniforms m_uniforms;

public:
	Application();
	~Application();
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;
	Application(Application&&) = delete;
	Application& operator=(Application&&) = delete;

	static Application* Get();

	bool OnInitialize();
	void OnShutDown();

	void OnUpdate(float frameTime) final override;
	bool OnDeviceReady() final override;
	void OnDeviceLost() final override;
	void OnPreRender(vk::CommandBuffer& cb) final override;
	void OnRender(vk::CommandBuffer& cb) final override;

	bool AddGraphicObject(std::shared_ptr<GraphicObject> object, const GAP311::PipelineDescription&);

private:
	bool CreateSceneResources();
	void ReorderRenderingPriority();

	void UpdateInput(float frameTime);
};