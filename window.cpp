#include "window.hpp"

void Window::createWindow() {

        SDL_PropertiesID props = SDL_CreateProperties();

    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Hello Vulkan");
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 800);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 600);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, false);

    SDL_Window* window = SDL_CreateWindowWithProperties(props);
    if(window == nullptr) {
        throw std::runtime_error("Error: Window::createWindow()");
    }

    SDL_DestroyProperties(props);

    m_window = SDL_GetWindowID(window);
}

void Window::loadVulkanLibrary() {
    SDL_Vulkan_LoadLibrary(nullptr);
    m_loader.init(reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr()));
}

void Window::createInstance() {

    m_version = vk::enumerateInstanceVersion(m_loader);
    std::cout << VK_VERSION_MAJOR(m_version) << "." << VK_VERSION_MINOR(m_version) << "." << VK_VERSION_PATCH(m_version) << std::endl;

    vk::ApplicationInfo applicationInfo {};
    applicationInfo.setApiVersion(m_version);

    std::vector<const char*> instanceExtensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };

    uint32_t requiredExtensionsCount;
    auto requiredExtensions = SDL_Vulkan_GetInstanceExtensions(&requiredExtensionsCount);
    for(uint32_t x=0; x<requiredExtensionsCount; x++) {
        instanceExtensions.push_back(requiredExtensions[x]);
    }

    auto instanceLayers = { "VK_LAYER_KHRONOS_validation" };

    vk::InstanceCreateInfo instanceCreateInfo {};
    instanceCreateInfo.setPApplicationInfo(&applicationInfo);
    instanceCreateInfo.setPEnabledLayerNames(instanceLayers);
    instanceCreateInfo.setPEnabledExtensionNames(instanceExtensions);

    vk::Result result = vk::createInstance(&instanceCreateInfo, nullptr, &m_instance, m_loader);
    if(result != vk::Result::eSuccess) {
        throw std::runtime_error("Error: Window::createInstance()");
    }
    m_loader.init(m_instance);
}

void Window::createSurface() {
    
    VkSurfaceKHR surface;
    
    auto window = SDL_GetWindowFromID(m_window);
    SDL_bool success = SDL_Vulkan_CreateSurface(window, m_instance, {}, &surface);
    
    if(!success) {
        
    }
    m_surface = surface;

}

void Window::createDevice() {

    auto m_physicalDevices = m_instance.enumeratePhysicalDevices(m_loader);
    m_physicalDevice = *(std::find_if(m_physicalDevices.begin(), m_physicalDevices.end(), [this](const auto& m_physicalDevice) {
        return m_physicalDevice.getProperties(m_loader).deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
    }));

    std::cout << m_physicalDevice.getProperties(m_loader).deviceName << std::endl;

    auto queueFamilyProperties = m_physicalDevice.getQueueFamilyProperties(m_loader);
    
    auto iter = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(), [](const auto& properties) {
        return properties.queueFlags & vk::QueueFlagBits::eGraphics;
    });

    const uint32_t graphicsQueueFamilyIndex = std::distance(queueFamilyProperties.begin(), iter);

    auto priorities {1.0f};
    std::vector deviceExtensions {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    vk::PhysicalDeviceFeatures features;
    
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo 
    {
        .sType = vk::StructureType::eDeviceQueueCreateInfo,
        .pNext = {},
        .flags = {}, 
        .queueFamilyIndex = graphicsQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &priorities
    };
    
    vk::DeviceCreateInfo deviceCreateInfo 
    {
        .sType = vk::StructureType::eDeviceCreateInfo,
        .pNext = {},
        .flags = {},
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledLayerCount = {},
        .ppEnabledLayerNames = {},
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &features,
    };

    vk::Result result = m_physicalDevice.createDevice(&deviceCreateInfo, nullptr, &m_device, m_loader);
    if(result != vk::Result::eSuccess) {
        throw std::runtime_error("Error: Window::createDevice()");
    }
    m_loader.init(m_device);

    m_queue = m_device.getQueue(graphicsQueueFamilyIndex, 0, m_loader);
}

