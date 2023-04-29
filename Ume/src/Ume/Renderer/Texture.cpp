#include <umepch.h>

#include "Texture.h"
#include "Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"

namespace Ume
{
	Ref<Texture2D> Texture2D::Create(const std::string& path)
	{
		switch (Renderer::GetRendererAPI())
		{
		case RendererAPI::API::None:
			UME_CORE_ASSERT(false, "RendererAPI::None is not suported");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return std::make_shared<OpenGLTexture2D>(path);
		}
		UME_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}