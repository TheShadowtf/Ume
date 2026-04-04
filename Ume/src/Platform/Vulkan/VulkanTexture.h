#pragma once

#include "Ume/Renderer/Texture.h"
#include "Platform/Vulkan/VulkanContext.h"

namespace Ume {

	class VulkanTexture2D : public Texture2D
	{
	public:
		VulkanTexture2D(const TextureSpecification& spec);
		VulkanTexture2D(const std::string& path);
		virtual ~VulkanTexture2D();

		virtual const TextureSpecification& GetSpecification() const override { return m_Specification; }
		virtual uint32_t GetWidth()      const override { return m_Specification.Width; }
		virtual uint32_t GetHeight()     const override { return m_Specification.Height; }
		virtual uint32_t GetRendererID() const override { return m_RendererID; }
		virtual const std::string& GetPath() const override { return m_Path; }
		uint64_t GetImGuiTextureID() const;

		virtual void SetData(void* data, uint32_t size) override;
		virtual void Bind(uint32_t slot = 0) const override;
		virtual bool IsLoaded() const override { return m_IsLoaded; }

		virtual bool operator==(const Texture& other) const override
		{
			return m_RendererID == other.GetRendererID();
		}

		VkImageView   GetImageView() const { return m_ImageView; }
		VkSampler     GetSampler()   const { return m_Sampler; }

	private:
		void ReleaseImGuiTexture();
		void CreateTextureImage(const void* data, uint32_t width, uint32_t height, VkFormat format, VkDeviceSize imageSize);
		void CreateSampler();

	private:
		TextureSpecification m_Specification;
		std::string          m_Path;
		bool                 m_IsLoaded = false;
		uint32_t             m_RendererID = 0;

		static uint32_t      s_NextID;

		VkImage        m_Image       = VK_NULL_HANDLE;
		VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;
		VkImageView    m_ImageView   = VK_NULL_HANDLE;
		VkSampler      m_Sampler     = VK_NULL_HANDLE;
		mutable VkDescriptorSet m_ImGuiDescriptorSet = VK_NULL_HANDLE;
	};

}
