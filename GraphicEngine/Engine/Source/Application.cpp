#include "Application.h"
#include "Object/GraphicObject.h"
#include "Object/GeometricShapes/Cube.h"
#include "Object/GeometricShapes/Square.h"
#include "Components/FloatingComponent.h"
#include "Components/SpinningComponent.h"
#include "Components/SatelliteComponent.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <stdio.h>
#include <algorithm>

Application* Application::s_pApp = nullptr;

Application::Application()
	: m_uniforms()
	, m_camera(glm::vec3(0, 10.0f, -200.0f), glm::vec3(0, 0, 1.0f))
	, m_theta(0)
{
	s_pApp = this;

	m_objects.reserve(1000);
	m_pipelines.reserve(1000);
	m_renderingPriority.reserve(1000);
}

Application::~Application()
{
}

Application* Application::Get()
{
	return s_pApp;
}

bool Application::OnInitialize()
{
	if (!CreateSceneResources())
	{
		printf("Failed to initialize graphics scene!\n");
		return false;
	}

	return true;
}

void Application::OnShutDown()
{

}

void Application::OnUpdate(float frameTime)
{
	UpdateInput(frameTime);

	m_camera.Update();
	m_uniforms.viewMatrix = m_camera.ViewMatrix();

	for (size_t i = 0; i < m_objects.size(); ++i)
	{
		m_objects[i]->Update(frameTime);
	}

	// Reorder rendering priority
	ReorderRenderingPriority();
}

