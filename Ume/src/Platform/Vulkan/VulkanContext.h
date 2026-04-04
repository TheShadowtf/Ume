#pragma once

#include "Ume/Renderer/GraphicsContext.h"
#include "Platform/Vulkan/VulkanUtils.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <optional>

namespace Ume {

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> GraphicsFamily;
		std::optional<uint32_t> PresentFamily;

		bool IsComplete() const { return GraphicsFamily.has_value() && PresentFamily.has_value(); }
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR        Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR>   PresentModes;
	};

	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);
		virtual ~VulkanContext();

		virtual void Init() override;
		virtual void SwapBuffers() override;

		void RecreateSwapChain();
		void BeginFrame();
		void EndFrame();
		void BeginSwapchainRenderPass();
		void EndSwapchainRenderPass();

		// --- Accessors used by other Vulkan classes ---
		VkInstance       GetInstance()      const { return m_Instance; }
		VkDevice         GetDevice()        const { return m_Device; }
		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkQueue          GetGraphicsQueue() const { return m_GraphicsQueue; }
		VkRenderPass     GetRenderPass()     const { return m_RenderPass; }
		VkCommandBuffer  GetCurrentCommandBuffer() const { return m_CommandBuffers[m_CurrentFrame]; }
		VkExtent2D       GetSwapChainExtent() const { return m_SwapChainExtent; }
		VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }
		VkDescriptorPool GetCurrentFrameDescriptorPool() const { return m_FrameDescriptorPools[m_CurrentFrame]; }
		uint32_t         GetCurrentFrame()   const { return m_CurrentFrame; }
		uint32_t         GetImageCount()     const { return (uint32_t)m_SwapChainImages.size(); }
		uint32_t         GetGraphicsQueueFamilyIndex() const { return m_GraphicsQueueFamilyIndex; }
		bool             IsFrameStarted() const { return m_FrameStarted; }
		bool             IsSwapchainRenderPassActive() const { return m_SwapchainRenderPassActive; }

		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

		// Creates (and caches) a render pass with the given number of color attachments
		// using standard formats (RGBA8 + R32_SINT for 2nd slot), suitable for pipeline creation.
		VkRenderPass GetOrCreatePipelineRenderPass(uint32_t numColorAttachments);

		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
		                  VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

		void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
		                 VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
		                 VkImage& image, VkDeviceMemory& imageMemory);
		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

		VkCommandBuffer BeginSingleTimeCommands();
		void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;

		void SetVSync(bool enabled);
		bool IsVSync() const { return m_VSyncEnabled; }

		static VulkanContext* Get() { return s_Instance; }

	private:
		void CreateInstance();
		void SetupDebugMessenger();
		void CreateSurface();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSwapChain();
		void CreateImageViews();
		void CreateRenderPass();
		void CreateDepthResources();
		void CreateFramebuffers();
		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateSyncObjects();
		void CreateRenderFinishedSemaphores();
		void CreateDescriptorPool();

		void CleanupSwapChain();

		bool IsDeviceSuitable(VkPhysicalDevice device) const;
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
		VkPresentModeKHR   ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes) const;
		VkExtent2D         ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		    VkDebugUtilsMessageTypeFlagsEXT type,
		    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		    void* pUserData);

	private:
		GLFWwindow* m_WindowHandle = nullptr;

		VkInstance               m_Instance               = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger          = VK_NULL_HANDLE;
		VkSurfaceKHR             m_Surface                 = VK_NULL_HANDLE;
		VkPhysicalDevice         m_PhysicalDevice          = VK_NULL_HANDLE;
		VkDevice                 m_Device                  = VK_NULL_HANDLE;
		VkQueue                  m_GraphicsQueue           = VK_NULL_HANDLE;
		VkQueue                  m_PresentQueue            = VK_NULL_HANDLE;
		uint32_t                 m_GraphicsQueueFamilyIndex = UINT32_MAX;
		uint32_t                 m_PresentQueueFamilyIndex  = UINT32_MAX;

		VkSwapchainKHR           m_SwapChain               = VK_NULL_HANDLE;
		VkFormat                 m_SwapChainImageFormat    = VK_FORMAT_UNDEFINED;
		VkExtent2D               m_SwapChainExtent         = {};
		std::vector<VkImage>     m_SwapChainImages;
		std::vector<VkImageView> m_SwapChainImageViews;
		std::vector<VkFramebuffer> m_SwapChainFramebuffers;

		VkImage        m_DepthImage       = VK_NULL_HANDLE;
		VkDeviceMemory m_DepthImageMemory = VK_NULL_HANDLE;
		VkImageView    m_DepthImageView   = VK_NULL_HANDLE;

		VkRenderPass     m_RenderPass    = VK_NULL_HANDLE;
		VkCommandPool    m_CommandPool   = VK_NULL_HANDLE;
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		std::vector<VkDescriptorPool> m_FrameDescriptorPools;

		// Cached pipeline-compatible render passes keyed by color attachment count
		std::unordered_map<uint32_t, VkRenderPass> m_PipelineRenderPasses;

		static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
		std::vector<VkCommandBuffer> m_CommandBuffers;
		std::vector<VkSemaphore>     m_ImageAvailableSemaphores;
		std::vector<VkSemaphore>     m_RenderFinishedSemaphores;
		std::vector<VkFence>         m_InFlightFences;

		uint32_t m_CurrentFrame       = 0;
		uint32_t m_CurrentImageIndex  = 0;
		bool     m_FrameStarted       = false;
		bool     m_SwapchainRenderPassActive = false;
		bool     m_VSyncEnabled       = true;

		static VulkanContext* s_Instance;
	};

}
