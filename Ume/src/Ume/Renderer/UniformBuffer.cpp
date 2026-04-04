#include "umepch.h"
#include "UniformBuffer.h"

#include "Ume/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLUniformBuffer.h"
#include "Platform/Vulkan/VulkanUniformBuffer.h"

namespace Ume {

	Ref<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    UME_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLUniformBuffer>(size, binding);
			case RendererAPI::API::Vulkan:  return CreateRef<VulkanUniformBuffer>(size, binding);
		}

		UME_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
