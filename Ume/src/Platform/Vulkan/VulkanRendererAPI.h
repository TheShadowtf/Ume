#pragma once

#include "Ume/Renderer/RendererAPI.h"
#include "Platform/Vulkan/VulkanContext.h"

namespace Ume {

	class VulkanRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;

		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		virtual void SetClearColor(const glm::vec4& color) override;
		virtual void Clear() override;

		virtual void SetBlendMode(BlendMode mode) override;
		virtual void SetDepthTest(bool enabled)  override;
		virtual void SetDepthWrite(bool enabled) override;
		virtual void SetCull(bool enabled)       override;

		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
		virtual void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)      override;

		virtual void SetLineWidth(float width) override;

	private:
		glm::vec4 m_ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
	};

}
