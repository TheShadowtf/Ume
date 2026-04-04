#include "umepch.h"
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanUtils.h"

#include "Ume/Core/Log.h"

#include <set>
#include <algorithm>
#include <array>

namespace Ume {

	VulkanContext* VulkanContext::s_Instance = nullptr;

	static const std::vector<const char*> s_ValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	static const std::vector<const char*> s_DeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

#ifdef UME_DEBUG
	static constexpr bool s_EnableValidationLayers = true;
#else
	static constexpr bool s_EnableValidationLayers = false;
#endif

	// -----------------------------------------------------------------------
	// Helpers
	// -----------------------------------------------------------------------

	static VkResult CreateDebugUtilsMessengerEXT(
	    VkInstance instance,
	    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	    const VkAllocationCallbacks* pAllocator,
	    VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	static void DestroyDebugUtilsMessengerEXT(
	    VkInstance instance,
	    VkDebugUtilsMessengerEXT debugMessenger,
	    const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
			func(instance, debugMessenger, pAllocator);
	}

	static bool CheckValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : s_ValidationLayers)
		{
			bool found = false;
			for (const auto& layerProps : availableLayers)
			{
				if (strcmp(layerName, layerProps.layerName) == 0) { found = true; break; }
			}
			if (!found) return false;
		}
		return true;
	}

	// -----------------------------------------------------------------------
	// Construction / destruction
	// -----------------------------------------------------------------------

	VulkanContext::VulkanContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		UME_CORE_ASSERT(windowHandle, "Window handle is null!");
		s_Instance = this;
	}

	VulkanContext::~VulkanContext()
	{
		vkDeviceWaitIdle(m_Device);

		CleanupSwapChain();

		for (VkDescriptorPool descriptorPool : m_FrameDescriptorPools)
			vkDestroyDescriptorPool(m_Device, descriptorPool, nullptr);
		m_FrameDescriptorPools.clear();

		vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
		}

		vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

		for (auto& [key, rp] : m_PipelineRenderPasses)
			vkDestroyRenderPass(m_Device, rp, nullptr);
		m_PipelineRenderPasses.clear();

		vkDestroyDevice(m_Device, nullptr);

		if (s_EnableValidationLayers)
			DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);

		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyInstance(m_Instance, nullptr);

		s_Instance = nullptr;
	}

	void VulkanContext::Init()
	{
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreateDepthResources();
		CreateFramebuffers();
		CreateCommandPool();
		CreateCommandBuffers();
		CreateSyncObjects();
		CreateDescriptorPool();

		UME_CORE_INFO("Vulkan Context initialized successfully.");
		BeginFrame();
	}

	// -----------------------------------------------------------------------
	// Frame management  (called from Application loop via RenderCommand)
	// -----------------------------------------------------------------------

	void VulkanContext::BeginFrame()
	{
		if (m_FrameStarted)
			return;

		vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

		VkResult result = vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX,
		    m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &m_CurrentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapChain();
			return;
		}
		UME_CORE_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to acquire swap chain image!");

		vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);
		vkResetDescriptorPool(m_Device, m_FrameDescriptorPools[m_CurrentFrame], 0);

		VkCommandBuffer cmd = m_CommandBuffers[m_CurrentFrame];
		vkResetCommandBuffer(cmd, 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

		m_SwapchainRenderPassActive = false;
		m_FrameStarted = true;
	}

	void VulkanContext::BeginSwapchainRenderPass()
	{
		UME_CORE_ASSERT(m_FrameStarted, "Swapchain render pass requires an active frame!");
		if (m_SwapchainRenderPassActive)
			return;

		VkCommandBuffer cmd = m_CommandBuffers[m_CurrentFrame];

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass        = m_RenderPass;
		renderPassInfo.framebuffer       = m_SwapChainFramebuffers[m_CurrentImageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_SwapChainExtent;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color        = { { 0.1f, 0.1f, 0.1f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = (uint32_t)clearValues.size();
		renderPassInfo.pClearValues    = clearValues.data();

		vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x        = 0.0f;
		viewport.y        = 0.0f;
		viewport.width    = (float)m_SwapChainExtent.width;
		viewport.height   = (float)m_SwapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_SwapChainExtent;
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		m_SwapchainRenderPassActive = true;
	}

	void VulkanContext::EndSwapchainRenderPass()
	{
		if (!m_SwapchainRenderPassActive)
			return;

		VkCommandBuffer cmd = m_CommandBuffers[m_CurrentFrame];
		vkCmdEndRenderPass(cmd);
		m_SwapchainRenderPassActive = false;
	}

	void VulkanContext::EndFrame()
	{
		if (!m_FrameStarted)
			return;

		VkCommandBuffer cmd = m_CommandBuffers[m_CurrentFrame];
		EndSwapchainRenderPass();
		VK_CHECK(vkEndCommandBuffer(cmd));

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[]      = { m_ImageAvailableSemaphores[m_CurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount     = 1;
		submitInfo.pWaitSemaphores        = waitSemaphores;
		submitInfo.pWaitDstStageMask      = waitStages;
		submitInfo.commandBufferCount     = 1;
		submitInfo.pCommandBuffers        = &cmd;

		VkSemaphore signalSemaphores[]  = { m_RenderFinishedSemaphores[m_CurrentImageIndex] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores    = signalSemaphores;

		VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]));

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores    = signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_SwapChain };
		presentInfo.swapchainCount  = 1;
		presentInfo.pSwapchains     = swapChains;
		presentInfo.pImageIndices   = &m_CurrentImageIndex;

		VkResult result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			RecreateSwapChain();
		else
			UME_CORE_ASSERT(result == VK_SUCCESS, "Failed to present swap chain image!");

		m_FrameStarted = false;
		m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void VulkanContext::SwapBuffers()
	{
		EndFrame();
		BeginFrame();
	}

	// -----------------------------------------------------------------------
	// Instance creation
	// -----------------------------------------------------------------------

	void VulkanContext::CreateInstance()
	{
		if (s_EnableValidationLayers && !CheckValidationLayerSupport())
			UME_CORE_WARN("Vulkan: validation layers requested but not available! Continuing without them.");

		VkApplicationInfo appInfo{};
		appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName   = "UmeApplication";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName        = "Ume";
		appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion         = VK_API_VERSION_1_2;

		// Extensions
		uint32_t glfwExtCount = 0;
		const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
		std::vector<const char*> extensions(glfwExts, glfwExts + glfwExtCount);

		if (s_EnableValidationLayers)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		VkInstanceCreateInfo createInfo{};
		createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo        = &appInfo;
		createInfo.enabledExtensionCount   = (uint32_t)extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (s_EnableValidationLayers && CheckValidationLayerSupport())
		{
			createInfo.enabledLayerCount   = (uint32_t)s_ValidationLayers.size();
			createInfo.ppEnabledLayerNames = s_ValidationLayers.data();

			debugCreateInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugCreateInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugCreateInfo.pfnUserCallback = DebugCallback;
			createInfo.pNext                = &debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext             = nullptr;
		}

		VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_Instance));

		UME_CORE_INFO("Vulkan instance created.");
	}

	void VulkanContext::SetupDebugMessenger()
	{
		if (!s_EnableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;

		VK_CHECK(CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger));
	}

	void VulkanContext::CreateSurface()
	{
		VK_CHECK(glfwCreateWindowSurface(m_Instance, m_WindowHandle, nullptr, &m_Surface));
	}

	// -----------------------------------------------------------------------
	// Physical device
	// -----------------------------------------------------------------------

	void VulkanContext::PickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
		UME_CORE_ASSERT(deviceCount > 0, "Failed to find a GPU with Vulkan support!");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

		// Prefer discrete GPU
		VkPhysicalDevice fallback = VK_NULL_HANDLE;
		for (const auto& device : devices)
		{
			if (IsDeviceSuitable(device))
			{
				VkPhysicalDeviceProperties props;
				vkGetPhysicalDeviceProperties(device, &props);
				if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					m_PhysicalDevice = device;
					break;
				}
				if (fallback == VK_NULL_HANDLE)
					fallback = device;
			}
		}

		if (m_PhysicalDevice == VK_NULL_HANDLE)
			m_PhysicalDevice = fallback;

		UME_CORE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Failed to find a suitable GPU!");

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

		UME_CORE_INFO("Vulkan Info:");
		UME_CORE_INFO("  Device:     {0}", properties.deviceName);
		UME_CORE_INFO("  API:        {0}.{1}.{2}",
		    VK_VERSION_MAJOR(properties.apiVersion),
		    VK_VERSION_MINOR(properties.apiVersion),
		    VK_VERSION_PATCH(properties.apiVersion));
		UME_CORE_INFO("  Driver:     {0}.{1}.{2}",
		    VK_VERSION_MAJOR(properties.driverVersion),
		    VK_VERSION_MINOR(properties.driverVersion),
		    VK_VERSION_PATCH(properties.driverVersion));
	}

	bool VulkanContext::IsDeviceSuitable(VkPhysicalDevice device) const
	{
		QueueFamilyIndices indices = FindQueueFamilies(device);
		bool extensionsSupported   = CheckDeviceExtensionSupport(device);
		bool swapChainAdequate     = false;
		if (extensionsSupported)
		{
			SwapChainSupportDetails sc = QuerySwapChainSupport(device);
			swapChainAdequate = !sc.Formats.empty() && !sc.PresentModes.empty();
		}

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(device, &features);

		return indices.IsComplete() && extensionsSupported && swapChainAdequate && features.samplerAnisotropy;
	}

	bool VulkanContext::CheckDeviceExtensionSupport(VkPhysicalDevice device) const
	{
		uint32_t count;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
		std::vector<VkExtensionProperties> available(count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available.data());

		std::set<std::string> required(s_DeviceExtensions.begin(), s_DeviceExtensions.end());
		for (const auto& ext : available)
			required.erase(ext.extensionName);

		return required.empty();
	}

	QueueFamilyIndices VulkanContext::FindQueueFamilies(VkPhysicalDevice device) const
	{
		QueueFamilyIndices indices;
		uint32_t count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
		std::vector<VkQueueFamilyProperties> families(count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

		for (uint32_t i = 0; i < count; i++)
		{
			if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.GraphicsFamily = i;

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
			if (presentSupport)
				indices.PresentFamily = i;

			if (indices.IsComplete()) break;
		}
		return indices;
	}

	// -----------------------------------------------------------------------
	// Logical device
	// -----------------------------------------------------------------------

	void VulkanContext::CreateLogicalDevice()
	{
		QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueFamilies = { indices.GraphicsFamily.value(), indices.PresentFamily.value() };

		float priority = 1.0f;
		for (uint32_t family : uniqueFamilies)
		{
			VkDeviceQueueCreateInfo info{};
			info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			info.queueFamilyIndex = family;
			info.queueCount       = 1;
			info.pQueuePriorities = &priority;
			queueCreateInfos.push_back(info);
		}

		VkPhysicalDeviceFeatures supportedFeatures{};
		vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &supportedFeatures);

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.independentBlend = supportedFeatures.independentBlend;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos       = queueCreateInfos.data();
		createInfo.queueCreateInfoCount    = (uint32_t)queueCreateInfos.size();
		createInfo.pEnabledFeatures        = &deviceFeatures;
		createInfo.enabledExtensionCount   = (uint32_t)s_DeviceExtensions.size();
		createInfo.ppEnabledExtensionNames = s_DeviceExtensions.data();

		if (s_EnableValidationLayers && CheckValidationLayerSupport())
		{
			createInfo.enabledLayerCount   = (uint32_t)s_ValidationLayers.size();
			createInfo.ppEnabledLayerNames = s_ValidationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		VK_CHECK(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device));

		vkGetDeviceQueue(m_Device, indices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, indices.PresentFamily.value(), 0, &m_PresentQueue);
		m_GraphicsQueueFamilyIndex = indices.GraphicsFamily.value();
		m_PresentQueueFamilyIndex = indices.PresentFamily.value();
	}

	// -----------------------------------------------------------------------
	// Swap chain
	// -----------------------------------------------------------------------

	SwapChainSupportDetails VulkanContext::QuerySwapChainSupport(VkPhysicalDevice device) const
	{
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.Capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);
		if (formatCount)
		{
			details.Formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.Formats.data());
		}

		uint32_t modeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &modeCount, nullptr);
		if (modeCount)
		{
			details.PresentModes.resize(modeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &modeCount, details.PresentModes.data());
		}
		return details;
	}

	VkSurfaceFormatKHR VulkanContext::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const
	{
		for (const auto& f : formats)
		{
			if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return f;
		}
		return formats[0];
	}

	VkPresentModeKHR VulkanContext::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes) const
	{
		if (!m_VSyncEnabled)
		{
			for (const auto& m : modes)
			{
				if (m == VK_PRESENT_MODE_IMMEDIATE_KHR)
					return m;
			}
			// IMMEDIATE not available — fall through to uncapped-ish MAILBOX
			for (const auto& m : modes)
			{
				if (m == VK_PRESENT_MODE_MAILBOX_KHR)
					return m;
			}
		}
		else
		{
			for (const auto& m : modes)
			{
				if (m == VK_PRESENT_MODE_MAILBOX_KHR)
					return m;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	void VulkanContext::SetVSync(bool enabled)
	{
		if (m_VSyncEnabled == enabled)
			return;
		m_VSyncEnabled = enabled;
		RecreateSwapChain();
	}

	VkExtent2D VulkanContext::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
			return capabilities.currentExtent;

		int w, h;
		glfwGetFramebufferSize(m_WindowHandle, &w, &h);
		VkExtent2D actual = { (uint32_t)w, (uint32_t)h };
		actual.width  = std::clamp(actual.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width);
		actual.height = std::clamp(actual.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		return actual;
	}

	void VulkanContext::CreateSwapChain()
	{
		SwapChainSupportDetails sc = QuerySwapChainSupport(m_PhysicalDevice);
		VkSurfaceFormatKHR fmt     = ChooseSwapSurfaceFormat(sc.Formats);
		VkPresentModeKHR   mode    = ChooseSwapPresentMode(sc.PresentModes);
		VkExtent2D         extent  = ChooseSwapExtent(sc.Capabilities);

		uint32_t imageCount = sc.Capabilities.minImageCount + 1;
		if (sc.Capabilities.maxImageCount > 0 && imageCount > sc.Capabilities.maxImageCount)
			imageCount = sc.Capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface          = m_Surface;
		createInfo.minImageCount    = imageCount;
		createInfo.imageFormat      = fmt.format;
		createInfo.imageColorSpace  = fmt.colorSpace;
		createInfo.imageExtent      = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
		uint32_t queueIndices[]   = { indices.GraphicsFamily.value(), indices.PresentFamily.value() };

		if (indices.GraphicsFamily != indices.PresentFamily)
		{
			createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices   = queueIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform   = sc.Capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode    = mode;
		createInfo.clipped        = VK_TRUE;
		createInfo.oldSwapchain   = VK_NULL_HANDLE;

		VK_CHECK(vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_SwapChain));

		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, nullptr);
		m_SwapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, m_SwapChainImages.data());

		m_SwapChainImageFormat = fmt.format;
		m_SwapChainExtent      = extent;
	}

	void VulkanContext::CreateImageViews()
	{
		m_SwapChainImageViews.resize(m_SwapChainImages.size());
		for (size_t i = 0; i < m_SwapChainImages.size(); i++)
			m_SwapChainImageViews[i] = CreateImageView(m_SwapChainImages[i], m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	// -----------------------------------------------------------------------
	// Render pass
	// -----------------------------------------------------------------------

	void VulkanContext::CreateRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format         = m_SwapChainImageFormat;
		colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format         = FindSupportedFormat(
		    { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		    VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorRef{};
		colorRef.attachment = 0;
		colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthRef{};
		depthRef.attachment = 1;
		depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount    = 1;
		subpass.pColorAttachments       = &colorRef;
		subpass.pDepthStencilAttachment = &depthRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass    = 0;
		dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = (uint32_t)attachments.size();
		renderPassInfo.pAttachments    = attachments.data();
		renderPassInfo.subpassCount    = 1;
		renderPassInfo.pSubpasses      = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies   = &dependency;

		VK_CHECK(vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass));
	}

	// -----------------------------------------------------------------------
	// Depth
	// -----------------------------------------------------------------------

	void VulkanContext::CreateDepthResources()
	{
		VkFormat depthFormat = FindSupportedFormat(
		    { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		    VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

		CreateImage(m_SwapChainExtent.width, m_SwapChainExtent.height,
		    depthFormat, VK_IMAGE_TILING_OPTIMAL,
		    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		    m_DepthImage, m_DepthImageMemory);

		m_DepthImageView = CreateImageView(m_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	// -----------------------------------------------------------------------
	// Framebuffers
	// -----------------------------------------------------------------------

	void VulkanContext::CreateFramebuffers()
	{
		m_SwapChainFramebuffers.resize(m_SwapChainImageViews.size());
		for (size_t i = 0; i < m_SwapChainImageViews.size(); i++)
		{
			std::array<VkImageView, 2> attachments = { m_SwapChainImageViews[i], m_DepthImageView };

			VkFramebufferCreateInfo fbInfo{};
			fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fbInfo.renderPass      = m_RenderPass;
			fbInfo.attachmentCount = (uint32_t)attachments.size();
			fbInfo.pAttachments    = attachments.data();
			fbInfo.width           = m_SwapChainExtent.width;
			fbInfo.height          = m_SwapChainExtent.height;
			fbInfo.layers          = 1;

			VK_CHECK(vkCreateFramebuffer(m_Device, &fbInfo, nullptr, &m_SwapChainFramebuffers[i]));
		}
	}

	// -----------------------------------------------------------------------
	// Command pool / buffers
	// -----------------------------------------------------------------------

	void VulkanContext::CreateCommandPool()
	{
		QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = indices.GraphicsFamily.value();

		VK_CHECK(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool));
	}

	void VulkanContext::CreateCommandBuffers()
	{
		m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool        = m_CommandPool;
		allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

		VK_CHECK(vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()));
	}

	void VulkanContext::CreateSyncObjects()
	{
		m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semInfo{};
		semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VK_CHECK(vkCreateSemaphore(m_Device, &semInfo, nullptr, &m_ImageAvailableSemaphores[i]));
			VK_CHECK(vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]));
		}

		CreateRenderFinishedSemaphores();
	}

	void VulkanContext::CreateRenderFinishedSemaphores()
	{
		for (VkSemaphore semaphore : m_RenderFinishedSemaphores)
			vkDestroySemaphore(m_Device, semaphore, nullptr);
		m_RenderFinishedSemaphores.clear();

		m_RenderFinishedSemaphores.resize(m_SwapChainImages.size());

		VkSemaphoreCreateInfo semInfo{};
		semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (size_t i = 0; i < m_RenderFinishedSemaphores.size(); i++)
			VK_CHECK(vkCreateSemaphore(m_Device, &semInfo, nullptr, &m_RenderFinishedSemaphores[i]));
	}

	// -----------------------------------------------------------------------
	// Descriptor pool
	// -----------------------------------------------------------------------

	void VulkanContext::CreateDescriptorPool()
	{
		std::array<VkDescriptorPoolSize, 3> poolSizes{};
		poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = 1000;
		poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = 1000;
		poolSizes[2].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[2].descriptorCount = 100;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
		poolInfo.pPoolSizes    = poolSizes.data();
		poolInfo.maxSets       = 2000;

		VK_CHECK(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool));

		std::array<VkDescriptorPoolSize, 2> framePoolSizes{};
		framePoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		framePoolSizes[0].descriptorCount = 512;
		framePoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		framePoolSizes[1].descriptorCount = 512 * 32;

		VkDescriptorPoolCreateInfo framePoolInfo{};
		framePoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		framePoolInfo.poolSizeCount = (uint32_t)framePoolSizes.size();
		framePoolInfo.pPoolSizes = framePoolSizes.data();
		framePoolInfo.maxSets = 512;

		m_FrameDescriptorPools.resize(MAX_FRAMES_IN_FLIGHT);
		for (VkDescriptorPool& descriptorPool : m_FrameDescriptorPools)
			VK_CHECK(vkCreateDescriptorPool(m_Device, &framePoolInfo, nullptr, &descriptorPool));
	}

	// -----------------------------------------------------------------------
	// Swap chain recreation
	// -----------------------------------------------------------------------

	void VulkanContext::CleanupSwapChain()
	{
		for (VkSemaphore semaphore : m_RenderFinishedSemaphores)
			vkDestroySemaphore(m_Device, semaphore, nullptr);
		m_RenderFinishedSemaphores.clear();

		vkDestroyImageView(m_Device, m_DepthImageView, nullptr);
		vkDestroyImage(m_Device, m_DepthImage, nullptr);
		vkFreeMemory(m_Device, m_DepthImageMemory, nullptr);

		for (auto fb : m_SwapChainFramebuffers)
			vkDestroyFramebuffer(m_Device, fb, nullptr);

		for (auto iv : m_SwapChainImageViews)
			vkDestroyImageView(m_Device, iv, nullptr);

		vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
	}

	void VulkanContext::RecreateSwapChain()
	{
		int w = 0, h = 0;
		glfwGetFramebufferSize(m_WindowHandle, &w, &h);
		while (w == 0 || h == 0)
		{
			glfwGetFramebufferSize(m_WindowHandle, &w, &h);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_Device);

		CleanupSwapChain();
		CreateSwapChain();
		CreateImageViews();
		CreateDepthResources();
		CreateFramebuffers();
		CreateRenderFinishedSemaphores();
		m_SwapchainRenderPassActive = false;
	}

	// -----------------------------------------------------------------------
	// Utility helpers
	// -----------------------------------------------------------------------

	VkFormat VulkanContext::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
	{
		for (VkFormat fmt : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, fmt, &props);
			if (tiling == VK_IMAGE_TILING_LINEAR  && (props.linearTilingFeatures  & features) == features) return fmt;
			if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) return fmt;
		}
		UME_CORE_ASSERT(false, "Failed to find supported format!");
		return VK_FORMAT_UNDEFINED;
	}

	uint32_t VulkanContext::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
	{
		VkPhysicalDeviceMemoryProperties memProps;
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProps);

		for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
				return i;
		}
		UME_CORE_ASSERT(false, "Failed to find suitable memory type!");
		return 0;
	}

	void VulkanContext::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
	                                 VkBuffer& buffer, VkDeviceMemory& bufferMemory)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size        = size;
		bufferInfo.usage       = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK(vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer));

		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(m_Device, buffer, &memReqs);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize  = memReqs.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, properties);

		VK_CHECK(vkAllocateMemory(m_Device, &allocInfo, nullptr, &bufferMemory));
		vkBindBufferMemory(m_Device, buffer, bufferMemory, 0);
	}

	void VulkanContext::CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
	{
		VkCommandBuffer cmd = BeginSingleTimeCommands();
		VkBufferCopy copy{}; copy.size = size;
		vkCmdCopyBuffer(cmd, src, dst, 1, &copy);
		EndSingleTimeCommands(cmd);
	}

	void VulkanContext::CreateImage(uint32_t width, uint32_t height, VkFormat format,
	                                VkImageTiling tiling, VkImageUsageFlags usage,
	                                VkMemoryPropertyFlags properties,
	                                VkImage& image, VkDeviceMemory& imageMemory)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType     = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width  = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth  = 1;
		imageInfo.mipLevels     = 1;
		imageInfo.arrayLayers   = 1;
		imageInfo.format        = format;
		imageInfo.tiling        = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage         = usage;
		imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

		VK_CHECK(vkCreateImage(m_Device, &imageInfo, nullptr, &image));

		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(m_Device, image, &memReqs);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize  = memReqs.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, properties);

		VK_CHECK(vkAllocateMemory(m_Device, &allocInfo, nullptr, &imageMemory));
		vkBindImageMemory(m_Device, image, imageMemory, 0);
	}

	void VulkanContext::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer cmd = BeginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout           = oldLayout;
		barrier.newLayout           = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image               = image;

		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		barrier.subresourceRange.baseMipLevel   = 0;
		barrier.subresourceRange.levelCount     = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount     = 1;

		VkPipelineStageFlags srcStage, dstStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
		{
			UME_CORE_ASSERT(false, "Unsupported image layout transition!");
			srcStage = dstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		}

		vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		EndSingleTimeCommands(cmd);
	}

	void VulkanContext::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer cmd = BeginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset      = 0;
		region.bufferRowLength   = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel       = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount     = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		EndSingleTimeCommands(cmd);
	}

	VkImageView VulkanContext::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image                           = image;
		viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format                          = format;
		viewInfo.subresourceRange.aspectMask     = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel   = 0;
		viewInfo.subresourceRange.levelCount     = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount     = 1;

		VkImageView imageView;
		VK_CHECK(vkCreateImageView(m_Device, &viewInfo, nullptr, &imageView));
		return imageView;
	}

	VkCommandBuffer VulkanContext::BeginSingleTimeCommands()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool        = m_CommandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer cmd;
		vkAllocateCommandBuffers(m_Device, &allocInfo, &cmd);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(cmd, &beginInfo);
		return cmd;
	}

	void VulkanContext::EndSingleTimeCommands(VkCommandBuffer cmd)
	{
		vkEndCommandBuffer(cmd);

		VkSubmitInfo submitInfo{};
		submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers    = &cmd;

		vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_GraphicsQueue);

		vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &cmd);
	}

	// -----------------------------------------------------------------------
	// Debug callback
	// -----------------------------------------------------------------------

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::DebugCallback(
	    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	    VkDebugUtilsMessageTypeFlagsEXT,
	    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	    void*)
	{
		if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			UME_CORE_ERROR("Vulkan Validation: {0}", pCallbackData->pMessage);
		else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			UME_CORE_WARN("Vulkan Validation: {0}", pCallbackData->pMessage);
		return VK_FALSE;
	}

	// -----------------------------------------------------------------------
	// Pipeline-compatible render pass factory
	// Attachment 0: VK_FORMAT_R8G8B8A8_UNORM (color)
	// Attachment 1: VK_FORMAT_R32_SINT        (entity ID / integer outputs)
	// + depth attachment
	// -----------------------------------------------------------------------

	VkRenderPass VulkanContext::GetOrCreatePipelineRenderPass(uint32_t numColorAttachments)
	{
		auto it = m_PipelineRenderPasses.find(numColorAttachments);
		if (it != m_PipelineRenderPasses.end())
			return it->second;

		const VkFormat colorFormats[] = { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R32_SINT };

		std::vector<VkAttachmentDescription> atts;
		std::vector<VkAttachmentReference>   colorRefs;

		for (uint32_t i = 0; i < numColorAttachments; i++)
		{
			VkFormat fmt = (i < 2) ? colorFormats[i] : VK_FORMAT_R8G8B8A8_UNORM;

			VkAttachmentDescription att{};
			att.format         = fmt;
			att.samples        = VK_SAMPLE_COUNT_1_BIT;
			att.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			att.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			att.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			att.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
			att.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			atts.push_back(att);

			VkAttachmentReference ref{};
			ref.attachment = i;
			ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorRefs.push_back(ref);
		}

		VkAttachmentDescription depthAtt{};
		depthAtt.format         = FindSupportedFormat(
		    { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		    VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		depthAtt.samples        = VK_SAMPLE_COUNT_1_BIT;
		depthAtt.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAtt.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAtt.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAtt.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAtt.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		atts.push_back(depthAtt);

		VkAttachmentReference depthRef{};
		depthRef.attachment = numColorAttachments;
		depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount    = (uint32_t)colorRefs.size();
		subpass.pColorAttachments       = colorRefs.empty() ? nullptr : colorRefs.data();
		subpass.pDepthStencilAttachment = &depthRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo rpInfo{};
		rpInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rpInfo.attachmentCount = (uint32_t)atts.size();
		rpInfo.pAttachments    = atts.data();
		rpInfo.subpassCount    = 1;
		rpInfo.pSubpasses      = &subpass;
		rpInfo.dependencyCount = 1;
		rpInfo.pDependencies   = &dependency;

		VkRenderPass rp = VK_NULL_HANDLE;
		VK_CHECK(vkCreateRenderPass(m_Device, &rpInfo, nullptr, &rp));
		m_PipelineRenderPasses[numColorAttachments] = rp;
		return rp;
	}

} // namespace Ume
