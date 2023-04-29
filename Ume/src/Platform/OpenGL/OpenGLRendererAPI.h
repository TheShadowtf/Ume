#pragma once

#include "Ume/Renderer/RendererAPI.h"

namespace Ume
{
	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;

		virtual void Clear() override;
		virtual void Clear(const glm::vec4& color) override;

		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		virtual void DrawIndex(const Ref<VertexArray>& vertexArray) override;

	};
}