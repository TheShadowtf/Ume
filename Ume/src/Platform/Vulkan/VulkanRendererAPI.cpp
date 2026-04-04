#include "umepch.h"
#include "Platform/Vulkan/VulkanRendererAPI.h"
#include "Platform/Vulkan/VulkanBuffer.h"
#include "Platform/Vulkan/VulkanVertexArray.h"
#include "Ume/Renderer/Buffer.h"

namespace Ume {

	void VulkanRendererAPI::Init()
	{
		UME_CORE_INFO("Vulkan RendererAPI initialized.");
		// Per-frame state reset is handled by VulkanContext::BeginFrame().
	}

	void VulkanRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		auto* ctx = VulkanContext::Get();
		if (!ctx) return;

		VkCommandBuffer cmd = ctx->GetCurrentCommandBuffer();

		VkViewport viewport{};
		viewport.x        = (float)x;
		viewport.y        = (float)y;
		viewport.width    = (float)width;
		viewport.height   = (float)height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset.x      = (int32_t)x;
		scissor.offset.y      = (int32_t)y;
		scissor.extent.width  = width;
		scissor.extent.height = height;
		vkCmdSetScissor(cmd, 0, 1, &scissor);
	}

	void VulkanRendererAPI::SetClearColor(const glm::vec4& color)
	{
		m_ClearColor = color;
	}

	void VulkanRendererAPI::Clear()
	{
		auto* ctx = VulkanContext::Get();
		if (!ctx) return;

		VkCommandBuffer cmd = ctx->GetCurrentCommandBuffer();
		VkExtent2D ext      = ctx->GetSwapChainExtent();

		VkClearAttachment clearAtt{};
		clearAtt.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
		clearAtt.colorAttachment = 0;
		clearAtt.clearValue.color = { { m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a } };

		VkClearAttachment depthAtt{};
		depthAtt.aspectMask               = VK_IMAGE_ASPECT_DEPTH_BIT;
		depthAtt.clearValue.depthStencil  = { 1.0f, 0 };

		VkClearRect clearRect{};
		clearRect.rect.offset        = { 0, 0 };
		clearRect.rect.extent        = ext;
		clearRect.baseArrayLayer     = 0;
		clearRect.layerCount         = 1;

		std::array<VkClearAttachment, 2> atts = { clearAtt, depthAtt };
		vkCmdClearAttachments(cmd, (uint32_t)atts.size(), atts.data(), 1, &clearRect);
	}

	void VulkanRendererAPI::SetBlendMode(BlendMode)  {}
	void VulkanRendererAPI::SetDepthTest(bool)        {}
	void VulkanRendererAPI::SetDepthWrite(bool)       {}
	void VulkanRendererAPI::SetCull(bool)             {}
	void VulkanRendererAPI::SetLineWidth(float)       {}

	void VulkanRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
	{
		auto* ctx       = VulkanContext::Get();
		if (!ctx) return;
		VkCommandBuffer cmd = ctx->GetCurrentCommandBuffer();

		const auto& vbs = vertexArray->GetVertexBuffers();
		for (uint32_t i = 0; i < (uint32_t)vbs.size(); i++)
		{
			auto* vb = static_cast<VulkanVertexBuffer*>(vbs[i].get());
			VkBuffer buf = vb->GetVkBuffer();
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, i, 1, &buf, &offset);
		}

		auto* ib = static_cast<VulkanIndexBuffer*>(vertexArray->GetIndexBuffer().get());
		vkCmdBindIndexBuffer(cmd, ib->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT32);

		uint32_t count = (indexCount == 0) ? vertexArray->GetIndexBuffer()->GetCount() : indexCount;
		vkCmdDrawIndexed(cmd, count, 1, 0, 0, 0);
	}

	void VulkanRendererAPI::DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
	{
		auto* ctx       = VulkanContext::Get();
		if (!ctx) return;
		VkCommandBuffer cmd = ctx->GetCurrentCommandBuffer();

		const auto& vbs = vertexArray->GetVertexBuffers();
		for (uint32_t i = 0; i < (uint32_t)vbs.size(); i++)
		{
			auto* vb = static_cast<VulkanVertexBuffer*>(vbs[i].get());
			VkBuffer buf = vb->GetVkBuffer();
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, i, 1, &buf, &offset);
		}

		vkCmdDraw(cmd, vertexCount, 1, 0, 0);
	}

} // namespace Ume
