#pragma once

#include <memory>

#include <vulkan/vulkan.hpp>
#include <VkBootstrap.h>

#if __has_include(<SDL_vulkan.h>)
#   define GAP311_ENABLE_SDL
#endif
#if __has_include(<GLFW/glfw3.h>)
#   define GAP311_ENABLE_GLFW
#endif
#if __has_include(<SFML/Window.hpp>)
#   define GAP311_ENABLE_SFML
#endif

namespace GAP311
{
    enum class FrameworkType
    {
        eNone,
#if defined(GAP311_ENABLE_SDL)
        eSDL,
#endif
#if defined(GAP311_ENABLE_GLFW)
        eGLFW,
#endif
#if defined(GAP311_ENABLE_SFML)
        eSFML,
#endif
    };

    enum class KeyCode
    {
        Unknown = 0,
        A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
        Left, Right, Up, Down, Enter, Space,
    };

    class IFramework
    {
    public:
        virtual bool Initialize(int32_t windowWidth, int32_t windowHeight) = 0;
        virtual void Shutdown() = 0;
        virtual void PumpEvents() = 0;

        virtual vk::SurfaceKHR CreateWindowSurface(vk::Instance instance) = 0;

        virtual void Log(const char* pMessage) = 0;

        virtual void RequestQuit() = 0;
        virtual bool IsQuitRequested() const = 0;

        virtual float GetFrameTime() const = 0;

        virtual vk::Offset2D GetMousePosition() const = 0;
        virtual vk::Offset2D GetMouseDelta() const = 0;
        virtual bool IsKeyDown(KeyCode keyCode) const = 0;
    };

    struct PipelineDescription;
    struct PipelineObjects;

    /// This class is intended to be used as a base class for demo applications in Vulkan
    /// It will abstract setup and shutdown of Vulkan and provides helpers for managing
    /// Vulkan objects
    class VulkanApp
    {
    public:
        bool Initialize(int32_t windowWidth, int32_t windowHeight, FrameworkType frameworkType);
        void Shutdown();

        void Run();

    protected:
        /// Perform any general initialization logic
        virtual bool OnInitialize() { return true; }
        /// Perform any shutdown logic
        virtual void OnShutdown() {}
        /// Per-frame tick update
        virtual void OnUpdate(float frameTime) {}
        /// Called when the Vulkan device has been fully initialized
        /// and is ready to service requests like creating resources
        virtual bool OnDeviceReady() { return true; }
        /// Called before the Vulkan device needs to be reconfigured
        /// usually as a result of the window being resized.
        /// Free any resources here, especially those which were created
        /// during OnDeviceReady
        virtual void OnDeviceLost() {}
        /// Record commands to be executed before the window's render pass
        virtual void OnPreRender(vk::CommandBuffer& cb) {}
        /// Record commands for rendering a frame
        virtual void OnRender(vk::CommandBuffer& cb) {}
        /// Callback for whenever an error occurs, by default will
        /// output a message to the framework's logging channel
        virtual void OnError(const char* pMessage);
        /// Helper to log a formatted error message
        bool Error(const char* pFormat, ...);

        /// Return references to Vulkan C++ objects
        vk::Instance GetInstance() const { return m_vkbInstance.instance; }
        vk::Device   GetDevice() const { return m_vkbDevice.device; }

        int32_t GetWindowWidth() const { return m_windowWidth; }
        int32_t GetWindowHeight() const { return m_windowHeight; }

        IFramework* GetFramework() const { return m_pFramework.get(); }

        bool IsKeyDown(KeyCode key) const { return m_pFramework->IsKeyDown(key); }

        int32_t FindMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags flags) const;

        bool CreatePipeline(const PipelineDescription& desc, PipelineObjects& obj);
        void DestroyPipeline(PipelineObjects& obj);

        template <typename V>
        bool CreateVertexBuffer(const std::vector<V>& vertices, vk::Buffer& buffer, vk::DeviceMemory& memory)
        {
            return CreateVertexBuffer(vertices.data(), sizeof(V) * vertices.size(), buffer, memory);
        }

        bool CreateVertexBuffer(const void* pData, vk::DeviceSize dataSize, vk::Buffer& buffer, vk::DeviceMemory& memory);

        bool CreateIndexBuffer(const std::vector<uint32_t>& indices, vk::Buffer& buffer, vk::DeviceMemory& memory);

        bool CreateUniformBuffer(vk::DeviceSize dataSize, vk::Buffer& buffer, vk::DeviceMemory& memory);

        /// Returns the RenderPass which will be used to render into the window's backbuffer
        vk::RenderPass GetWindowRenderPass() const { return m_vkWindowRenderPass; }

        /// Returns a structure representing the viewport of the window
        vk::Viewport GetViewport();

