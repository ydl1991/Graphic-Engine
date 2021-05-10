#include "Framework.h"

#include <vector>
#include <cstdarg>
#include <fstream>

#include <VkBootstrap.h>

#pragma warning(disable: 4834) // allow ignoring nodiscard

namespace GAP311
{

#if defined(GAP311_ENABLE_SDL)
IFramework* FrameworkCreateSDL();
#endif
#if defined(GAP311_ENABLE_GLFW)
IFramework* FrameworkCreateGLFW();
#endif
#if defined(GAP311_ENABLE_SFML)
IFramework* FrameworkCreateSFML();
#endif

bool VulkanApp::Initialize(int32_t width, int32_t height, FrameworkType frameworkType)
{
#if defined(GAP311_ENABLE_SDL)
    if (frameworkType == FrameworkType::eSDL)
        m_pFramework.reset(FrameworkCreateSDL());
#endif
#if defined(GAP311_ENABLE_GLFW)
    if (frameworkType == FrameworkType::eGLFW)
        m_pFramework.reset(FrameworkCreateGLFW());
#endif
#if defined(GAP311_ENABLE_SFML)
    if (frameworkType == FrameworkType::eSFML)
        m_pFramework.reset(FrameworkCreateSFML());
#endif

    if (!m_pFramework)
        return false;

    m_windowWidth = width;
    m_windowHeight = height;

    if (!m_pFramework->Initialize(width, height))
        return false;

    if (!InitializeVulkan())
        return false;

    if (!OnInitialize())
        return false;

    return true;
}

void VulkanApp::Shutdown()
{
    OnShutdown();
    ShutdownVulkan();
    if (m_pFramework) m_pFramework->Shutdown();
    m_pFramework = nullptr;
}

void VulkanApp::Run()
{
    while (!m_pFramework->IsQuitRequested())
    {
        m_pFramework->PumpEvents();

        OnUpdate(m_pFramework->GetFrameTime());

        RenderFrame();
    }
}

void VulkanApp::OnError(const char* pMessage)
{
    if (m_pFramework)
    {
        m_pFramework->Log(pMessage);
    }
}

bool VulkanApp::Error(const char* pMessage, ...)
{
    static char buffer[2048];

    __debugbreak();

    va_list args;
    va_start(args, pMessage);
    std::vsnprintf(buffer, _countof(buffer), pMessage, args);
    va_end(args);

    OnError(buffer);
    return false;
}

bool VulkanApp::InitializeVulkan()
{
    // Vulkan must first have an instance created, which will act as the DLL context for the app
    vkb::InstanceBuilder instBuilder;
    auto instResult = instBuilder
        .set_app_name("Vulkan VulkanApp")
        .request_validation_layers()
        .use_default_debug_messenger()
        .build();
    if (!instResult)
        return Error("Failed to create Vulkan instance.");

    m_vkbInstance = instResult.value();

    // Since we're going to be outputting graphics to a window, we need a surface for vulkan to render into

    m_vkWindowSurface = m_pFramework->CreateWindowSurface(m_vkbInstance.instance);
    if (!m_vkWindowSurface)
        return Error("Failed to create window Vulkan surface.");

    // Now that we have a surface we can look for the physical device which can draw to this surface

    vk::PhysicalDeviceFeatures deviceFeatures;
    deviceFeatures.fillModeNonSolid = true; // Require wireframe support

    vkb::PhysicalDeviceSelector selector(m_vkbInstance);
    auto selectResult = selector
        .set_surface(m_vkWindowSurface)
        .set_required_features(deviceFeatures)
        .select();
    if (!selectResult)
        return Error("Failed to choose suitable PhysicalDevice.");

    // Finally, we can create a logical device using the physical device, all our commands will go through the logical device

    vkb::DeviceBuilder deviceBuilder(selectResult.value());
    auto deviceResult = deviceBuilder.build();
    if (!deviceResult)
        return Error("Failed to create Vulkan device.");

    m_vkbDevice = deviceResult.value();

    // Different commands might need to go into different queues, so lets look those up

    auto graphicsResult = m_vkbDevice.get_queue(vkb::QueueType::graphics);
    if (!graphicsResult)
        return Error("Failed to get queue for graphics operations.");

    m_vkGraphicsQueue = graphicsResult.value();

    auto presentResult = m_vkbDevice.get_queue(vkb::QueueType::present);
    if (!presentResult)
        return Error("Failed to get queue for present operations.");

    m_vkPresentQueue = presentResult.value();

    // Now with basic device setup out of the way we need to finish creating the objects
    // that will allow us to issue rendering commands to the window that will be displayed

    vk::Device device(m_vkbDevice.device); // vulkan-hpp helper class

    // One command pool will house all the graphics commands we issue

    vk::CommandPoolCreateInfo commandPoolCreateInfo;
    commandPoolCreateInfo.queueFamilyIndex = m_vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    m_vkGraphicsCommandPool = device.createCommandPool(commandPoolCreateInfo);
    if (!m_vkGraphicsCommandPool)
        return Error("Failed to create command pool.");

    std::vector<vk::DescriptorPoolSize> descriptorPoolSizes;
    descriptorPoolSizes.emplace_back(vk::DescriptorType::eUniformBuffer, 128);

    vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
    descriptorPoolCreateInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    descriptorPoolCreateInfo.maxSets = 256;
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();
    m_vkDescriptorPool = device.createDescriptorPool(descriptorPoolCreateInfo);
    if (!m_vkDescriptorPool)
        return Error("Failed to create descriptor pool.");

    // The objects that handle drawing into the window are dependent upon the swap chain, so lets create those
    // The swap chain and related objects must be recreated when the window is resized, as some dependent
    // resources, such as framebuffers, must get recreated to match the new window size

    if (!RebuildSwapchain())
        return Error("Failed to construct swapchain and framebuffers.");

    // Finally, we need to allocate some synchronization objects to keep our commands ordered, especially
    // since the GPU will execute things at a different rate than the CPU

    m_frames.resize(2); // we'll only allow the CPU to have two frames of rendering commands in progress
    m_currentFrameIndex = 0;
    for (auto& frameData : m_frames)
    {
        // This semaphore will be invoked when the framebuffer image is ready to be used, we cannot
        // begin drawing on the GPU until this occurs
        frameData.semaphoreImageAvailable = device.createSemaphore(vk::SemaphoreCreateInfo());
        if (!frameData.semaphoreImageAvailable)
            return Error("Failed to create semaphore.");

        // This semaphore will be signaled when rendering finishes and should be presented
        frameData.semaphoreFinished = device.createSemaphore(vk::SemaphoreCreateInfo());
        if (!frameData.semaphoreFinished)
            return Error("Failed to create semaphore.");

        // This fence will allow us to wait on the CPU until the frame has finished rendering
        vk::FenceCreateInfo fenceInfo;
        fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
        frameData.fenceInFlight = device.createFence(fenceInfo);
        if (!frameData.fenceInFlight)
            return Error("Failed to create fence.");
    }

    return true;
}

void VulkanApp::ShutdownVulkan()
{
    vk::Device device(m_vkbDevice.device);
    if (device)
    {
        device.waitIdle();

        OnDeviceLost();

        for (auto& frame : m_frames)
        {
            if (frame.semaphoreImageAvailable)
                device.destroySemaphore(frame.semaphoreImageAvailable);
            if (frame.semaphoreFinished)
                device.destroySemaphore(frame.semaphoreFinished);
            if (frame.fenceInFlight)
                device.destroyFence(frame.fenceInFlight);
        }
        m_frames.clear();

        if (m_vkWindowRenderPass)
        {
            device.destroyRenderPass(m_vkWindowRenderPass);
            m_vkWindowRenderPass = nullptr;
        }
    }

    DestroyFramebuffers();

    if (device)
    {
        if (m_vkDescriptorPool)
            device.destroyDescriptorPool(m_vkDescriptorPool);
        if (m_vkGraphicsCommandPool)
            device.destroyCommandPool(m_vkGraphicsCommandPool);
    }

    m_vkPresentQueue = nullptr;
    m_vkGraphicsQueue = nullptr;

    vkb::destroy_swapchain(m_vkbSwapchain);
    vkb::destroy_device(m_vkbDevice);
    if (m_vkWindowSurface)
    {
        vkDestroySurfaceKHR(m_vkbInstance.instance, m_vkWindowSurface, m_vkbInstance.allocation_callbacks);
        m_vkWindowSurface = nullptr;
    }
    vkb::destroy_instance(m_vkbInstance);
}

bool VulkanApp::RebuildSwapchain()
{
    vk::Device device(m_vkbDevice.device);
    device.waitIdle();

    OnDeviceLost();

    DestroyFramebuffers();

    vkb::SwapchainBuilder swapchainBuilder(m_vkbDevice);
    auto swapchainResult = swapchainBuilder
        .set_old_swapchain(m_vkbSwapchain)
        .build();
    if (!swapchainResult)
        return Error("Failed to build swapchain.");

    m_vkbSwapchain = swapchainResult.value();

    // Rebuild window render pass?

    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = vk::Format(m_vkbSwapchain.image_format);
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    vk::SubpassDependency subpassDep;
    subpassDep.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDep.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    subpassDep.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    subpassDep.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo renderpassCreateInfo;
    renderpassCreateInfo.attachmentCount = 1;
    renderpassCreateInfo.pAttachments = &colorAttachment;
    renderpassCreateInfo.subpassCount = 1;
    renderpassCreateInfo.pSubpasses = &subpass;
    renderpassCreateInfo.dependencyCount = 1;
    renderpassCreateInfo.pDependencies = &subpassDep;
    m_vkWindowRenderPass = device.createRenderPass(renderpassCreateInfo);
    if (!m_vkWindowRenderPass)
        return Error("Failed to create default render pass.");

    auto imageViews = m_vkbSwapchain.get_image_views().value();
    if (imageViews.empty())
        return Error("Failed to allocate image views.");

    vk::CommandBufferAllocateInfo cbAllocateInfo;
    cbAllocateInfo.commandPool = m_vkGraphicsCommandPool;
    cbAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
    cbAllocateInfo.commandBufferCount = static_cast<uint32_t>(imageViews.size());

    std::vector<vk::CommandBuffer> cbs = device.allocateCommandBuffers(cbAllocateInfo);
    if (cbs.empty())
        return Error("Failed to allocate command buffers.");

    for (auto imageView : imageViews)
    {
        vk::ImageView attachments[] = { imageView };

        vk::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.renderPass = m_vkWindowRenderPass;
        fbCreateInfo.attachmentCount = 1;
        fbCreateInfo.pAttachments = attachments;
        fbCreateInfo.width = m_vkbSwapchain.extent.width;
        fbCreateInfo.height = m_vkbSwapchain.extent.height;
        fbCreateInfo.layers = 1;

        vk::Framebuffer fb = device.createFramebuffer(fbCreateInfo);
        if (!fb)
            return Error("Failed to create framebuffer.");

        m_framebuffers.push_back({ fb, imageView, cbs.back() });
        cbs.pop_back();
    }

    m_targetFramebufferIndex = 0;

    return OnDeviceReady();
}

void VulkanApp::DestroyFramebuffers()
{
    if (m_vkbSwapchain.swapchain)
    {
        vk::Device device(m_vkbDevice.device);

        std::vector<VkImageView> imageViews;
        imageViews.reserve(m_framebuffers.size());

        std::vector<vk::CommandBuffer> cbs;
        cbs.reserve(m_framebuffers.size());

        for (auto& framebuffer : m_framebuffers)
        {
            device.destroyFramebuffer(framebuffer.framebuffer);
            imageViews.push_back(framebuffer.imageView);
            cbs.push_back(framebuffer.commandBuffer);
        }

        device.freeCommandBuffers(m_vkGraphicsCommandPool, cbs);
        m_framebuffers.clear();
        m_vkbSwapchain.destroy_image_views(imageViews);
    }
}

void VulkanApp::RenderFrame()
{
    auto device = GetDevice();

    auto& currentFrame = m_frames[m_currentFrameIndex];

    // Wait for our the frame to complete before we start changing it
    device.waitForFences(1, &currentFrame.fenceInFlight, true, UINT64_MAX);

    // get next image to render into
    uint32_t imageIndex = 0;
    auto acquireResult = device.acquireNextImageKHR(m_vkbSwapchain.swapchain, UINT64_MAX,
        currentFrame.semaphoreImageAvailable, {}, &imageIndex);
    if (acquireResult == vk::Result::eErrorOutOfDateKHR)
    {
        RebuildSwapchain();
        return;
    }
    m_targetFramebufferIndex = imageIndex;
    // wait for image fence?

    auto& targetFramebuffer = m_framebuffers[imageIndex];

    auto& commandBuffer = targetFramebuffer.commandBuffer;
    commandBuffer.reset();

    vk::CommandBufferBeginInfo commandBufferBeginInfo;
    commandBuffer.begin(&commandBufferBeginInfo);
    OnPreRender(commandBuffer);
    BeginWindowRenderPass(commandBuffer);
    OnRender(commandBuffer);
    EndWindowRenderPass(commandBuffer);
    commandBuffer.end();

    // 
    // Submit
    //

    vk::Semaphore waitSemaphores[] =
    {
        currentFrame.semaphoreImageAvailable,
    };
    vk::PipelineStageFlags waitFlags[] =
    {
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
    };
    vk::Semaphore signalSemaphores[] =
    {
        currentFrame.semaphoreFinished,
    };

    vk::SubmitInfo submitInfo;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitDstStageMask = waitFlags;
    submitInfo.pSignalSemaphores = signalSemaphores;
    submitInfo.signalSemaphoreCount = 1;
    
    vk::CommandBuffer buffers[] = { commandBuffer };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = buffers;

    device.resetFences(1, &currentFrame.fenceInFlight);
    m_vkGraphicsQueue.submit(1, &submitInfo, currentFrame.fenceInFlight);

    // Present
    //
    // The present command should wait until rendering is finished and this is ensured
    // by having it wait until the semaphores bundled with the submit are signaled

    vk::PresentInfoKHR presentInfo;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.waitSemaphoreCount = 1;

    vk::SwapchainKHR swapchains[] = { m_vkbSwapchain.swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    auto presentResult = m_vkPresentQueue.presentKHR(presentInfo);
    if (presentResult == vk::Result::eErrorOutOfDateKHR)
    {
        RebuildSwapchain();
        return;
    }

    m_currentFrameIndex = (m_currentFrameIndex + 1) % m_frames.size();
}

void VulkanApp::BeginWindowRenderPass(vk::CommandBuffer& cb)
{
    vk::Viewport viewport = GetViewport();

    vk::Rect2D scissor;
    scissor.extent = m_vkbSwapchain.extent;

    cb.setViewport(0, 1, &viewport);
    cb.setScissor(0, 1, &scissor);

    vk::RenderPassBeginInfo renderPassInfo;
    renderPassInfo.renderPass = m_vkWindowRenderPass;
    renderPassInfo.framebuffer = m_framebuffers[m_targetFramebufferIndex].framebuffer;
    renderPassInfo.renderArea.setExtent(m_vkbSwapchain.extent);

    vk::ClearValue clearValue;
    clearValue.color.setFloat32({ 0.0f, 0.0f, 0.0f, 1.0f });
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    cb.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
}

void VulkanApp::EndWindowRenderPass(vk::CommandBuffer& cb)
{
    cb.endRenderPass();
}

int32_t VulkanApp::FindMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags flags) const
{
    vk::PhysicalDeviceMemoryProperties props(m_vkbDevice.physical_device.memory_properties);

    for (int32_t i = 0; i != props.memoryTypeCount; ++i)
    {
        if (requirements.memoryTypeBits & (0x1 << i))
        {
            if ((props.memoryTypes[i].propertyFlags & flags) == flags)
            {
                return i;
            }
        }
    }

    return -1;
}

bool VulkanApp::CreatePipeline(const PipelineDescription& desc, PipelineObjects& obj)
{
    auto device = GetDevice();

    vk::GraphicsPipelineCreateInfo pipelineInfo;

    /// Stream (Vertex) Inputs ///
    
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    pipelineInfo.pVertexInputState = &vertexInputInfo;

    vk::VertexInputBindingDescription vertexBindingInfo;
    // It is possible to draw without providing vertices, so only provide binding
    // info if we have vertex attributes specified.
    if (!desc.vertexAttributes.empty())
    {
        // We only support binding one vertex stream with per-vertex data
        vertexBindingInfo.binding = 0;
        vertexBindingInfo.inputRate = vk::VertexInputRate::eVertex;
        vertexBindingInfo.stride = desc.vertexStride;
        vertexInputInfo.pVertexBindingDescriptions = &vertexBindingInfo;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
    }

    std::vector<vk::VertexInputAttributeDescription> vertexAttributeInfos;
    vertexAttributeInfos.reserve(desc.vertexAttributes.size());
    for (auto& attrib : desc.vertexAttributes)
    {
        vk::VertexInputAttributeDescription vertexAttribute;
        vertexAttribute.binding = 0;
        vertexAttribute.location = attrib.location;
        vertexAttribute.format = attrib.format;
        vertexAttribute.offset = attrib.offset;
        vertexAttributeInfos.emplace_back(vertexAttribute);
    }
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeInfos.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeInfos.size());

