#pragma once

#include "Ume/Renderer/Buffer.h"
#include "Platform/Vulkan/VulkanContext.h"

namespace Ume {

	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(uint32_t size);
		VulkanVertexBuffer(float* vertices, uint32_t size);
		virtual ~VulkanVertexBuffer();

		virtual void Bind() const override {}
		virtual void Unbind() const override {}

		virtual void SetData(const void* data, uint32_t size) override;

		virtual const BufferLayout& GetLayout() const override { return m_Layout; }
		virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }

		VkBuffer GetVkBuffer() const { return m_Buffer; }

	private:
		void AllocateBuffer(uint32_t size);

	private:
		VkBuffer       m_Buffer       = VK_NULL_HANDLE;
		VkDeviceMemory m_BufferMemory = VK_NULL_HANDLE;

		// Host-visible staging buffer for dynamic updates
		VkBuffer       m_StagingBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory m_StagingBufferMemory  = VK_NULL_HANDLE;
		uint32_t       m_AllocatedSize       = 0;

		BufferLayout   m_Layout;
	};

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(uint32_t* indices, uint32_t count);
		virtual ~VulkanIndexBuffer();

		virtual void Bind() const override {}
		virtual void Unbind() const override {}

		virtual uint32_t GetCount() const override { return m_Count; }

		VkBuffer GetVkBuffer() const { return m_Buffer; }

	private:
		VkBuffer       m_Buffer       = VK_NULL_HANDLE;
		VkDeviceMemory m_BufferMemory = VK_NULL_HANDLE;
		uint32_t       m_Count        = 0;
	};

}