void Window::createSwapchain() {

    auto const surfaceCapabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface, m_loader);
    m_surfaceFormat = m_physicalDevice.getSurfaceFormatsKHR(m_surface, m_loader).front();
    auto const presentMode = m_physicalDevice.getSurfacePresentModesKHR(m_surface, m_loader).front();

    vk::SwapchainCreateInfoKHR swapchainCreateInfo;
    swapchainCreateInfo.setSurface(m_surface);
    swapchainCreateInfo.setMinImageCount(3);
    swapchainCreateInfo.setImageFormat(m_surfaceFormat.format);
    swapchainCreateInfo.setImageColorSpace(m_surfaceFormat.colorSpace);
    swapchainCreateInfo.setImageExtent(surfaceCapabilities.currentExtent);
    swapchainCreateInfo.setImageArrayLayers(1);
    swapchainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
    swapchainCreateInfo.setImageSharingMode(vk::SharingMode::eExclusive);
    swapchainCreateInfo.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
    swapchainCreateInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
    swapchainCreateInfo.setPresentMode(presentMode);
    swapchainCreateInfo.setClipped(VK_FALSE);
    swapchainCreateInfo.setOldSwapchain(nullptr);

    vk::Result result = m_device.createSwapchainKHR(&swapchainCreateInfo, nullptr, &m_swapchain, m_loader);
    if(result != vk::Result::eSuccess) {
        throw std::runtime_error("Error: Window::createSwapchain()");
    }

    auto images = m_device.getSwapchainImagesKHR(m_swapchain, m_loader);
    m_frames.resize(images.size());

    for(std::size_t x=0; x<images.size(); x++) {
        m_frames[x].image = images[x];
    }
}

void Window::createImageView() {
    
    for(auto& frame : m_frames) {
        
        vk::ImageViewCreateInfo const imageViewCreateInfo 
        {
            .sType = vk::StructureType::eImageViewCreateInfo,
            .pNext = {},
            .flags = {},
            .image = frame.image,
            .viewType = vk::ImageViewType::e2D,
            .format = m_surfaceFormat.format,
            .components = vk::ComponentMapping
            {
                .r = vk::ComponentSwizzle::eIdentity,
                .g = vk::ComponentSwizzle::eIdentity,
                .b = vk::ComponentSwizzle::eIdentity,
                .a = vk::ComponentSwizzle::eIdentity
            },
            .subresourceRange = 
            {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        vk::Result result = m_device.createImageView(&imageViewCreateInfo, nullptr, &frame.imageView, m_loader);
        if(result != vk::Result::eSuccess) {
            throw std::runtime_error("Error: Window::createImageView()");
        }    
    }
}

void Window::createRenderPass() {

    std::vector const descriptions 
    {
        vk::AttachmentDescription
        {
            .flags = {}, 
            .format = m_surfaceFormat.format, 
            .samples = vk::SampleCountFlagBits::e1, 
            .loadOp = vk::AttachmentLoadOp::eClear, 
            .storeOp = vk::AttachmentStoreOp::eStore, 
            .stencilLoadOp = vk::AttachmentLoadOp::eDontCare, 
            .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
            .initialLayout = vk::ImageLayout::eUndefined,
            .finalLayout = vk::ImageLayout::ePresentSrcKHR
        }
    };

    std::vector const attachments 
    {
        vk::AttachmentReference
        { 
            .attachment = 0, 
            .layout = vk::ImageLayout::eColorAttachmentOptimal 
        }
    };

    std::vector const supasses 
    {
        vk::SubpassDescription 
        {
            .flags = {},
            .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
            .inputAttachmentCount = {},
            .pInputAttachments = {},
            .colorAttachmentCount = static_cast<uint32_t>(attachments.size()),
            .pColorAttachments = attachments.data(),
            .pResolveAttachments = {},
            .pDepthStencilAttachment = {},
            .preserveAttachmentCount = {},
            .pPreserveAttachments = {},
        }
    };

    std::vector const dependencies 
    {
        vk::SubpassDependency 
        {
            .srcSubpass = VK_SUBPASS_EXTERNAL, 
            .dstSubpass = {}, 
            .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput, 
            .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
            .srcAccessMask = {},
            .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
            .dependencyFlags = vk::DependencyFlagBits::eByRegion
        }
    };
    
    vk::RenderPassCreateInfo const renderPassCreateInfo 
    {
        .sType = vk::StructureType::eRenderPassCreateInfo,
        .pNext = {},
        .flags = {},
        .attachmentCount = static_cast<uint32_t>(descriptions.size()),
        .pAttachments = descriptions.data(),
        .subpassCount = static_cast<uint32_t>(supasses.size()),
        .pSubpasses = supasses.data(),
        .dependencyCount = static_cast<uint32_t>(dependencies.size()),
        .pDependencies = dependencies.data(),
    };
 
    vk::Result result = m_device.createRenderPass(&renderPassCreateInfo, nullptr, &m_renderPass, m_loader);
    if(result != vk::Result::eSuccess) {
        throw std::runtime_error("Error: Window::createRenderPass()");
    }
}

void Window::createFramebuffer() {

    auto const surfaceCapabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface, m_loader);

    for(auto& frame : m_frames) {

        vk::FramebufferCreateInfo const framebufferCreateInfo
        {
            .sType = vk::StructureType::eFramebufferCreateInfo,
            .pNext = {},
            .flags = {},
            .renderPass = m_renderPass,
            .attachmentCount = 1,
            .pAttachments = &frame.imageView,
            .width = surfaceCapabilities.currentExtent.width,
            .height = surfaceCapabilities.currentExtent.height,
            .layers = 1
        };

        vk::Result result = m_device.createFramebuffer(&framebufferCreateInfo, nullptr, &frame.framebuffer, m_loader);
        if(result != vk::Result::eSuccess) {
            throw std::runtime_error("Error: Window::createFramebuffer()");
        }
    }
}

void Window::createCommandPool() {

    const vk::CommandPoolCreateInfo commandPoolCreateInfo 
    {
        .sType = vk::StructureType::eCommandPoolCreateInfo,
        .pNext = {},
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphicsQueueFamilyIndex
    };

    for(auto& frame : m_frames) {
        vk::Result result = m_device.createCommandPool(&commandPoolCreateInfo, nullptr, &frame.commandPool, m_loader);
        if(result != vk::Result::eSuccess) {
            throw std::runtime_error("Error: Window::createCommandPool()");
        }   
    }
}

void Window::allocateCommandBuffer() {

    for(auto& frame : m_frames) {
    
        vk::CommandBufferAllocateInfo const commandBufferAllocateInfo 
        {
            .sType = vk::StructureType::eCommandBufferAllocateInfo,
            .pNext = {},
            .commandPool = frame.commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };
        
        vk::Result result = m_device.allocateCommandBuffers(&commandBufferAllocateInfo, &frame.commandBuffer, m_loader);
        if(result != vk::Result::eSuccess) {
            throw std::runtime_error("Error: Window::allocateCommandBuffer()");
        }
    } 
}

void Window::createSemaphore() {

    const vk::SemaphoreCreateInfo semaphoreCreateInfo {
        .sType = vk::StructureType::eSemaphoreCreateInfo,
        .pNext = {},
        .flags = {}
    };

    for(auto& frame : m_frames) {
        vk::Result result = m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &frame.signalSemaphore, m_loader);
        if(result != vk::Result::eSuccess) {
            throw std::runtime_error("Error: Window::createSemaphore()");
        }
    }

    for(auto& frame : m_frames) {
        vk::Result result = m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &frame.waitSemaphore, m_loader);
        if(result != vk::Result::eSuccess) {
            throw std::runtime_error("Error: Window::createSemaphore()");
        }
    }
}

