#include "umepch.h"
#include "Platform/Vulkan/VulkanTexture.h"

#include <examples/imgui_impl_vulkan.h>

#include <algorithm>
#include <stb_image.h>
#include <vector>

namespace Ume {

	uint32_t VulkanTexture2D::s_NextID = 1;

	static uint32_t SourceBytesPerPixel(ImageFormat fmt)
	{
		switch (fmt)
		{
			case ImageFormat::R8:      return 1;
			case ImageFormat::RGB8:    return 3;
			case ImageFormat::RGBA8:   return 4;
			case ImageFormat::RGBA32F: return 16;
			default: break;
		}
		UME_CORE_ASSERT(false, "Unknown ImageFormat!");
		return 0;
	}

	static uint32_t UploadBytesPerPixel(ImageFormat fmt)
	{
		switch (fmt)
		{
			case ImageFormat::RGB8:
				return 4;
			default:
				return SourceBytesPerPixel(fmt);
		}
	}

	static std::vector<uint8_t> ExpandTextureDataForUpload(const void* data, uint32_t width, uint32_t height, ImageFormat format)
	{
		const size_t pixelCount = static_cast<size_t>(width) * height;

		switch (format)
		{
			case ImageFormat::RGB8:
			{
				const auto* source = static_cast<const uint8_t*>(data);
				std::vector<uint8_t> expanded(pixelCount * 4);
				for (size_t index = 0; index < pixelCount; index++)
				{
					expanded[index * 4 + 0] = source[index * 3 + 0];
					expanded[index * 4 + 1] = source[index * 3 + 1];
					expanded[index * 4 + 2] = source[index * 3 + 2];
					expanded[index * 4 + 3] = 255;
				}
				return expanded;
			}
			default:
			{
				const size_t uploadSize = pixelCount * UploadBytesPerPixel(format);
				std::vector<uint8_t> expanded(uploadSize);
				memcpy(expanded.data(), data, uploadSize);
				return expanded;
			}
		}
	}

	static std::vector<uint8_t> CreateInitialTextureData(uint32_t width, uint32_t height, ImageFormat format)
	{
		const size_t uploadSize = static_cast<size_t>(width) * height * UploadBytesPerPixel(format);
		std::vector<uint8_t> data(uploadSize);

		if (format == ImageFormat::RGBA32F)
		{
			auto* floats = reinterpret_cast<float*>(data.data());
			const size_t floatCount = uploadSize / sizeof(float);
			std::fill(floats, floats + floatCount, 1.0f);
		}
		else
		{
			std::fill(data.begin(), data.end(), 0xFF);
		}

		return data;
	}

	static VkFormat ImageFormatToVkFormat(ImageFormat fmt)
	{
		switch (fmt)
		{
			case ImageFormat::R8:     return VK_FORMAT_R8_UNORM;
			case ImageFormat::RGB8:   return VK_FORMAT_R8G8B8A8_UNORM; // Upgrade to RGBA for alignment
			case ImageFormat::RGBA8:  return VK_FORMAT_R8G8B8A8_UNORM;
			case ImageFormat::RGBA32F:return VK_FORMAT_R32G32B32A32_SFLOAT;
			default: break;
		}
		UME_CORE_ASSERT(false, "Unknown ImageFormat!");
		return VK_FORMAT_UNDEFINED;
	}

	// -----------------------------------------------------------------------

	VulkanTexture2D::VulkanTexture2D(const TextureSpecification& spec)
		: m_Specification(spec)
	{
		m_RendererID = s_NextID++;
		VkFormat fmt = ImageFormatToVkFormat(spec.Format);

		auto initialData = CreateInitialTextureData(spec.Width, spec.Height, spec.Format);
		CreateTextureImage(initialData.data(), spec.Width, spec.Height, fmt, static_cast<VkDeviceSize>(initialData.size()));
		CreateSampler();
		m_IsLoaded = true;
	}

	VulkanTexture2D::VulkanTexture2D(const std::string& path)
		: m_Path(path)
	{
		m_RendererID = s_NextID++;

		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

		if (data)
		{
			m_Specification.Width  = (uint32_t)width;
			m_Specification.Height = (uint32_t)height;
			m_Specification.Format = ImageFormat::RGBA8;

			CreateTextureImage(data, (uint32_t)width, (uint32_t)height, VK_FORMAT_R8G8B8A8_UNORM, static_cast<VkDeviceSize>(width) * height * 4);
			CreateSampler();

			stbi_image_free(data);
			m_IsLoaded = true;
		}
		else
		{
			UME_CORE_ERROR("VulkanTexture2D: failed to load '{0}'", path);
			// Fallback 1×1 white
			m_Specification.Width  = 1;
			m_Specification.Height = 1;
			m_Specification.Format = ImageFormat::RGBA8;
			uint32_t white = 0xFFFFFFFF;
			CreateTextureImage(&white, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, sizeof(white));
			CreateSampler();
		}
	}

