#pragma once

#include <imgui.h>

#include "Ume/Renderer/Framebuffer.h"
#include "Ume/Renderer/RendererAPI.h"
#include "Ume/Renderer/Texture.h"
#include "Platform/Vulkan/VulkanFramebuffer.h"
#include "Platform/Vulkan/VulkanTexture.h"

#include <cstdint>
#include <memory>

namespace Ume::ImGuiTextureHelpers
{
	inline ImTextureID ToImTextureID(uint64_t textureID)
	{
		return reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(textureID));
	}

	inline ImTextureID GetTextureID(const Ref<Texture2D>& texture)
	{
		if (!texture)
			return nullptr;

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			auto vulkanTexture = std::dynamic_pointer_cast<VulkanTexture2D>(texture);
			return vulkanTexture ? ToImTextureID(vulkanTexture->GetImGuiTextureID()) : nullptr;
		}

		return ToImTextureID(texture->GetRendererID());
	}

	inline ImTextureID GetFramebufferAttachmentID(Framebuffer* framebuffer, uint32_t index = 0)
	{
		if (!framebuffer)
			return nullptr;

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			auto* vulkanFramebuffer = dynamic_cast<VulkanFramebuffer*>(framebuffer);
			return vulkanFramebuffer ? ToImTextureID(vulkanFramebuffer->GetColorAttachmentImGuiTextureID(index)) : nullptr;
		}

		return ToImTextureID(framebuffer->GetColorAttachmentRendererID(index));
	}
}