void Window::createFence() {

    vk::FenceCreateInfo const fenceCreateInfo
    {
        .sType = vk::StructureType::eFenceCreateInfo,
        .pNext = {},
        .flags = vk::FenceCreateFlagBits::eSignaled
    };

    for(auto& frame : m_frames) {
        vk::Result result = m_device.createFence(&fenceCreateInfo, nullptr, &frame.fence, m_loader);
        if(result != vk::Result::eSuccess) {
            throw std::runtime_error("Error: Window::Fence()");
        }
    }
}

void Window::show() {
    auto window = SDL_GetWindowFromID(m_window);
    SDL_ShowWindow(window);
}

void Window::hide() {
    auto window = SDL_GetWindowFromID(m_window);
    SDL_HideWindow(window);
}

void Window::setClearColor(float r, float g, float b) {
    m_clearValue.color = vk::ClearColorValue { std::array<float, 4>{ r, g, b, 1} };
}

bool Window::shouldShutdown() {
    return !running;
}

void Window::pollEvent() {

    while(SDL_PollEvent(&m_event)) 
    {    
        switch (m_event.type)
        {
            case SDL_EVENT_QUIT:
                running = false;
                break;
        }

        if(m_event.key.keysym.sym == SDLK_ESCAPE) {
            running = false;
        }
    }
}

