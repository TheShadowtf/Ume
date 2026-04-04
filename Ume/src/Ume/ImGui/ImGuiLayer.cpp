#include "umepch.h"
#include "ImGuiLayer.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>
#include <examples/imgui_impl_vulkan.h>

#include "Ume/Core/Application.h"

#include "Ume/Renderer/Renderer.h"
#include "Platform/Vulkan/VulkanContext.h"

#include <filesystem>

// TEMPORARY
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "ImGuizmo.h"

namespace Ume {
	static void ImGuiVulkanCheckResult(VkResult result)
	{
		if (result != VK_SUCCESS)
			UME_CORE_ERROR("ImGui Vulkan error: {0}", (int)result);
	}

	ImGuiLayer::ImGuiLayer()
		: Layer("ImGuiLayer")
	{
	}

	void ImGuiLayer::OnAttach()
	{
		UME_PROFILE_FUNCTION();

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		std::error_code currentPathError;
		m_IniFilePath = (std::filesystem::current_path(currentPathError) / "imgui.ini").string();
		if (!currentPathError)
			io.IniFilename = m_IniFilePath.c_str();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		if (Renderer::GetAPI() != RendererAPI::API::Vulkan)
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

		// Keep startup responsive by using the default font first; load custom fonts shortly after first frames.
		io.Fonts->AddFontDefault();
		m_PendingCustomFontLoad = Renderer::GetAPI() != RendererAPI::API::Vulkan;
		m_FrameCounter = 0;
		if (Renderer::GetAPI() == RendererAPI::API::Vulkan)
			LoadCustomFonts();

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();

		style.WindowPadding = ImVec2(8.0f, 6.0f);
		style.FramePadding = ImVec2(8.0f, 5.0f);
		style.ItemSpacing = ImVec2(7.0f, 6.0f);
		style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
		style.IndentSpacing = 18.0f;
		style.ScrollbarSize = 13.0f;
		style.GrabMinSize = 10.0f;
		style.WindowRounding = 0.0f;
		style.ChildRounding = 0.0f;
		style.FrameRounding = 4.0f;
		style.PopupRounding = 3.0f;
		style.ScrollbarRounding = 10.0f;
		style.GrabRounding = 3.0f;
		style.TabRounding = 4.0f;
		style.WindowBorderSize = 1.0f;
		style.ChildBorderSize = 1.0f;
		style.FrameBorderSize = 1.0f;
		style.PopupBorderSize = 1.0f;
		style.TabBorderSize = 0.0f;
		
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		SetDarkThemeColors();

		Application& app = Application::Get();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

		// Setup Platform/Renderer bindings
		if (Renderer::GetAPI() == RendererAPI::API::Vulkan)
		{
			auto* context = VulkanContext::Get();
			UME_CORE_ASSERT(context, "VulkanContext is null during ImGui Vulkan initialization!");
			ImGui_ImplGlfw_InitForVulkan(window, true);

			ImGui_ImplVulkan_InitInfo initInfo{};
			initInfo.Instance = context->GetInstance();
			initInfo.PhysicalDevice = context->GetPhysicalDevice();
			initInfo.Device = context->GetDevice();
			initInfo.QueueFamily = context->GetGraphicsQueueFamilyIndex();
			initInfo.Queue = context->GetGraphicsQueue();
			initInfo.PipelineCache = VK_NULL_HANDLE;
			initInfo.DescriptorPool = context->GetDescriptorPool();
			initInfo.MinImageCount = std::max(2u, context->GetImageCount());
			initInfo.ImageCount = std::max(2u, context->GetImageCount());
			initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
			initInfo.Allocator = nullptr;
			initInfo.CheckVkResultFn = ImGuiVulkanCheckResult;

			ImGui_ImplVulkan_Init(&initInfo, context->GetRenderPass());
			UploadFontsTexture();
		}
		else
		{
			ImGui_ImplGlfw_InitForOpenGL(window, true);
			ImGui_ImplOpenGL3_Init("#version 410");
		}
	}