    // Only support drawing triangles
    vk::PipelineInputAssemblyStateCreateInfo assemblyInputInfo;
    assemblyInputInfo.topology = vk::PrimitiveTopology::eTriangleList;
    pipelineInfo.pInputAssemblyState = &assemblyInputInfo;

    /// Uniform Inputs (descriptors) ///

    std::vector<vk::DescriptorSetLayoutBinding> descriptorSetBindings;
    descriptorSetBindings.reserve(desc.uniformBuffers.size() + desc.uniformImages.size());

    for (auto& uniformBuffer : desc.uniformBuffers)
    {
        vk::DescriptorSetLayoutBinding binding;
        binding.binding = uniformBuffer.binding;
        binding.descriptorType = vk::DescriptorType::eUniformBuffer;
        binding.descriptorCount = 1;
        binding.stageFlags = vk::ShaderStageFlagBits::eAllGraphics;
        descriptorSetBindings.emplace_back(binding);
    }

    for (auto& uniformImage : desc.uniformImages)
    {
        vk::DescriptorSetLayoutBinding binding;
        binding.binding = uniformImage.binding;
        binding.descriptorType = vk::DescriptorType::eSampler;
        binding.descriptorCount = 1;
        binding.stageFlags = vk::ShaderStageFlagBits::eAllGraphics;
        descriptorSetBindings.emplace_back(binding);
    }

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo;
    descriptorSetLayoutInfo.pBindings = descriptorSetBindings.data();
    descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(descriptorSetBindings.size());
    obj.descriptorSetLayout = device.createDescriptorSetLayout(descriptorSetLayoutInfo);
    if (!obj.descriptorSetLayout)
        return Error("Failed to create descriptor set layout.");