bool Application::OnDeviceReady()
{
	// Add Sun
	GAP311::PipelineDescription descSun;
	descSun.vertexAttributes.push_back({ 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos) });
	descSun.vertexAttributes.push_back({ 1, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) });
	descSun.vertexStride = sizeof(Vertex);
	descSun.vertexShaderFilename = "Shaders/simple.vert.spv";
	descSun.fragmentShaderFilename = "Shaders/simple.frag.spv";
	descSun.uniformBuffers.push_back({ 0, sizeof(Uniforms) });
	descSun.uniformBuffers.push_back({ 1, sizeof(ObjectUniforms) });
	descSun.wireframeMode = false;

	std::vector<Vertex> sunVertices;
	std::vector<uint32_t> sunIndices;
	if (!m_graphicLoader.LoadOBJFile("TestFiles/SolarSystem/sun.obj", sunVertices, sunIndices))
	{
		return Error("Failed to load sun obj file.");
	}

	if (!AddGraphicObject(std::make_shared<GraphicObject>(glm::vec3(), std::move(sunVertices), std::move(sunIndices)), descSun))
	{
		return Error("Failed to create sun object.");
	}

	auto& pSun = m_objects.back();
	pSun->SetMaterialEmissive({ 1.0f, 0.2f, 0.0f, 1.0f });
	pSun->SetMaterialDiffuse({ 0.4f, 0.4f, 0.4f, 1.0f });
	pSun->SetMaterialAmbient({ 0.1f, 0.1f, 0.1f, 1.0f });
	pSun->SetMaterialSpecular({ 0.2f, 0.2f, 0.2f, 1.0f });
	pSun->SetMaterialShininess(100.f);
	pSun->AddComponent(std::make_unique<SpinningComponent>(m_objects.back(), glm::vec3(0.f, 1.f, 0.f), 0.005f));
	pSun->DisableLighting();

	// Add Mercury
	GAP311::PipelineDescription descMercury;
	descMercury.vertexAttributes.push_back({ 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos) });
	descMercury.vertexAttributes.push_back({ 1, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) });
	descMercury.vertexStride = sizeof(Vertex);
	descMercury.vertexShaderFilename = "Shaders/simple.vert.spv";
	descMercury.fragmentShaderFilename = "Shaders/simple.frag.spv";
	descMercury.uniformBuffers.push_back({ 0, sizeof(Uniforms) });
	descMercury.uniformBuffers.push_back({ 1, sizeof(ObjectUniforms) });
	descMercury.wireframeMode = false;

	std::vector<Vertex> mercuryVertices;
	std::vector<uint32_t> mercuryIndices;
	if (!m_graphicLoader.LoadOBJFile("TestFiles/SolarSystem/mercury.obj", mercuryVertices, mercuryIndices))
	{
		return Error("Failed to load mercury obj file.");
	}

	if (!AddGraphicObject(std::make_shared<GraphicObject>(glm::vec3(100.f, 0.f, 0.f), std::move(mercuryVertices), std::move(mercuryIndices)), descMercury))
	{
		return Error("Failed to create mercury object.");
	}

	m_objects.back()->SetMaterialDiffuse({ 0.525f, 0.35f, 0.18f, 1.0f });
	m_objects.back()->SetMaterialAmbient({ 0.1f, 0.1f, 0.1f, 1.0f });
	m_objects.back()->SetMaterialSpecular({ 0.2f, 0.2f, 0.2f, 1.0f });
	m_objects.back()->SetMaterialShininess(32.f);
	m_objects.back()->AddComponent(std::make_unique<SpinningComponent>(m_objects.back(), glm::vec3(0.f, 1.f, 0.f), 0.003f));
	m_objects.back()->AddComponent(std::make_unique<SatelliteComponent>(m_objects.back(), pSun, 0.02f));
	m_objects.back()->EnableLighting();

	// Add Venus
	GAP311::PipelineDescription descVenus;
	descVenus.vertexAttributes.push_back({ 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos) });
	descVenus.vertexAttributes.push_back({ 1, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) });
	descVenus.vertexStride = sizeof(Vertex);
	descVenus.vertexShaderFilename = "Shaders/simple.vert.spv";
	descVenus.fragmentShaderFilename = "Shaders/simple.frag.spv";
	descVenus.uniformBuffers.push_back({ 0, sizeof(Uniforms) });
	descVenus.uniformBuffers.push_back({ 1, sizeof(ObjectUniforms) });
	descVenus.wireframeMode = false;

	std::vector<Vertex> venusVertices;
	std::vector<uint32_t> venusIndices;
	if (!m_graphicLoader.LoadOBJFile("TestFiles/SolarSystem/venus.obj", venusVertices, venusIndices))
	{
		return Error("Failed to load venus obj file.");
	}

	if (!AddGraphicObject(std::make_shared<GraphicObject>(glm::vec3(200.f, 0.f, 0.f), std::move(venusVertices), std::move(venusIndices)), descVenus))
	{
		return Error("Failed to create venus object.");
	}

	m_objects.back()->SetMaterialDiffuse({ 1.f, 0.5f, 0.5f, 1.0f });
	m_objects.back()->SetMaterialAmbient({ 0.1f, 0.1f, 0.1f, 1.0f });
	m_objects.back()->SetMaterialSpecular({ 0.2f, 0.2f, 0.2f, 1.0f });
	m_objects.back()->SetMaterialShininess(32.f);
	m_objects.back()->AddComponent(std::make_unique<SpinningComponent>(m_objects.back(), glm::vec3(0.f, 1.f, 0.f), 0.001f));
	m_objects.back()->AddComponent(std::make_unique<SatelliteComponent>(m_objects.back(), pSun, 0.015f));
	m_objects.back()->EnableLighting();

	// Add Earth
	GAP311::PipelineDescription descEarth;
	descEarth.vertexAttributes.push_back({ 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos) });
	descEarth.vertexAttributes.push_back({ 1, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) });
	descEarth.vertexStride = sizeof(Vertex);
	descEarth.vertexShaderFilename = "Shaders/simple.vert.spv";
	descEarth.fragmentShaderFilename = "Shaders/simple.frag.spv";
	descEarth.uniformBuffers.push_back({ 0, sizeof(Uniforms) });
	descEarth.uniformBuffers.push_back({ 1, sizeof(ObjectUniforms) });
	descEarth.wireframeMode = false;

	std::vector<Vertex> earthVertices;
	std::vector<uint32_t> earthIndices;
	if (!m_graphicLoader.LoadOBJFile("TestFiles/SolarSystem/earth.obj", earthVertices, earthIndices))
	{
		return Error("Failed to load earth obj file.");
	}

	if (!AddGraphicObject(std::make_shared<GraphicObject>(glm::vec3(300.f, 0.f, 0.f), std::move(earthVertices), std::move(earthIndices)), descEarth))
	{
		return Error("Failed to create earth object.");
	}

	auto& pEarth = m_objects.back();
	pEarth->SetMaterialDiffuse({ 0.0f, 0.6f, 1.0f, 1.0f });
	pEarth->SetMaterialAmbient({ 0.1f, 0.1f, 0.1f, 1.0f });
	pEarth->SetMaterialSpecular({ 0.2f, 0.2f, 0.2f, 1.0f });
	pEarth->SetMaterialShininess(32.f);
	pEarth->AddComponent(std::make_unique<SpinningComponent>(m_objects.back(), glm::vec3(0.f, 1.f, 0.f), 0.015f));
	pEarth->AddComponent(std::make_unique<SatelliteComponent>(m_objects.back(), pSun, 0.012f));
	pEarth->EnableLighting();

	// Add Moon
	GAP311::PipelineDescription descMoon;
	descMoon.vertexAttributes.push_back({ 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos) });
	descMoon.vertexAttributes.push_back({ 1, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) });
	descMoon.vertexStride = sizeof(Vertex);
	descMoon.vertexShaderFilename = "Shaders/simple.vert.spv";
	descMoon.fragmentShaderFilename = "Shaders/simple.frag.spv";
	descMoon.uniformBuffers.push_back({ 0, sizeof(Uniforms) });
	descMoon.uniformBuffers.push_back({ 1, sizeof(ObjectUniforms) });
	descMoon.wireframeMode = false;

	std::vector<Vertex> moonVertices;
	std::vector<uint32_t> moonIndices;
	if (!m_graphicLoader.LoadOBJFile("TestFiles/SolarSystem/moon.obj", moonVertices, moonIndices))
	{
		return Error("Failed to load moon obj file.");
	}

	if (!AddGraphicObject(std::make_shared<GraphicObject>(glm::vec3(303.f, 0.f, 0.f), std::move(moonVertices), std::move(moonIndices)), descMoon))
	{
		return Error("Failed to create moon object.");
	}

	m_objects.back()->SetMaterialDiffuse({ 0.8f, 0.8f, 0.8f, 1.0f });
	m_objects.back()->SetMaterialAmbient({ 0.1f, 0.1f, 0.1f, 1.0f });
	m_objects.back()->SetMaterialSpecular({ 0.2f, 0.2f, 0.2f, 1.0f });
	m_objects.back()->SetMaterialShininess(32.f);
	m_objects.back()->AddComponent(std::make_unique<SpinningComponent>(m_objects.back(), glm::vec3(0.f, 1.f, 0.f), 0.01f));
	m_objects.back()->AddComponent(std::make_unique<SatelliteComponent>(m_objects.back(), pEarth, 0.1f));
	m_objects.back()->EnableLighting();

	pEarth->AddChild(m_objects.back());

	// Add Mars
	GAP311::PipelineDescription descMars;
	descMars.vertexAttributes.push_back({ 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos) });
	descMars.vertexAttributes.push_back({ 1, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) });
	descMars.vertexStride = sizeof(Vertex);
	descMars.vertexShaderFilename = "Shaders/simple.vert.spv";
	descMars.fragmentShaderFilename = "Shaders/simple.frag.spv";
	descMars.uniformBuffers.push_back({ 0, sizeof(Uniforms) });
	descMars.uniformBuffers.push_back({ 1, sizeof(ObjectUniforms) });
	descMars.wireframeMode = false;

	std::vector<Vertex> marsVertices;
	std::vector<uint32_t> marsIndices;
	if (!m_graphicLoader.LoadOBJFile("TestFiles/SolarSystem/mars.obj", marsVertices, marsIndices))
	{
		return Error("Failed to load mars obj file.");
	}

	if (!AddGraphicObject(std::make_shared<GraphicObject>(glm::vec3(500.f, 0.f, 0.f), std::move(marsVertices), std::move(marsIndices)), descMars))
	{
		return Error("Failed to create mars object.");
	}

	m_objects.back()->SetMaterialDiffuse({ 1.0f, 0.0f, 0.0f, 1.0f });
	m_objects.back()->SetMaterialAmbient({ 0.1f, 0.1f, 0.1f, 1.0f });
	m_objects.back()->SetMaterialSpecular({ 0.2f, 0.2f, 0.2f, 1.0f });
	m_objects.back()->SetMaterialShininess(32.f);
	m_objects.back()->AddComponent(std::make_unique<SpinningComponent>(m_objects.back(), glm::vec3(0.f, 1.f, 0.f), 0.015f));
	m_objects.back()->AddComponent(std::make_unique<SatelliteComponent>(m_objects.back(), pSun, 0.01f));
	m_objects.back()->EnableLighting();

	// Add Jupiter
	GAP311::PipelineDescription descJupiter;
	descJupiter.vertexAttributes.push_back({ 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos) });
	descJupiter.vertexAttributes.push_back({ 1, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) });
	descJupiter.vertexStride = sizeof(Vertex);
	descJupiter.vertexShaderFilename = "Shaders/simple.vert.spv";
	descJupiter.fragmentShaderFilename = "Shaders/simple.frag.spv";
	descJupiter.uniformBuffers.push_back({ 0, sizeof(Uniforms) });
	descJupiter.uniformBuffers.push_back({ 1, sizeof(ObjectUniforms) });
	descJupiter.wireframeMode = false;

	std::vector<Vertex> jupiterVertices;
	std::vector<uint32_t> jupiterIndices;
	if (!m_graphicLoader.LoadOBJFile("TestFiles/SolarSystem/jupiter.obj", jupiterVertices, jupiterIndices))
	{
		return Error("Failed to load jupiter obj file.");
	}

	if (!AddGraphicObject(std::make_shared<GraphicObject>(glm::vec3(800.f, 0.f, 0.f), std::move(jupiterVertices), std::move(jupiterIndices)), descJupiter))
	{
		return Error("Failed to create jupiter object.");
	}

	m_objects.back()->SetMaterialDiffuse({ 1.0f, 0.8f, 0.6f, 1.0f });
	m_objects.back()->SetMaterialAmbient({ 0.1f, 0.1f, 0.1f, 1.0f });
	m_objects.back()->SetMaterialSpecular({ 0.2f, 0.2f, 0.2f, 1.0f });
	m_objects.back()->SetMaterialShininess(32.f);
	m_objects.back()->AddComponent(std::make_unique<SpinningComponent>(m_objects.back(), glm::vec3(0.f, 1.f, 0.f), 0.02f));
	m_objects.back()->AddComponent(std::make_unique<SatelliteComponent>(m_objects.back(), pSun, 0.005f));
	m_objects.back()->EnableLighting();

	// Add Saturn
	GAP311::PipelineDescription descSaturn;
	descSaturn.vertexAttributes.push_back({ 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos) });
	descSaturn.vertexAttributes.push_back({ 1, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) });
	descSaturn.vertexStride = sizeof(Vertex);
	descSaturn.vertexShaderFilename = "Shaders/simple.vert.spv";
	descSaturn.fragmentShaderFilename = "Shaders/simple.frag.spv";
	descSaturn.uniformBuffers.push_back({ 0, sizeof(Uniforms) });
	descSaturn.uniformBuffers.push_back({ 1, sizeof(ObjectUniforms) });
	descSaturn.wireframeMode = false;

	std::vector<Vertex> saturnVertices;
	std::vector<uint32_t> saturnIndices;
	if (!m_graphicLoader.LoadOBJFile("TestFiles/SolarSystem/saturn.obj", saturnVertices, saturnIndices))
	{
		return Error("Failed to load saturn obj file.");
	}

	if (!AddGraphicObject(std::make_shared<GraphicObject>(glm::vec3(1100.f, 0.f, 0.f), std::move(saturnVertices), std::move(saturnIndices)), descSaturn))
	{
		return Error("Failed to create saturn object.");
	}

	m_objects.back()->SetMaterialDiffuse({ 1.0f, 1.0f, 0.6f, 1.0f });
	m_objects.back()->SetMaterialAmbient({ 0.1f, 0.1f, 0.1f, 1.0f });
	m_objects.back()->SetMaterialSpecular({ 0.2f, 0.2f, 0.2f, 1.0f });
	m_objects.back()->SetMaterialShininess(32.f);
	m_objects.back()->AddComponent(std::make_unique<SpinningComponent>(m_objects.back(), glm::vec3(0.f, 1.f, 0.f), 0.019f));
	m_objects.back()->AddComponent(std::make_unique<SatelliteComponent>(m_objects.back(), pSun, 0.002f));
	m_objects.back()->EnableLighting();

	// Add Uranus
	GAP311::PipelineDescription descUranus;
	descUranus.vertexAttributes.push_back({ 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos) });
	descUranus.vertexAttributes.push_back({ 1, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) });
	descUranus.vertexStride = sizeof(Vertex);
	descUranus.vertexShaderFilename = "Shaders/simple.vert.spv";
	descUranus.fragmentShaderFilename = "Shaders/simple.frag.spv";
	descUranus.uniformBuffers.push_back({ 0, sizeof(Uniforms) });
	descUranus.uniformBuffers.push_back({ 1, sizeof(ObjectUniforms) });
	descUranus.wireframeMode = false;

	std::vector<Vertex> uranusVertices;
	std::vector<uint32_t> uranusIndices;
	if (!m_graphicLoader.LoadOBJFile("TestFiles/SolarSystem/uranus.obj", uranusVertices, uranusIndices))
	{
		return Error("Failed to load uranus obj file.");
	}

	if (!AddGraphicObject(std::make_shared<GraphicObject>(glm::vec3(1500.f, 0.f, 0.f), std::move(uranusVertices), std::move(uranusIndices)), descUranus))
	{
		return Error("Failed to create uranus object.");
	}

	m_objects.back()->SetMaterialDiffuse({ 0.0f, 0.4f, 1.0f, 1.0f });
	m_objects.back()->SetMaterialAmbient({ 0.1f, 0.1f, 0.1f, 1.0f });
	m_objects.back()->SetMaterialSpecular({ 0.2f, 0.2f, 0.2f, 1.0f });
	m_objects.back()->SetMaterialShininess(32.f);
	m_objects.back()->AddComponent(std::make_unique<SpinningComponent>(m_objects.back(), glm::vec3(0.f, 1.f, 0.f), 0.016f));
	m_objects.back()->AddComponent(std::make_unique<SatelliteComponent>(m_objects.back(), pSun, 0.001f));
	m_objects.back()->EnableLighting();

	// Add Neptune
	GAP311::PipelineDescription descNeptune;
	descNeptune.vertexAttributes.push_back({ 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos) });
	descNeptune.vertexAttributes.push_back({ 1, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) });
	descNeptune.vertexStride = sizeof(Vertex);
	descNeptune.vertexShaderFilename = "Shaders/simple.vert.spv";
	descNeptune.fragmentShaderFilename = "Shaders/simple.frag.spv";
	descNeptune.uniformBuffers.push_back({ 0, sizeof(Uniforms) });
	descNeptune.uniformBuffers.push_back({ 1, sizeof(ObjectUniforms) });
	descNeptune.wireframeMode = false;

	std::vector<Vertex> neptuneVertices;
	std::vector<uint32_t> neptuneIndices;
	if (!m_graphicLoader.LoadOBJFile("TestFiles/SolarSystem/neptune.obj", neptuneVertices, neptuneIndices))
	{
		return Error("Failed to load neptune obj file.");
	}

	if (!AddGraphicObject(std::make_shared<GraphicObject>(glm::vec3(1900.f, 0.f, 0.f), std::move(neptuneVertices), std::move(neptuneIndices)), descNeptune))
	{
		return Error("Failed to create neptune object.");
	}

	m_objects.back()->SetMaterialDiffuse({ 0.0f, 0.2f, 0.6f, 1.0f });
	m_objects.back()->SetMaterialAmbient({ 0.1f, 0.1f, 0.1f, 1.0f });
	m_objects.back()->SetMaterialSpecular({ 0.2f, 0.2f, 0.2f, 1.0f });
	m_objects.back()->SetMaterialShininess(32.f);
	m_objects.back()->AddComponent(std::make_unique<SpinningComponent>(m_objects.back(), glm::vec3(0.f, 1.f, 0.f), 0.017f));
	m_objects.back()->AddComponent(std::make_unique<SatelliteComponent>(m_objects.back(), pSun, 0.0005f));
	m_objects.back()->EnableLighting();

	return true;
}

