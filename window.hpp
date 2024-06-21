#pragma once

#include <iostream>
#include <ranges>
#include <memory> 

#define VULKAN_HPP_NO_CONSTRUCTORS
#include "vulkan/vulkan.hpp"

#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"

class Window {

    private:
        struct Frame {
            vk::Image                   image;
            vk::ImageView               imageView;
            vk::Framebuffer             framebuffer;
            vk::CommandPool             commandPool;
            vk::CommandBuffer           commandBuffer;
            vk::Semaphore               signalSemaphore;
            vk::Semaphore               waitSemaphore;
            vk::Fence                   fence;
        };

    private:
        SDL_Window*                     m_window;
        SDL_Event                       m_event;
        uint32_t                        frameIndex = 0;
        bool                            running = true;

    private:
        uint32_t                        graphicsQueueFamilyIndex;
        uint32_t                        m_version;
        
        vk::DispatchLoaderDynamic       m_loader;
        vk::Instance                    m_instance;
        vk::SurfaceKHR                  m_surface;
        vk::PhysicalDevice              m_physicalDevice;
        vk::Device                      m_device;
        vk::Queue                       m_queue;
        vk::SwapchainKHR                m_swapchain;
        vk::SurfaceFormatKHR            m_surfaceFormat;
        vk::RenderPass                  m_renderPass;
        vk::ClearValue                  m_clearValue        {};

        std::vector<Frame>              m_frames;

    public:
        void createWindow();
        void loadVulkanLibrary();
        void createInstance();
        void createSurface();
        void createDevice();
        void createSwapchain();
        void createImageView();
        void createRenderPass();
        void createFramebuffer();
        void createCommandPool();
        void allocateCommandBuffer();
        void createSemaphore();
        void createFence();

    public:
        Window() = default;
        Window(Window&&) = default;

    public:
        Window& operator = (Window& window);

    public:
        void show();
        void hide();
        void setClearColor(float r, float g, float b);
        bool shouldShutdown();
        void pollEvent();
        void update();
        void cleanUp();

        static Window createDefaultWindow();
};
