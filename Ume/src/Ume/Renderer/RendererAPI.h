#pragma once

#include "glm/glm.hpp"
#include "Ume/Renderer/VertexArray.h"

namespace Ume
{
	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0,
			OpenGL = 1
		};
	public:
		virtual ~RendererAPI() = default;
		virtual void Init() = 0;
		virtual void Clear() = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void Clear(const glm::vec4& color) = 0;

		virtual void DrawIndex(const Ref<VertexArray>& vertexArray, uint32_t indexCound = 0) = 0;

		inline static API GetApi() { return s_API; }
	private:
		static API s_API;
	};
}