void Application::OnDeviceLost()
{
	auto device = GetDevice();

	for (int i = 0; i < m_pipelines.size(); ++i)
	{
		DestroyPipeline(m_pipelines[i]);
		if (m_objects[i]->VertexBuffer())       device.destroyBuffer(m_objects[i]->VertexBuffer());
		if (m_objects[i]->VertexBufferMemory()) device.freeMemory(m_objects[i]->VertexBufferMemory());
		if (m_objects[i]->IndexBuffer())        device.destroyBuffer(m_objects[i]->IndexBuffer());
		if (m_objects[i]->IndexBufferMemory())  device.freeMemory(m_objects[i]->IndexBufferMemory());
	}
}

void Application::OnPreRender(vk::CommandBuffer& cb)
{
	for (int i = 0; i < m_renderingPriority.size(); ++i)
	{
		int index = m_renderingPriority[i];
		cb.updateBuffer(m_pipelines[index].uniformBuffers[0].buffer, 0, sizeof(m_uniforms), &m_uniforms);
	}

	for (int i = 0; i < m_renderingPriority.size(); ++i)
	{
		int index = m_renderingPriority[i];
		cb.updateBuffer(m_pipelines[index].uniformBuffers[1].buffer, 0, sizeof(m_objects[index]->m_objectUniform), &m_objects[index]->m_objectUniform);
	}
}

