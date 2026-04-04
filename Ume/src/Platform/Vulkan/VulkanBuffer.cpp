#include "umepch.h"
#include "Platform/Vulkan/VulkanBuffer.h"

namespace Ume {

	// -----------------------------------------------------------------------
	// VulkanVertexBuffer
	// -----------------------------------------------------------------------

	VulkanVertexBuffer::VulkanVertexBuffer(uint32_t size)
	{
		AllocateBuffer(size);
	}

	VulkanVertexBuffer::VulkanVertexBuffer(float* vertices, uint32_t size)
	{
		AllocateBuffer(size);
		SetData(vertices, size);
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		auto* ctx = VulkanContext::Get();
		VkDevice device = ctx->GetDevice();
		vkDeviceWaitIdle(device);

		if (m_Buffer       != VK_NULL_HANDLE) vkDestroyBuffer(device, m_Buffer, nullptr);
		if (m_BufferMemory != VK_NULL_HANDLE) vkFreeMemory(device, m_BufferMemory, nullptr);

		if (m_StagingBuffer       != VK_NULL_HANDLE) vkDestroyBuffer(device, m_StagingBuffer, nullptr);
		if (m_StagingBufferMemory != VK_NULL_HANDLE) vkFreeMemory(device, m_StagingBufferMemory, nullptr);
	}

	void VulkanVertexBuffer::AllocateBuffer(uint32_t size)
	{
		auto* ctx = VulkanContext::Get();
		m_AllocatedSize = size;

		// Create host-visible staging buffer
		ctx->CreateBuffer(size,
		    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		    m_StagingBuffer, m_StagingBufferMemory);

		// Create device-local buffer
		ctx->CreateBuffer(size,
		    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		    m_Buffer, m_BufferMemory);
	}

	void VulkanVertexBuffer::SetData(const void* data, uint32_t size)
	{
		auto* ctx    = VulkanContext::Get();
		VkDevice dev = ctx->GetDevice();

		void* mapped;
		vkMapMemory(dev, m_StagingBufferMemory, 0, size, 0, &mapped);
		memcpy(mapped, data, size);
		vkUnmapMemory(dev, m_StagingBufferMemory);

		ctx->CopyBuffer(m_StagingBuffer, m_Buffer, size);
	}

	// -----------------------------------------------------------------------
	// VulkanIndexBuffer
	// -----------------------------------------------------------------------

	VulkanIndexBuffer::VulkanIndexBuffer(uint32_t* indices, uint32_t count)
		: m_Count(count)
	{
		auto* ctx = VulkanContext::Get();
		VkDeviceSize bufferSize = sizeof(uint32_t) * count;

		VkBuffer       stagingBuffer;
		VkDeviceMemory stagingMemory;
		ctx->CreateBuffer(bufferSize,
		    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		    stagingBuffer, stagingMemory);

		VkDevice dev = ctx->GetDevice();
		void* mapped;
		vkMapMemory(dev, stagingMemory, 0, bufferSize, 0, &mapped);
		memcpy(mapped, indices, (size_t)bufferSize);
		vkUnmapMemory(dev, stagingMemory);

		ctx->CreateBuffer(bufferSize,
		    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		    m_Buffer, m_BufferMemory);

		ctx->CopyBuffer(stagingBuffer, m_Buffer, bufferSize);

		vkDestroyBuffer(dev, stagingBuffer, nullptr);
		vkFreeMemory(dev, stagingMemory, nullptr);
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		auto* ctx = VulkanContext::Get();
		VkDevice device = ctx->GetDevice();
		vkDeviceWaitIdle(device);
		vkDestroyBuffer(device, m_Buffer, nullptr);
		vkFreeMemory(device, m_BufferMemory, nullptr);
	}

} // namespace Ume
