#pragma once

#include "RendererAPI.h"

namespace Ume
{
	class RendererCommand
	{
	public:
		inline static void Init()
		{
			s_RendererAPI->Init();
		}

		inline static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{

			s_RendererAPI->SetViewport(x, y, width, height);
		}

		inline static void Clear()
		{
			s_RendererAPI->Clear();
		}
		inline static void Clear(const glm::vec4& color)
		{
			s_RendererAPI->Clear(color);
		}

		inline static void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCound = 0) { s_RendererAPI->DrawIndex(vertexArray, indexCound); }
	private:
		static RendererAPI* s_RendererAPI;
	};
}