        /// Helper to load a SPIR-V shader file as a ShaderModule
        vk::ShaderModule LoadShaderModule(const char* pFilename);

    private: // Vulkan specific functionality
        bool InitializeVulkan();
        void ShutdownVulkan();
        bool RebuildSwapchain();
        void DestroyFramebuffers();
        void RenderFrame();
        void BeginWindowRenderPass(vk::CommandBuffer& cb);
        void EndWindowRenderPass(vk::CommandBuffer& cb);

        int32_t m_windowWidth = 0;
        int32_t m_windowHeight = 0;
        vkb::Instance m_vkbInstance;
        vkb::Device m_vkbDevice;
        vk::Queue m_vkGraphicsQueue;
        vk::Queue m_vkPresentQueue;
        vk::SurfaceKHR m_vkWindowSurface;
        vkb::Swapchain m_vkbSwapchain;
        vk::RenderPass m_vkWindowRenderPass;

        vk::CommandPool m_vkGraphicsCommandPool;
        vk::DescriptorPool m_vkDescriptorPool;

        struct FramebufferData
        {
            vk::Framebuffer framebuffer;
            vk::ImageView imageView;
            vk::CommandBuffer commandBuffer;
        };
        std::vector<FramebufferData> m_framebuffers;
        size_t m_targetFramebufferIndex = 0;

        struct FrameData
        {
            vk::Semaphore semaphoreImageAvailable;
            vk::Semaphore semaphoreFinished;
            vk::Fence fenceInFlight;
        };
        std::vector<FrameData> m_frames;
        size_t m_currentFrameIndex = 0;

    private: // Framework specific functionality
        std::unique_ptr<IFramework> m_pFramework;
    };

    /// This structure abstracts the configuration of the graphics pipeline
    /// from a simple rendering perspective.
    /// 
    /// In Vulkan, when you are building a Pipeline object, you are building a description
    /// of how the different rendering components and hardware should work together
    /// to generate output. Think of it as you're stringing along the components of an
    /// assembly line.
    /// 
    /// Typically, an assembly line starts with one or more inputs that are lined up.
    /// This stream of input flows along through a number of stages to be transformed,
    /// reconfigured, or otherwise manipulated at each stage.  Along the way each stage
    /// may require additional resources.  Eventually the input all arrives at the other
    /// end in its final form.
    /// 
    /// Note that this description only describes the form the pipeline should take, it does
    /// not provide the input data.  Data will be provided through binding commands written
    /// before the draw command which will perform the execution of the pipeline.
    /// 
    /// This is a loose abstraction of the vk::Pipeline*CreateInfo and vk::DescriptorSet* classes.
    /// 
    struct PipelineDescription
    {
        /// Describes an attribute of a vertex, which is an input variable to a vertex shader
        /// e.g.  layout(location = ) in vec3 position;
        struct VertexAttribute
        {
            /// The location number specified in the shader
            /// e.g.  layout(location = 0)
            uint32_t location;
            /// The data format of the input object
            /// This defines the size and number of components in each attribute
            /// e.g. vec3 could be vk::Format::eR32G32B32Sfloat
            vk::Format format;
            /// Offset in memory from the start of a vertex for where the attribute data should be read
            /// This is usually in the range of 0 to vertexStride.
            /// e.g. This could be the offsetof() the member variable in a vertex struct
            uint32_t offset;
        };
        std::vector<VertexAttribute> vertexAttributes;

        /// Distance between vertex objects in the vertex array buffer (size of a vertex)
        uint32_t vertexStride = 0;

        /// Describes a shader Uniform (also known as constant) which is an input variable
        /// to a shader that is shared between all shader invocations for that draw call
        struct UniformBuffer
        {
            /// The layout binding number in the shader
            /// e.g.  layout(binding = 0)
            uint32_t binding;
            /// Size in bytes of the buffer
            size_t byteSize;
        };
        std::vector<UniformBuffer> uniformBuffers;

        struct UniformImage
        {
            uint32_t binding;
            size_t byteSize;
        };
        std::vector<UniformImage> uniformImages;

        /// Shader stages
        std::string vertexShaderFilename;
        std::string fragmentShaderFilename;
        std::string geometryShaderFilename;

        /// Draw lines instead of filled geometry
        bool wireframeMode = false;
    };

    struct PipelineObjects
    {
        vk::Pipeline pipeline;
        vk::PipelineLayout pipelineLayout;
        vk::DescriptorSet descriptorSet;
        vk::DescriptorSetLayout descriptorSetLayout;

        struct UniformBuffer
        {
            uint32_t binding;
            vk::Buffer buffer;
            vk::DeviceMemory bufferMemory;
        };
        std::vector<UniformBuffer> uniformBuffers;

        void Bind(vk::CommandBuffer& cb)
        {
            cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
            cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
        }
    };
}