void Window::update() {

    if(vk::Result result = m_device.waitForFences(1, &m_frames[frameIndex].fence, vk::True, UINT64_MAX, m_loader); result != vk::Result::eSuccess) {
        throw std::runtime_error("Error: render() Failed to wait for fences");
    }

    if(vk::Result result = m_device.resetFences(1, &m_frames[frameIndex].fence, m_loader); result != vk::Result::eSuccess) {
        throw std::runtime_error("Error: render() Failed to reset fences");
    }

    m_device.resetCommandPool(m_frames[frameIndex].commandPool, {}, m_loader);

    auto imageIndex = m_device.acquireNextImageKHR(m_swapchain, UINT64_MAX, m_frames[frameIndex].waitSemaphore, {}, m_loader);
    if(imageIndex.result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to aquire next image");
    }

    auto const surfaceCapabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface, m_loader);

    vk::RenderPassBeginInfo renderPassBeginInfo 
    {
        .sType = vk::StructureType::eRenderPassBeginInfo,
        .pNext = {},
        .renderPass = m_renderPass,
        .framebuffer = m_frames[frameIndex].framebuffer,
        .renderArea {
            .offset = {0, 0},
            .extent = surfaceCapabilities.currentExtent
        },
        .clearValueCount = 1,
        .pClearValues = &m_clearValue
    };

    vk::CommandBufferBeginInfo const commandBufferBeginInfo 
    {
        .sType = vk::StructureType::eCommandBufferBeginInfo,
        .pNext = {},
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        .pInheritanceInfo =  {},
    };

    m_frames[frameIndex].commandBuffer.begin(commandBufferBeginInfo, m_loader);
    m_frames[frameIndex].commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline, m_loader);
    m_frames[frameIndex].commandBuffer.endRenderPass(m_loader);
    m_frames[frameIndex].commandBuffer.end(m_loader);

    vk::PipelineStageFlags waitMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo const submitInfo
    {
        .sType = vk::StructureType::eSubmitInfo,
        .pNext = {},
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_frames[frameIndex].waitSemaphore,
        .pWaitDstStageMask = &waitMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &(m_frames[frameIndex].commandBuffer),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &m_frames[frameIndex].signalSemaphore,
    };

    if(vk::Result result = m_queue.submit(1, &submitInfo, m_frames[frameIndex].fence, m_loader); result != vk::Result::eSuccess) {
        throw std::runtime_error("Could not submitted");
    }

    vk::Result result {};
    vk::PresentInfoKHR const presentInfo 
    {
        .sType = vk::StructureType::ePresentInfoKHR,
        .pNext = {},
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_frames[frameIndex].signalSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &m_swapchain,
        .pImageIndices = &imageIndex.value,
        .pResults = &result
    };

    if(result != vk::Result::eSuccess) {
        throw std::runtime_error("Could not present");
    }

    if(vk::Result result = m_queue.presentKHR(presentInfo, m_loader); result != vk::Result::eSuccess) {
        throw std::runtime_error("Could not queue present");   
    }

    frameIndex++;
    frameIndex = frameIndex % static_cast<uint32_t>(m_frames.size());
}

void Window::cleanUp() {

    m_device.waitIdle(m_loader);

    for(auto& frame : m_frames) {
        if(frame.fence) m_device.destroyFence(frame.fence, nullptr, m_loader);
        if(frame.waitSemaphore) m_device.destroySemaphore(frame.waitSemaphore, nullptr, m_loader);
        if(frame.signalSemaphore) m_device.destroySemaphore(frame.signalSemaphore, nullptr, m_loader);       
        if(frame.commandPool) m_device.destroyCommandPool(frame.commandPool, nullptr, m_loader);
        if(frame.framebuffer) m_device.destroyFramebuffer(frame.framebuffer, nullptr, m_loader);
        if(frame.imageView) m_device.destroyImageView(frame.imageView, nullptr, m_loader);
    }

    if(m_renderPass) m_device.destroyRenderPass(m_renderPass, nullptr, m_loader);
    
    if(m_swapchain) m_device.destroySwapchainKHR(m_swapchain, nullptr, m_loader);
    if(m_device) m_device.destroy(nullptr, m_loader);
    if(m_surface) m_instance.destroySurfaceKHR(m_surface, nullptr, m_loader);
    if(m_instance) m_instance.destroy(nullptr, m_loader);

    SDL_Vulkan_UnloadLibrary();
    if(m_window != 0) {
        auto window = SDL_GetWindowFromID(m_window);
        SDL_DestroyWindow(window);
    } 
    
        
    SDL_Quit();
}

Window Window::createDefaultWindow() {

    Window window;

    try {

        window.createWindow();
        window.loadVulkanLibrary();
        window.createInstance();
        window.createSurface();
        window.createDevice();
        window.createSwapchain();
        window.createImageView();
        window.createRenderPass();
        window.createFramebuffer();
        window.createCommandPool();
        window.allocateCommandBuffer();
        window.createSemaphore();
        window.createFence();

    } catch(vk::SystemError error) {
        std::cout << error.code() << std::endl;
    }

    return window;
}
