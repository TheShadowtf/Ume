#include "umepch.h"

#include "Buffer.h"
#include "Renderer.h"
#include "Ume/Renderer/VertexArray.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace Ume
{
	VertexArray* VertexArray::Create()
	{
		switch (Renderer::GetRendererAPI())
		{
		case RendererAPI::API::None:
			UME_CORE_ASSERT(false, "RendererAPI::None is not suported");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return new OpenGLVertexArray();
		}
		UME_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}