    vk::PipelineLayoutCreateInfo layoutInfo;
    layoutInfo.pSetLayouts = &obj.descriptorSetLayout;
    layoutInfo.setLayoutCount = 1;

    pipelineInfo.layout = obj.pipelineLayout = device.createPipelineLayout(layoutInfo);
    if (!obj.pipelineLayout)
        return Error("Failed to create pipeline layout.");

    // Create buffers and associate with the descriptor set

    vk::DescriptorSetAllocateInfo descriptorSetAllocInfo;
    descriptorSetAllocInfo.descriptorPool = m_vkDescriptorPool;
    descriptorSetAllocInfo.descriptorSetCount = 1;
    descriptorSetAllocInfo.pSetLayouts = &obj.descriptorSetLayout;
    auto descriptorSets = device.allocateDescriptorSets(descriptorSetAllocInfo);
    if (descriptorSets.empty())
        return Error("Failed to allocate descriptor set.");
    obj.descriptorSet = descriptorSets[0];

    for (auto& bufferDesc : desc.uniformBuffers)
    {
        obj.uniformBuffers.push_back({});
        PipelineObjects::UniformBuffer& bufferObjects = obj.uniformBuffers.back();
        bufferObjects.binding = bufferDesc.binding;

        vk::BufferCreateInfo bufferInfo;
        bufferInfo.size = bufferDesc.byteSize;
        bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer |
            vk::BufferUsageFlagBits::eTransferDst; // TransferDst means we can copy data TO the buffer
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;
        
        bufferObjects.buffer = device.createBuffer(bufferInfo);
        if (!bufferObjects.buffer)
            return Error("Failed to create uniform buffer.");

        auto bufferMemoryReq = device.getBufferMemoryRequirements(bufferObjects.buffer);

        vk::MemoryPropertyFlags bufferMemoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;

        vk::MemoryAllocateInfo bufferAllocInfo;
        bufferAllocInfo.allocationSize = bufferMemoryReq.size;
        bufferAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(bufferMemoryReq, bufferMemoryFlags);
        bufferObjects.bufferMemory = device.allocateMemory(bufferAllocInfo);
        if (!bufferObjects.bufferMemory)
            return Error("Failed to allocate memory for uniform buffer.");

        device.bindBufferMemory(bufferObjects.buffer, bufferObjects.bufferMemory, 0);

        vk::WriteDescriptorSet update;
        update.dstSet = obj.descriptorSet;
        update.dstBinding = bufferObjects.binding;
        update.dstArrayElement = 0;
        update.descriptorCount = 1;
        update.descriptorType = vk::DescriptorType::eUniformBuffer;

        vk::DescriptorBufferInfo descriptorBufferInfo;
        descriptorBufferInfo.buffer = bufferObjects.buffer;
        descriptorBufferInfo.offset = 0;
        descriptorBufferInfo.range = bufferDesc.byteSize;
        update.pBufferInfo = &descriptorBufferInfo;

        device.updateDescriptorSets({ update }, {});
    }