	VulkanTexture2D::~VulkanTexture2D()
	{
		auto* ctx    = VulkanContext::Get();
		if (!ctx)
			return;
		VkDevice dev = ctx->GetDevice();
		vkDeviceWaitIdle(dev);
		ReleaseImGuiTexture();

		vkDestroySampler(dev, m_Sampler, nullptr);
		vkDestroyImageView(dev, m_ImageView, nullptr);
		vkDestroyImage(dev, m_Image, nullptr);
		vkFreeMemory(dev, m_ImageMemory, nullptr);
	}

	uint64_t VulkanTexture2D::GetImGuiTextureID() const
	{
		if (m_ImGuiDescriptorSet == VK_NULL_HANDLE && m_Sampler != VK_NULL_HANDLE && m_ImageView != VK_NULL_HANDLE)
			m_ImGuiDescriptorSet = ImGui_ImplVulkan_AddTexture(m_Sampler, m_ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		return reinterpret_cast<uint64_t>(m_ImGuiDescriptorSet);
	}

	void VulkanTexture2D::ReleaseImGuiTexture()
	{
		if (m_ImGuiDescriptorSet == VK_NULL_HANDLE)
			return;

		ImGui_ImplVulkan_RemoveTexture(m_ImGuiDescriptorSet);
		m_ImGuiDescriptorSet = VK_NULL_HANDLE;
	}

	void VulkanTexture2D::CreateTextureImage(const void* data, uint32_t width, uint32_t height, VkFormat format, VkDeviceSize imageSize)
	{
		auto* ctx    = VulkanContext::Get();
		VkDevice dev = ctx->GetDevice();

		VkBuffer       stagingBuffer;
		VkDeviceMemory stagingMemory;
		ctx->CreateBuffer(imageSize,
		    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		    stagingBuffer, stagingMemory);

		void* mapped;
		vkMapMemory(dev, stagingMemory, 0, imageSize, 0, &mapped);
		memcpy(mapped, data, (size_t)imageSize);
		vkUnmapMemory(dev, stagingMemory);

		ctx->CreateImage(width, height, format,
		    VK_IMAGE_TILING_OPTIMAL,
		    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		    m_Image, m_ImageMemory);

		ctx->TransitionImageLayout(m_Image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		ctx->CopyBufferToImage(stagingBuffer, m_Image, width, height);
		ctx->TransitionImageLayout(m_Image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(dev, stagingBuffer, nullptr);
		vkFreeMemory(dev, stagingMemory, nullptr);

		m_ImageView = ctx->CreateImageView(m_Image, format, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void VulkanTexture2D::CreateSampler()
	{
		auto* ctx    = VulkanContext::Get();
		VkDevice dev = ctx->GetDevice();

		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(ctx->GetPhysicalDevice(), &props);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter               = VK_FILTER_LINEAR;
		samplerInfo.minFilter               = VK_FILTER_LINEAR;
		samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable        = VK_TRUE;
		samplerInfo.maxAnisotropy           = props.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable           = VK_FALSE;
		samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		VK_CHECK(vkCreateSampler(dev, &samplerInfo, nullptr, &m_Sampler));
	}

	void VulkanTexture2D::SetData(void* data, uint32_t size)
	{
		const uint32_t expectedSize = m_Specification.Width * m_Specification.Height * SourceBytesPerPixel(m_Specification.Format);
		UME_CORE_ASSERT(size == expectedSize, "Data must be entire texture!");

		auto uploadData = ExpandTextureDataForUpload(data, m_Specification.Width, m_Specification.Height, m_Specification.Format);

		// Rebuild image with new data
		auto* ctx    = VulkanContext::Get();
		VkDevice dev = ctx->GetDevice();
		vkDeviceWaitIdle(dev);
		ReleaseImGuiTexture();

		vkDestroyImageView(dev, m_ImageView, nullptr);
		vkDestroyImage(dev, m_Image, nullptr);
		vkFreeMemory(dev, m_ImageMemory, nullptr);

		VkFormat fmt = ImageFormatToVkFormat(m_Specification.Format);
		CreateTextureImage(uploadData.data(), m_Specification.Width, m_Specification.Height, fmt, static_cast<VkDeviceSize>(uploadData.size()));
	}

	void VulkanTexture2D::Bind(uint32_t slot) const
	{
		// Texture binding in Vulkan happens through descriptor sets, not per-slot binding.
		// The VulkanRendererAPI manages descriptor set updates.
		(void)slot;
	}

} // namespace Ume