void Application::OnRender(vk::CommandBuffer& cb)
{
	for (int i = 0; i < m_renderingPriority.size(); ++i)
	{
		int index = m_renderingPriority[i];
		m_pipelines[index].Bind(cb);
		m_objects[index]->Draw(cb);
	}
}

bool Application::AddGraphicObject(std::shared_ptr<GraphicObject> object, const GAP311::PipelineDescription& desc)
{
	m_pipelines.emplace_back(GAP311::PipelineObjects());
	if (!CreatePipeline(desc, m_pipelines.back()))
	{
		m_pipelines.pop_back();
		return Error("Failed to create pipeline objects.");
	}

	size_t index = m_objects.size();
	m_objects.emplace_back(std::move(object));
	
	if (!CreateVertexBuffer(m_objects[index]->Vertices(), m_objects[index]->VertexBuffer(), m_objects[index]->VertexBufferMemory()))
	{
		m_objects.pop_back();
		return Error("Failed to create vertex buffer.");
	}

	if (m_objects[index]->Indices().size() > 0 &&
		!CreateIndexBuffer(m_objects[index]->Indices(), m_objects[index]->IndexBuffer(), m_objects[index]->IndexBufferMemory()))
	{
		m_objects.pop_back();
		return Error("Failed to create index buffer.");
	}

	m_renderingPriority.emplace_back((int)index);

	return true;
}