	void ImGuiLayer::OnDetach()
	{
		UME_PROFILE_FUNCTION();

		if (Renderer::GetAPI() == RendererAPI::API::Vulkan)
		{
			if (auto* context = VulkanContext::Get())
				vkDeviceWaitIdle(context->GetDevice());
			ImGui_ImplVulkan_Shutdown();
		}
		else
			ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::OnEvent(Event& e)
	{
		if (m_BlockEvents)
		{
			ImGuiIO& io = ImGui::GetIO();
			e.Handled |= e.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
			e.Handled |= e.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
		}
	}
	
	void ImGuiLayer::Begin()
	{
		UME_PROFILE_FUNCTION();

		if (Renderer::GetAPI() == RendererAPI::API::Vulkan)
			ImGui_ImplVulkan_NewFrame();
		else
			ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void ImGuiLayer::End()
	{
		UME_PROFILE_FUNCTION();

		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

		// Rendering
		ImGui::Render();
		if (Renderer::GetAPI() == RendererAPI::API::Vulkan)
		{
			auto* context = VulkanContext::Get();
			UME_CORE_ASSERT(context, "VulkanContext is null during ImGui rendering!");
			if (context && context->IsFrameStarted())
			{
				context->BeginSwapchainRenderPass();
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), context->GetCurrentCommandBuffer());
				context->EndSwapchainRenderPass();
			}
		}
		else
		{
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}

		if (Renderer::GetAPI() != RendererAPI::API::Vulkan && (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		if (m_PendingCustomFontLoad)
		{
			m_FrameCounter++;
			if (m_FrameCounter >= 4)
			{
				LoadCustomFonts();
				UploadFontsTexture();

				m_PendingCustomFontLoad = false;
			}
		}
	}

	void ImGuiLayer::LoadCustomFonts()
	{
		ImGuiIO& io = ImGui::GetIO();
		const float fontSize = 18.0f;
		io.Fonts->Clear();
		io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Bold.ttf", fontSize);
		ImFont* regularFont = io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Regular.ttf", fontSize);
		if (regularFont)
			io.FontDefault = regularFont;

		if (io.Fonts->Fonts.empty())
			io.Fonts->AddFontDefault();
	}

	void ImGuiLayer::UploadFontsTexture()
	{
		if (Renderer::GetAPI() == RendererAPI::API::Vulkan)
		{
			auto* context = VulkanContext::Get();
			VkCommandBuffer commandBuffer = context->BeginSingleTimeCommands();
			ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
			context->EndSingleTimeCommands(commandBuffer);
			ImGui_ImplVulkan_DestroyFontUploadObjects();
		}
		else
		{
			ImGui_ImplOpenGL3_DestroyFontsTexture();
			ImGui_ImplOpenGL3_CreateFontsTexture();
		}
	}

	void ImGuiLayer::SetHazelDarkThemeColors()
	{
		auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		
		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

		// Title
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	}

	void ImGuiLayer::SetDarkThemeColors()
	{
		auto& colors = ImGui::GetStyle().Colors;

		const ImVec4 appBg = ImVec4(0.112f, 0.112f, 0.116f, 1.0f);
		const ImVec4 panelBg = ImVec4(0.154f, 0.157f, 0.163f, 1.0f);
		const ImVec4 surfaceBg = ImVec4(0.186f, 0.190f, 0.198f, 1.0f);
		const ImVec4 elevatedBg = ImVec4(0.214f, 0.220f, 0.229f, 1.0f);
		const ImVec4 lineColor = ImVec4(0.098f, 0.102f, 0.106f, 1.0f);
		const ImVec4 blueAccent = ImVec4(0.239f, 0.549f, 0.945f, 1.0f);
		const ImVec4 blueAccentSoft = ImVec4(0.239f, 0.549f, 0.945f, 0.32f);
		const ImVec4 textColor = ImVec4(0.862f, 0.870f, 0.886f, 1.0f);
		const ImVec4 textMuted = ImVec4(0.548f, 0.568f, 0.600f, 1.0f);

		colors[ImGuiCol_Text] = textColor;
		colors[ImGuiCol_TextDisabled] = textMuted;
		colors[ImGuiCol_WindowBg] = panelBg;
		colors[ImGuiCol_ChildBg] = appBg;
		colors[ImGuiCol_PopupBg] = elevatedBg;
		colors[ImGuiCol_Border] = lineColor;
		colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

		colors[ImGuiCol_FrameBg] = appBg;
		colors[ImGuiCol_FrameBgHovered] = surfaceBg;
		colors[ImGuiCol_FrameBgActive] = elevatedBg;

		colors[ImGuiCol_TitleBg] = panelBg;
		colors[ImGuiCol_TitleBgActive] = panelBg;
		colors[ImGuiCol_TitleBgCollapsed] = panelBg;
		colors[ImGuiCol_MenuBarBg] = appBg;

		colors[ImGuiCol_ScrollbarBg] = appBg;
		colors[ImGuiCol_ScrollbarGrab] = surfaceBg;
		colors[ImGuiCol_ScrollbarGrabHovered] = elevatedBg;
		colors[ImGuiCol_ScrollbarGrabActive] = blueAccent;

		colors[ImGuiCol_CheckMark] = blueAccent;
		colors[ImGuiCol_SliderGrab] = blueAccent;
		colors[ImGuiCol_SliderGrabActive] = blueAccent;

		colors[ImGuiCol_Button] = surfaceBg;
		colors[ImGuiCol_ButtonHovered] = elevatedBg;
		colors[ImGuiCol_ButtonActive] = blueAccentSoft;

		colors[ImGuiCol_Header] = surfaceBg;
		colors[ImGuiCol_HeaderHovered] = elevatedBg;
		colors[ImGuiCol_HeaderActive] = blueAccentSoft;

		colors[ImGuiCol_Separator] = lineColor;
		colors[ImGuiCol_SeparatorHovered] = blueAccentSoft;
		colors[ImGuiCol_SeparatorActive] = blueAccent;

		colors[ImGuiCol_ResizeGrip] = blueAccentSoft;
		colors[ImGuiCol_ResizeGripHovered] = blueAccentSoft;
		colors[ImGuiCol_ResizeGripActive] = blueAccent;

		colors[ImGuiCol_Tab] = appBg;
		colors[ImGuiCol_TabHovered] = elevatedBg;
		colors[ImGuiCol_TabActive] = surfaceBg;
		colors[ImGuiCol_TabUnfocused] = appBg;
		colors[ImGuiCol_TabUnfocusedActive] = surfaceBg;

		colors[ImGuiCol_DockingEmptyBg] = appBg;
		colors[ImGuiCol_DockingPreview] = blueAccentSoft;
		colors[ImGuiCol_NavHighlight] = blueAccent;
		colors[ImGuiCol_TextSelectedBg] = blueAccentSoft;
	}
	void ImGuiLayer::SetLightThemeColors()
{
// Fallback empty
}

uint32_t ImGuiLayer::GetActiveWidgetID() const
{
return ImGui::GetActiveID();
}
}


