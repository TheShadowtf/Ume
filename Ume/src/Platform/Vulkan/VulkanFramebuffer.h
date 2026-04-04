#pragma once

#include "Ume/Renderer/Framebuffer.h"
#include "Platform/Vulkan/VulkanContext.h"

namespace Ume {

	// Vulkan offscreen framebuffer for Renderer2D scene rendering.
	// The swap chain framebuffer is managed by VulkanContext.
	// This class handles off-screen render targets (editor viewport, etc.)
	class VulkanFramebuffer : public Framebuffer
	{
	public:
		VulkanFramebuffer(const FramebufferSpecification& spec);
		virtual ~VulkanFramebuffer();

		virtual void Bind()   override;
		virtual void Unbind() override;

		virtual void Resize(uint32_t width, uint32_t height) override;
		virtual int  ReadPixel(uint32_t attachmentIndex, int x, int y) override;

		virtual void ClearAttachment(uint32_t attachmentIndex, const glm::vec4& value) override;
		virtual void ClearAttachment(uint32_t attachmentIndex, int value) override;
		virtual void ClearDepthAttachment(float value = 1.0f) override;

		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override;
		uint64_t GetColorAttachmentImGuiTextureID(uint32_t index = 0) const;
		virtual const FramebufferSpecification& GetSpecification() const override { return m_Specification; }

	private:
		struct Attachment
		{
			VkImage        Image       = VK_NULL_HANDLE;
			VkDeviceMemory Memory      = VK_NULL_HANDLE;
			VkImageView    View        = VK_NULL_HANDLE;
			VkSampler      Sampler     = VK_NULL_HANDLE;
			uint32_t       RendererID  = 0;
			VkFormat       Format      = VK_FORMAT_UNDEFINED;
			mutable VkDescriptorSet ImGuiDescriptorSet = VK_NULL_HANDLE;
		};

		void CreateAttachmentSampler(Attachment& attachment);
		void ReleaseAttachmentImGuiTexture(Attachment& attachment);
		void Invalidate();
		void Cleanup();

	private:
		FramebufferSpecification m_Specification;

		std::vector<Attachment> m_ColorAttachments;
		Attachment              m_DepthAttachment;

		VkRenderPass  m_RenderPass  = VK_NULL_HANDLE;
		VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;
	};

}