    /// Shaders ///

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

    vk::PipelineShaderStageCreateInfo vertexShaderStage;
    vertexShaderStage.stage = vk::ShaderStageFlagBits::eVertex;
    vertexShaderStage.module = LoadShaderModule(desc.vertexShaderFilename.c_str());
    vertexShaderStage.pName = "main";
    shaderStages.emplace_back(vertexShaderStage);

    vk::PipelineShaderStageCreateInfo fragmentShaderStage;
    fragmentShaderStage.stage = vk::ShaderStageFlagBits::eFragment;
    fragmentShaderStage.module = LoadShaderModule(desc.fragmentShaderFilename.c_str());
    fragmentShaderStage.pName = "main";
    shaderStages.emplace_back(fragmentShaderStage);

    if (!desc.geometryShaderFilename.empty())
    {
        vk::PipelineShaderStageCreateInfo geometryShaderStage;
        geometryShaderStage.stage = vk::ShaderStageFlagBits::eGeometry;
        geometryShaderStage.module = LoadShaderModule(desc.geometryShaderFilename.c_str());
        geometryShaderStage.pName = "main";
        shaderStages.emplace_back(geometryShaderStage);
    }

    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());

    /// Rasterization ///

    vk::PipelineRasterizationStateCreateInfo rasterizationInfo;
    rasterizationInfo.polygonMode = desc.wireframeMode ? vk::PolygonMode::eLine : vk::PolygonMode::eFill;
    rasterizationInfo.cullMode = desc.wireframeMode ? vk::CullModeFlagBits::eNone : vk::CullModeFlagBits::eBack;
    rasterizationInfo.lineWidth = 1.0f;
    rasterizationInfo.frontFace = vk::FrontFace::eCounterClockwise;
    pipelineInfo.pRasterizationState = &rasterizationInfo;

    vk::PipelineMultisampleStateCreateInfo multisampleInfo;
    multisampleInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
    pipelineInfo.pMultisampleState = &multisampleInfo;

    /// Output and Blending ///
    
    pipelineInfo.renderPass = GetWindowRenderPass();

    vk::PipelineViewportStateCreateInfo viewportInfo;
    pipelineInfo.pViewportState = &viewportInfo;

    vk::Viewport viewport = GetViewport();
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = &viewport;

    vk::Rect2D scissor;
    scissor.extent.setWidth(static_cast<uint32_t>(viewport.width));
    scissor.extent.setHeight(static_cast<uint32_t>(viewport.height));
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = &scissor;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eA |
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB;

    vk::PipelineColorBlendStateCreateInfo blendInfo;
    blendInfo.logicOp = vk::LogicOp::eCopy;
    blendInfo.attachmentCount = 1;
    blendInfo.pAttachments = &colorBlendAttachment;
    pipelineInfo.pColorBlendState = &blendInfo;

    // Create vulkan object

    obj.pipeline = device.createGraphicsPipeline(nullptr, pipelineInfo).value;
    if (!obj.pipeline)
        return Error("Failed to create graphics pipeline.");

    // Now that the pipeline has been created we do not need to hold on to our shader modules
    for (auto& shaderStage : shaderStages)
    {
        if (shaderStage.module)
            device.destroyShaderModule(shaderStage.module);
    }

    return true;
}