bool Application::CreateSceneResources()
{
	m_camera.SetPerspectiveView(90.0f, GetWindowWidth() * 1.0f, GetWindowHeight() * 1.0f, 0.1f, 1000.0f);

	m_uniforms.lightPosition = glm::vec4(0.0f, 0.0f, 0.0f, 1.f);
	m_uniforms.lightColor = glm::vec4(1.0f, 1.0f, 0.8f, 0.4f);
	m_uniforms.viewMatrix = m_camera.ViewMatrix();
	m_uniforms.projMatrix = m_camera.ProjMatrix();
	m_uniforms.projMatrix[1][1] *= -1;

	return true;
}

void Application::ReorderRenderingPriority()
{
	if (m_objects.size() < 2)
		return;

	std::sort(m_renderingPriority.begin(), m_renderingPriority.end(), [this](int a, int b) {

		float objDistToCamera = std::abs(glm::distance(m_objects[a]->Position(), m_camera.Position()));
		float comparedObjDistToCamera = std::abs(glm::distance(m_objects[b]->Position(), m_camera.Position()));

		return objDistToCamera > comparedObjDistToCamera;
	});
}

void Application::UpdateInput(float frameTime)
{
	using GAP311::KeyCode;

	const float cameraSpeed = 40.0f;
	const float rotateSpeed = 0.25f;
	const float mouseSpeedH = 0.25f;
	const float mouseSpeedV = 0.05f;

	if (IsKeyDown(KeyCode::W))
	{
		m_camera.SetPosition(m_camera.Position() + m_camera.Direction() * frameTime * cameraSpeed);
	}
	if (IsKeyDown(KeyCode::S))
	{
		m_camera.SetPosition(m_camera.Position() + m_camera.Direction() * frameTime * -cameraSpeed);
	}
	if (IsKeyDown(KeyCode::A))
	{
		m_camera.SetPosition(m_camera.Position() + glm::cross(m_camera.Direction(), glm::vec3(0, 1, 0)) * frameTime * -cameraSpeed);
	}
	if (IsKeyDown(KeyCode::D))
	{
		m_camera.SetPosition(m_camera.Position() + glm::cross(m_camera.Direction(), glm::vec3(0, 1, 0)) * frameTime * cameraSpeed);
	}
	if (IsKeyDown(KeyCode::Q))
	{
		m_camera.SetYaw(m_camera.Yaw() + glm::two_pi<float>() * frameTime * rotateSpeed);
		//m_cameraDirection = glm::rotateY(m_cameraDirection, glm::two_pi<float>() * frameTime * rotateSpeed);
	}
	if (IsKeyDown(KeyCode::E))
	{
		m_camera.SetYaw(m_camera.Yaw() + glm::two_pi<float>() * frameTime * -rotateSpeed);
		//m_cameraDirection = glm::rotateY(m_cameraDirection, glm::two_pi<float>() * frameTime * -rotateSpeed);
	}

	if (IsKeyDown(KeyCode::T))
	{
		m_objects[1]->ChangePosition(glm::vec3(0, frameTime * 0.5f, 0));
	}
	if (IsKeyDown(KeyCode::G))
	{
		m_objects[1]->ChangePosition(glm::vec3(0, frameTime * -0.5f, 0));
	}

	if (IsKeyDown(KeyCode::Space))
	{
		vk::Offset2D mouseDelta = GetFramework()->GetMouseDelta();

		if (mouseDelta.x != 0)
		{
			m_camera.SetPitch(m_camera.Pitch() + glm::two_pi<float>() * frameTime * mouseDelta.x * mouseSpeedH);
			//m_cameraDirection = glm::rotateY(m_cameraDirection, glm::two_pi<float>() * frameTime * mouseDelta.x * mouseSpeedH);
		}

		if (mouseDelta.y != 0)
		{
			m_camera.SetPitch(m_camera.Pitch() + glm::two_pi<float>() * frameTime * mouseDelta.x * mouseSpeedV);
			//m_cameraDirection = glm::rotateX(m_cameraDirection, glm::two_pi<float>() * frameTime * mouseDelta.y * -mouseSpeedV);
		}
	}
}
