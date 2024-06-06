#define VULKAN_HPP_NO_CONSTRUCTORS
#include "vulkan/vulkan.hpp"

#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"

#include <iostream>
#include <vector>
#include <algorithm>

int main() {

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
    SDL_DestroyProperties(props);

    SDL_Vulkan_LoadLibrary(nullptr);

    vk::DispatchLoaderDynamic loader(reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr()));

    uint32_t version = vk::enumerateInstanceVersion(loader);
    std::cout << VK_VERSION_MAJOR(version) << "." << VK_VERSION_MINOR(version) << "." << VK_VERSION_PATCH(version) << std::endl;

    vk::ApplicationInfo applicationInfo {};
    applicationInfo.setApiVersion(version);

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

    auto instance = vk::createInstance(instanceCreateInfo, nullptr, loader);
    loader.init(instance);

    auto physicalDevices = instance.enumeratePhysicalDevices(loader);
    auto physicalDevice = *(std::find_if(physicalDevices.begin(), physicalDevices.end(), [&loader](const auto& physicalDevice) {
        return physicalDevice.getProperties(loader).deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
    }));

    VkSurfaceKHR tempSurface;
    if(!SDL_Vulkan_CreateSurface(window, instance, {}, &tempSurface)) {
        std::runtime_error("Failed to create surface");
    };
    vk::SurfaceKHR surface(tempSurface);

    std::cout << physicalDevice.getProperties(loader).deviceName << std::endl;

    auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties(loader);
    
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

    auto device = physicalDevice.createDevice(deviceCreateInfo, nullptr, loader);
    loader.init(device);

    vk::Queue graphicsQueue = device.getQueue(graphicsQueueFamilyIndex, 0, loader);

    auto const surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface, loader);
    auto const surfaceFormat = physicalDevice.getSurfaceFormatsKHR(surface, loader).front();
    auto const presentMode = physicalDevice.getSurfacePresentModesKHR(surface, loader).front();

    vk::SwapchainCreateInfoKHR swapchainCreateInfo;
    swapchainCreateInfo.setSurface(surface);
    swapchainCreateInfo.setMinImageCount(3);
    swapchainCreateInfo.setImageFormat(surfaceFormat.format);
    swapchainCreateInfo.setImageColorSpace(surfaceFormat.colorSpace);
    swapchainCreateInfo.setImageExtent(surfaceCapabilities.currentExtent);
    swapchainCreateInfo.setImageArrayLayers(1);
    swapchainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
    swapchainCreateInfo.setImageSharingMode(vk::SharingMode::eExclusive);
    swapchainCreateInfo.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
    swapchainCreateInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
    swapchainCreateInfo.setPresentMode(presentMode);
    swapchainCreateInfo.setClipped(VK_FALSE);
    swapchainCreateInfo.setOldSwapchain(nullptr);

    vk::SwapchainKHR swapchain = device.createSwapchainKHR(swapchainCreateInfo, nullptr, loader);

    std::vector const descriptions 
    {
        vk::AttachmentDescription
        {
            .flags = {}, 
            .format = surfaceFormat.format, 
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

    vk::RenderPass renderPass = device.createRenderPass(renderPassCreateInfo, nullptr, loader);

    auto images = device.getSwapchainImagesKHR(swapchain, loader);
    
    std::vector<vk::ImageView> imageViews;
    std::vector<vk::Framebuffer> framebuffers;

    for(auto const& image : images) {
        
        vk::ImageViewCreateInfo const imageViewCreateInfo 
        {
            .sType = vk::StructureType::eImageViewCreateInfo,
            .pNext = {},
            .flags = {},
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = surfaceFormat.format,
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

        auto const imageView = device.createImageView(imageViewCreateInfo, nullptr, loader);
        imageViews.push_back(imageView);

        vk::FramebufferCreateInfo const framebufferCreateInfo
        {
            .sType = vk::StructureType::eFramebufferCreateInfo,
            .pNext = {},
            .flags = {},
            .renderPass = renderPass,
            .attachmentCount = 1,
            .pAttachments = &imageView,
            .width = surfaceCapabilities.currentExtent.width,
            .height = surfaceCapabilities.currentExtent.height,
            .layers = 1
        };

        framebuffers.push_back(device.createFramebuffer(framebufferCreateInfo, nullptr, loader));
    }

    vk::CommandPoolCreateInfo const commandPoolCreateInfo 
    {
        .sType = vk::StructureType::eCommandPoolCreateInfo,
        .pNext = {},
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphicsQueueFamilyIndex
    };

    std::vector<vk::CommandPool> commandPools(images.size());
    std::vector<vk::CommandBuffer> commandBuffers;

    for(auto& commandPool : commandPools) 
    {
        commandPool = device.createCommandPool(commandPoolCreateInfo, nullptr, loader);
    
        vk::CommandBufferAllocateInfo const commandBufferAllocateInfo 
        {
            .sType = vk::StructureType::eCommandBufferAllocateInfo,
            .pNext = {},
            .commandPool = commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };

        auto commandBuffer = device.allocateCommandBuffers(commandBufferAllocateInfo, loader).front();
        commandBuffers.push_back(commandBuffer);
    } 
    
    std::vector<vk::Fence> fences(images.size());
    vk::FenceCreateInfo const fenceCreateInfo
    {
        .sType = vk::StructureType::eFenceCreateInfo,
        .pNext = {},
        .flags = vk::FenceCreateFlagBits::eSignaled
    };

    for(auto& fence : fences) {
        fence = device.createFence(fenceCreateInfo, nullptr, loader);
    }

    std::vector<vk::Semaphore> signalSemaphores(images.size());
        vk::SemaphoreCreateInfo const semaphoreCreateInfo {
        .sType = vk::StructureType::eSemaphoreCreateInfo,
        .pNext = {},
        .flags = {}
    };

    for(auto& semaphore : signalSemaphores) {
        semaphore = device.createSemaphore(semaphoreCreateInfo, nullptr, loader);
    }

    std::vector<vk::Semaphore> waitSemaphores(images.size());
    for(auto& semaphore : waitSemaphores) {
        semaphore = device.createSemaphore(semaphoreCreateInfo, nullptr, loader);
    }

    uint32_t frameIndex = 0;
    bool running = true;

    SDL_ShowWindow(window);

    unsigned int red = 0;
    unsigned int green = 0;
    unsigned int blue = 0;

    while(running) 
    {
        SDL_Event event;
        while(SDL_PollEvent(&event)) 
        {    
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
            }

            if(event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }

        if(vk::Result result = device.waitForFences(1, &fences[frameIndex], vk::True, UINT64_MAX, loader); result != vk::Result::eSuccess) {
            throw std::runtime_error("Error: render() Failed to wait for fences");
        }

        if(vk::Result result = device.resetFences(1, &fences[frameIndex], loader); result != vk::Result::eSuccess) {
            throw std::runtime_error("Error: render() Failed to reset fences");
        }

        device.resetCommandPool(commandPools[frameIndex], {}, loader);

        auto imageIndex = device.acquireNextImageKHR(swapchain, UINT64_MAX, waitSemaphores[frameIndex], {}, loader);
        if(imageIndex.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to aquire next image");
        }

        vk::CommandBufferBeginInfo const commandBufferBeginInfo 
        {
            .sType = vk::StructureType::eCommandBufferBeginInfo,
            .pNext = {},
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
            .pInheritanceInfo =  {},
        };

        red = red % 256;
        green = green % 256;
        blue = blue % 256;

        red += 6;
        green += 3;
        blue += 1;

        vk::ClearValue clearValue {
            .color = vk::ClearColorValue { std::array<float, 4>{ red / static_cast<float>(256), green / static_cast<float>(256), blue / static_cast<float>(256), 1} }
        };
        
        vk::RenderPassBeginInfo const renderPassBeginInfo 
        {
            .sType = vk::StructureType::eRenderPassBeginInfo,
            .pNext = {},
            .renderPass = renderPass,
            .framebuffer = framebuffers[frameIndex],
            .renderArea {
                .offset = {0, 0},
                .extent = surfaceCapabilities.currentExtent
            },
            .clearValueCount = 1,
            .pClearValues = &clearValue
        };

        commandBuffers[frameIndex].begin(commandBufferBeginInfo, loader);
        commandBuffers[frameIndex].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline, loader);
        commandBuffers[frameIndex].endRenderPass(loader);
        commandBuffers[frameIndex].end(loader);

        vk::PipelineStageFlags waitMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo const submitInfo
        {
            .sType = vk::StructureType::eSubmitInfo,
            .pNext = {},
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &waitSemaphores[frameIndex],
            .pWaitDstStageMask = &waitMask,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffers[frameIndex],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &signalSemaphores[frameIndex],
        };

        if(vk::Result result = graphicsQueue.submit(1, &submitInfo, fences[frameIndex], loader); result != vk::Result::eSuccess) {
            throw std::runtime_error("Could not submitted");
        }

        vk::Result result {};
        vk::PresentInfoKHR const presentInfo 
        {
            .sType = vk::StructureType::ePresentInfoKHR,
            .pNext = {},
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &signalSemaphores[frameIndex],
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &imageIndex.value,
            .pResults = &result
        };

        if(result != vk::Result::eSuccess) {
            throw std::runtime_error("Could not present");
        }

        if(vk::Result result = graphicsQueue.presentKHR(presentInfo, loader); result != vk::Result::eSuccess) {
            throw std::runtime_error("Could not queue present");   
        }

        frameIndex++;
        frameIndex = frameIndex % static_cast<uint32_t>(images.size());
    }

    SDL_HideWindow(window);
    device.waitIdle(loader);

    for(auto& semaphore : waitSemaphores) {
        device.destroySemaphore(semaphore, {}, loader);
    }

    for(auto& semaphore : signalSemaphores) {
        device.destroySemaphore(semaphore, {}, loader);
    }

    for(auto& fence : fences) {
        device.destroyFence(fence, {}, loader);
    }

    for(auto& commandPool : commandPools) {
        device.destroyCommandPool(commandPool, nullptr, loader);
    }
    for(auto& framebuffer : framebuffers) device.destroyFramebuffer(framebuffer, {}, loader);
    for(auto& imageView : imageViews) device.destroyImageView(imageView, {}, loader);
    device.destroyRenderPass(renderPass, {}, loader);
    device.destroySwapchainKHR(swapchain, {}, loader);
    device.destroy({}, loader);
    instance.destroySurfaceKHR(surface, {}, loader);
    instance.destroy({}, loader);

    SDL_DestroyWindow(window);

    SDL_Vulkan_UnloadLibrary();    
}