void VulkanApp::DestroyPipeline(PipelineObjects& obj)
{
    auto device = GetDevice();

    for (auto& uniformBuffer : obj.uniformBuffers)
    {
        if (uniformBuffer.buffer)       device.destroyBuffer(uniformBuffer.buffer);
        if (uniformBuffer.bufferMemory) device.freeMemory(uniformBuffer.bufferMemory);
    }

    if (obj.descriptorSet)       device.freeDescriptorSets(m_vkDescriptorPool, 1, &obj.descriptorSet);
    if (obj.descriptorSetLayout) device.destroyDescriptorSetLayout(obj.descriptorSetLayout);
    if (obj.pipelineLayout)      device.destroyPipelineLayout(obj.pipelineLayout);
    if (obj.pipeline)            device.destroyPipeline(obj.pipeline);
}

bool VulkanApp::CreateVertexBuffer(const void* pData, vk::DeviceSize dataSize, vk::Buffer& buffer, vk::DeviceMemory& memory)
{
    auto device = GetDevice();

    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = dataSize;
    bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;
    
    buffer = device.createBuffer(bufferInfo);
    if (!buffer)
        return Error("Failed to create vertex buffer.");

    auto bufferMemoryReq = device.getBufferMemoryRequirements(buffer);

    vk::MemoryPropertyFlags bufferMemoryFlags = vk::MemoryPropertyFlagBits::eHostVisible;

    vk::MemoryAllocateInfo bufferAllocInfo;
    bufferAllocInfo.allocationSize = bufferMemoryReq.size;
    bufferAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(bufferMemoryReq, bufferMemoryFlags);
    memory = device.allocateMemory(bufferAllocInfo);
    if (!memory)
        return Error("Failed to allocate memory for vertex buffer.");

    device.bindBufferMemory(buffer, memory, 0);

    // Upload
    void* pBufferData = device.mapMemory(memory, 0, bufferInfo.size);
    memcpy(pBufferData, pData, bufferInfo.size);
    vk::MappedMemoryRange mappedRange;
    mappedRange.memory = memory;
    mappedRange.offset = 0;
    mappedRange.size = VK_WHOLE_SIZE;
    device.flushMappedMemoryRanges(mappedRange);
    device.unmapMemory(memory);

    return true;
}

