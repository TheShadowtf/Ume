#include <umepch.h>
#include "Buffer.h"
#include "Renderer.h"
#include "Platform/OpenGL/OpenGLBuffer.h"

namespace Ume
{
	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size)
	{
		switch (Renderer::GetRendererAPI())
		{
		case RendererAPI::API::None:
			UME_CORE_ASSERT(false, "RendererAPI::None is not suported");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLVertexBuffer>(size);
		}
		UME_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size)
	{
		switch (Renderer::GetRendererAPI())
		{
		case RendererAPI::API::None:
			UME_CORE_ASSERT(false, "RendererAPI::None is not suported");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLVertexBuffer>(vertices, size);
		}
		UME_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
	{
		switch (Renderer::GetRendererAPI())
		{
		case RendererAPI::API::None:
			UME_CORE_ASSERT(false, "RendererAPI::None is not suported");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLIndexBuffer>(indices, count);
		}
		UME_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}