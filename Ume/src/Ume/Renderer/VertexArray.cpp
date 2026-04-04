#include "umepch.h"
#include "Ume/Renderer/VertexArray.h"

#include "Ume/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"
#include "Platform/Vulkan/VulkanVertexArray.h"

namespace Ume {

	Ref<VertexArray> VertexArray::Create()
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    UME_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLVertexArray>();
			case RendererAPI::API::Vulkan:  return CreateRef<VulkanVertexArray>();
		}

		UME_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}