bool VulkanApp::CreateIndexBuffer(const std::vector<uint32_t>& indicies, vk::Buffer& buffer, vk::DeviceMemory& memory)
{
    auto device = GetDevice();

    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = sizeof(uint32_t) * indicies.size();
    bufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;
    
    buffer = device.createBuffer(bufferInfo);
    if (!buffer)
        return Error("Failed to create index buffer.");

    auto bufferMemoryReq = device.getBufferMemoryRequirements(buffer);

    vk::MemoryPropertyFlags bufferMemoryFlags = vk::MemoryPropertyFlagBits::eHostVisible;

    vk::MemoryAllocateInfo bufferAllocInfo;
    bufferAllocInfo.allocationSize = bufferMemoryReq.size;
    bufferAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(bufferMemoryReq, bufferMemoryFlags);
    memory = device.allocateMemory(bufferAllocInfo);
    if (!memory)
        return Error("Failed to allocate memory for index buffer.");

    device.bindBufferMemory(buffer, memory, 0);

    // Upload
    void* pBufferData = device.mapMemory(memory, 0, bufferInfo.size);
    memcpy(pBufferData, indicies.data(), bufferInfo.size);
    vk::MappedMemoryRange mappedRange;
    mappedRange.memory = memory;
    mappedRange.offset = 0;
    mappedRange.size = VK_WHOLE_SIZE;
    device.flushMappedMemoryRanges(mappedRange);
    device.unmapMemory(memory);

    return true;
}

