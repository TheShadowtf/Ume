#include "umepch.h"
#include "Platform/Vulkan/VulkanUniformBuffer.h"

namespace Ume {

	VulkanUniformBuffer::VulkanUniformBuffer(uint32_t size, uint32_t binding)
		: m_Size(size), m_Binding(binding)
	{
		auto* ctx     = VulkanContext::Get();
		uint32_t count = ctx->GetImageCount();

		m_Buffers.resize(count);
		m_BufferMemories.resize(count);
		m_MappedMemories.resize(count);

		for (uint32_t i = 0; i < count; i++)
		{
			ctx->CreateBuffer(size,
			    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			    m_Buffers[i], m_BufferMemories[i]);

			vkMapMemory(ctx->GetDevice(), m_BufferMemories[i], 0, size, 0, &m_MappedMemories[i]);
		}
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
		auto* ctx    = VulkanContext::Get();
		VkDevice dev = ctx->GetDevice();
		vkDeviceWaitIdle(dev);

		for (size_t i = 0; i < m_Buffers.size(); i++)
		{
			vkUnmapMemory(dev, m_BufferMemories[i]);
			vkDestroyBuffer(dev, m_Buffers[i], nullptr);
			vkFreeMemory(dev, m_BufferMemories[i], nullptr);
		}
	}

	void VulkanUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		auto* ctx = VulkanContext::Get();
		uint32_t frame = ctx->GetCurrentFrame();
		memcpy((uint8_t*)m_MappedMemories[frame] + offset, data, size);
	}

} // namespace Ume
