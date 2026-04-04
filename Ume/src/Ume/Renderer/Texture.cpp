#include "umepch.h"
#include "Ume/Renderer/Texture.h"

#include "Ume/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"
#include "Platform/Vulkan/VulkanTexture.h"
#include "Ume/Asset/VPakReader.h"

namespace Ume {

	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    UME_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(specification);
			case RendererAPI::API::Vulkan:  return CreateRef<VulkanTexture2D>(specification);
		}

		UME_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const std::string& path)
	{
		// Check VPakRegistry first: try full path, then filename-only fallback.
		if (VPakRegistry::Contains(path))
			return VPakRegistry::FindTexture(path);

		const std::string filename = std::filesystem::path(path).filename().string();
		if (VPakRegistry::Contains(filename))
			return VPakRegistry::FindTexture(filename);

		// Fall back to filesystem load.
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    UME_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(path);
			case RendererAPI::API::Vulkan:  return CreateRef<VulkanTexture2D>(path);
		}

		UME_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