bool VulkanApp::CreateUniformBuffer(vk::DeviceSize dataSize, vk::Buffer& buffer, vk::DeviceMemory& memory)
{
    auto device = GetDevice();

    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = dataSize;
    bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer |
        vk::BufferUsageFlagBits::eTransferDst; // TransferDst means we can copy data TO the buffer
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;
    
    buffer = device.createBuffer(bufferInfo);
    if (!buffer)
        return Error("Failed to create uniform buffer.");

    auto bufferMemoryReq = device.getBufferMemoryRequirements(buffer);

    vk::MemoryPropertyFlags bufferMemoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;

    vk::MemoryAllocateInfo bufferAllocInfo;
    bufferAllocInfo.allocationSize = bufferMemoryReq.size;
    bufferAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(bufferMemoryReq, bufferMemoryFlags);
    memory = device.allocateMemory(bufferAllocInfo);
    if (!memory)
        return Error("Failed to allocate memory for uniform buffer.");

    device.bindBufferMemory(buffer, memory, 0);
    return true;
}

vk::Viewport VulkanApp::GetViewport()
{
    vk::Viewport viewport;
    viewport.width = static_cast<float>(m_vkbSwapchain.extent.width);
    viewport.height = static_cast<float>(m_vkbSwapchain.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}

vk::ShaderModule VulkanApp::LoadShaderModule(const char* pFilename)
{
    std::ifstream file(pFilename, std::ios::binary);
    if (!file.good())
    {
        Error("Failed to open file: %s", pFilename);
        return nullptr;
    }

    std::vector<char> code;
    file.seekg(0, std::ios_base::end);
    code.resize(file.tellg());
    file.seekg(0);
    file.read(code.data(), code.size());
    file.close();

    vk::ShaderModuleCreateInfo moduleInfo;
    moduleInfo.pCode = reinterpret_cast<uint32_t*>(code.data());
    moduleInfo.codeSize = code.size();

    return GetDevice().createShaderModule(moduleInfo);
}

}