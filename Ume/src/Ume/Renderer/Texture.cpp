#include <umepch.h>

#include "Texture.h"
#include "Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"

namespace Ume
{
	Ume::Ref<Ume::Texture2D> Texture2D::Create(uint32_t width, uint32_t height)
	{
		switch (Renderer::GetRendererAPI())
		{
		case RendererAPI::API::None:
			UME_CORE_ASSERT(false, "RendererAPI::None is not suported");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLTexture2D>(width, height);
		}
		UME_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const std::string& path)
	{
		switch (Renderer::GetRendererAPI())
		{
		case RendererAPI::API::None:
			UME_CORE_ASSERT(false, "RendererAPI::None is not suported");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLTexture2D>(path);
		}
		UME_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}