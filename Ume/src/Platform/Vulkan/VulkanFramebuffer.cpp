#include "umepch.h"
#include "Platform/Vulkan/VulkanFramebuffer.h"

#include <examples/imgui_impl_vulkan.h>

namespace Ume {

	static uint32_t s_FBRendererIDCounter = 1;

	VulkanFramebuffer::VulkanFramebuffer(const FramebufferSpecification& spec)
		: m_Specification(spec)
	{
		Invalidate();
	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{
		Cleanup();
	}

	void VulkanFramebuffer::Cleanup()
	{
		auto* ctx    = VulkanContext::Get();
		if (!ctx)
			return;
		VkDevice dev = ctx->GetDevice();
		vkDeviceWaitIdle(dev);

		for (auto& att : m_ColorAttachments)
		{
			ReleaseAttachmentImGuiTexture(att);
			if (att.Sampler != VK_NULL_HANDLE)
				vkDestroySampler(dev, att.Sampler, nullptr);
			vkDestroyImageView(dev, att.View, nullptr);
			vkDestroyImage(dev, att.Image, nullptr);
			vkFreeMemory(dev, att.Memory, nullptr);
		}
		m_ColorAttachments.clear();

		if (m_DepthAttachment.Image != VK_NULL_HANDLE)
		{
			if (m_DepthAttachment.Sampler != VK_NULL_HANDLE)
				vkDestroySampler(dev, m_DepthAttachment.Sampler, nullptr);
			vkDestroyImageView(dev, m_DepthAttachment.View, nullptr);
			vkDestroyImage(dev, m_DepthAttachment.Image, nullptr);
			vkFreeMemory(dev, m_DepthAttachment.Memory, nullptr);
			m_DepthAttachment = {};
		}

		if (m_Framebuffer  != VK_NULL_HANDLE) { vkDestroyFramebuffer(dev, m_Framebuffer, nullptr);  m_Framebuffer  = VK_NULL_HANDLE; }
		if (m_RenderPass   != VK_NULL_HANDLE) { vkDestroyRenderPass(dev, m_RenderPass, nullptr);    m_RenderPass   = VK_NULL_HANDLE; }
	}

	void VulkanFramebuffer::Invalidate()
	{
		auto* ctx    = VulkanContext::Get();
		VkDevice dev = ctx->GetDevice();

		Cleanup();

		uint32_t w = m_Specification.Width;
		uint32_t h = m_Specification.Height;
		if (w == 0 || h == 0) return;

		std::vector<VkAttachmentDescription> attachmentDescs;
		std::vector<VkAttachmentReference>   colorRefs;
		bool hasDepth = false;
		VkAttachmentReference depthRef{};

		// Color attachments
		for (auto& texSpec : m_Specification.Attachments.Attachments)
		{
			bool isColor = (texSpec.TextureFormat != FramebufferTextureFormat::DEPTH24STENCIL8);

			if (isColor)
			{
				// RED_INTEGER uses integer format (entity ID picking); everything else is RGBA8
				VkFormat fmt = (texSpec.TextureFormat == FramebufferTextureFormat::RED_INTEGER)
				    ? VK_FORMAT_R32_SINT
				    : VK_FORMAT_R8G8B8A8_UNORM;

				Attachment att;
				att.RendererID = s_FBRendererIDCounter++;

				VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				if (fmt == VK_FORMAT_R32_SINT)
					usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

				ctx->CreateImage(w, h, fmt,
				    VK_IMAGE_TILING_OPTIMAL,
				    usage,
				    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				    att.Image, att.Memory);

				att.View = ctx->CreateImageView(att.Image, fmt, VK_IMAGE_ASPECT_COLOR_BIT);
				CreateAttachmentSampler(att);
				att.Format = fmt;
				m_ColorAttachments.push_back(att);

				VkAttachmentDescription desc{};
				desc.format        = fmt;
				desc.samples       = VK_SAMPLE_COUNT_1_BIT;
				desc.loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
				desc.storeOp       = VK_ATTACHMENT_STORE_OP_STORE;
				desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				desc.finalLayout   = (fmt == VK_FORMAT_R32_SINT)
				    ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
				    : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				attachmentDescs.push_back(desc);

				VkAttachmentReference ref{};
				ref.attachment = (uint32_t)(attachmentDescs.size() - 1);
				ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colorRefs.push_back(ref);
			}
			else
			{
				hasDepth = true;
				VkFormat depthFmt = ctx->FindSupportedFormat(
				    { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
				    VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

				m_DepthAttachment.RendererID = s_FBRendererIDCounter++;
				ctx->CreateImage(w, h, depthFmt,
				    VK_IMAGE_TILING_OPTIMAL,
				    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				    m_DepthAttachment.Image, m_DepthAttachment.Memory);

				m_DepthAttachment.View = ctx->CreateImageView(m_DepthAttachment.Image, depthFmt, VK_IMAGE_ASPECT_DEPTH_BIT);

				VkAttachmentDescription desc{};
				desc.format        = depthFmt;
				desc.samples       = VK_SAMPLE_COUNT_1_BIT;
				desc.loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
				desc.storeOp       = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				desc.finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				attachmentDescs.push_back(desc);

				depthRef.attachment = (uint32_t)(attachmentDescs.size() - 1);
				depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
		}

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount    = (uint32_t)colorRefs.size();
		subpass.pColorAttachments       = colorRefs.empty() ? nullptr : colorRefs.data();
		subpass.pDepthStencilAttachment = hasDepth ? &depthRef : nullptr;

		VkSubpassDependency dep{};
		dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
		dep.dstSubpass    = 0;
		dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dep.srcAccessMask = 0;
		dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo rpInfo{};
		rpInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rpInfo.attachmentCount = (uint32_t)attachmentDescs.size();
		rpInfo.pAttachments    = attachmentDescs.data();
		rpInfo.subpassCount    = 1;
		rpInfo.pSubpasses      = &subpass;
		rpInfo.dependencyCount = 1;
		rpInfo.pDependencies   = &dep;

		VK_CHECK(vkCreateRenderPass(dev, &rpInfo, nullptr, &m_RenderPass));

		// Collect image views
		std::vector<VkImageView> views;
		for (auto& att : m_ColorAttachments) views.push_back(att.View);
		if (hasDepth) views.push_back(m_DepthAttachment.View);

		VkFramebufferCreateInfo fbInfo{};
		fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbInfo.renderPass      = m_RenderPass;
		fbInfo.attachmentCount = (uint32_t)views.size();
		fbInfo.pAttachments    = views.data();
		fbInfo.width           = w;
		fbInfo.height          = h;
		fbInfo.layers          = 1;

		VK_CHECK(vkCreateFramebuffer(dev, &fbInfo, nullptr, &m_Framebuffer));
	}

	void VulkanFramebuffer::Bind()
	{
		// Off-screen rendering via secondary command buffer or inline render pass
		// For editor viewport, we start a new render pass on the offscreen framebuffer.
		VkCommandBuffer cmd = VulkanContext::Get()->GetCurrentCommandBuffer();

		VkRenderPassBeginInfo rpInfo{};
		rpInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rpInfo.renderPass        = m_RenderPass;
		rpInfo.framebuffer       = m_Framebuffer;
		rpInfo.renderArea.offset = { 0, 0 };
		rpInfo.renderArea.extent = { m_Specification.Width, m_Specification.Height };

		std::vector<VkClearValue> clearValues;
		clearValues.reserve(m_ColorAttachments.size() + (m_DepthAttachment.Image != VK_NULL_HANDLE ? 1 : 0));

		for (const auto& attachment : m_ColorAttachments)
		{
			VkClearValue clearValue{};
			if (attachment.Format == VK_FORMAT_R32_SINT)
				clearValue.color.int32[0] = -1;
			else
				clearValue.color = { { 0.1f, 0.1f, 0.1f, 1.0f } };
			clearValues.push_back(clearValue);
		}

		if (m_DepthAttachment.Image != VK_NULL_HANDLE)
		{
			VkClearValue depthClear{};
			depthClear.depthStencil = { 1.0f, 0 };
			clearValues.push_back(depthClear);
		}

		rpInfo.clearValueCount = (uint32_t)clearValues.size();
		rpInfo.pClearValues    = clearValues.data();

		vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport vp{};
		vp.width    = (float)m_Specification.Width;
		vp.height   = (float)m_Specification.Height;
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;
		vkCmdSetViewport(cmd, 0, 1, &vp);

		VkRect2D scissor{};
		scissor.extent = { m_Specification.Width, m_Specification.Height };
		vkCmdSetScissor(cmd, 0, 1, &scissor);
	}

	void VulkanFramebuffer::Unbind()
	{
		VkCommandBuffer cmd = VulkanContext::Get()->GetCurrentCommandBuffer();
		vkCmdEndRenderPass(cmd);
	}

	void VulkanFramebuffer::Resize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0) return;
		if (width == m_Specification.Width && height == m_Specification.Height) return;
		m_Specification.Width  = width;
		m_Specification.Height = height;
		VulkanContext::Get()->GetDevice(); // ensure context alive
		Invalidate();
	}

	int VulkanFramebuffer::ReadPixel(uint32_t, int, int)
	{
		// Vulkan readback requires a host-visible image and a layout transition.
		// Not yet implemented — return -1 (no entity hit).
		return -1;
	}

	void VulkanFramebuffer::ClearAttachment(uint32_t, const glm::vec4&) {}
	void VulkanFramebuffer::ClearAttachment(uint32_t, int) {}
	void VulkanFramebuffer::ClearDepthAttachment(float) {}

	void VulkanFramebuffer::CreateAttachmentSampler(Attachment& attachment)
	{
		auto* ctx = VulkanContext::Get();
		VkDevice dev = ctx->GetDevice();

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		VK_CHECK(vkCreateSampler(dev, &samplerInfo, nullptr, &attachment.Sampler));
	}

	void VulkanFramebuffer::ReleaseAttachmentImGuiTexture(Attachment& attachment)
	{
		if (attachment.ImGuiDescriptorSet == VK_NULL_HANDLE)
			return;

		ImGui_ImplVulkan_RemoveTexture(attachment.ImGuiDescriptorSet);
		attachment.ImGuiDescriptorSet = VK_NULL_HANDLE;
	}

	uint32_t VulkanFramebuffer::GetColorAttachmentRendererID(uint32_t index) const
	{
		UME_CORE_ASSERT(index < m_ColorAttachments.size(), "Index out of range");
		return m_ColorAttachments[index].RendererID;
	}

	uint64_t VulkanFramebuffer::GetColorAttachmentImGuiTextureID(uint32_t index) const
	{
		UME_CORE_ASSERT(index < m_ColorAttachments.size(), "Index out of range");
		auto& attachment = m_ColorAttachments[index];

		if (attachment.ImGuiDescriptorSet == VK_NULL_HANDLE && attachment.Sampler != VK_NULL_HANDLE && attachment.View != VK_NULL_HANDLE)
			attachment.ImGuiDescriptorSet = ImGui_ImplVulkan_AddTexture(attachment.Sampler, attachment.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		return reinterpret_cast<uint64_t>(attachment.ImGuiDescriptorSet);
	}

} // namespace Ume
