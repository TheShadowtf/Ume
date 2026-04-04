#pragma once

#include "Ume/Renderer/UniformBuffer.h"
#include "Platform/Vulkan/VulkanContext.h"

namespace Ume {

	class VulkanUniformBuffer : public UniformBuffer
	{
	public:
		VulkanUniformBuffer(uint32_t size, uint32_t binding);
		virtual ~VulkanUniformBuffer();

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;

		VkBuffer GetBuffer(uint32_t frameIndex) const { return m_Buffers[frameIndex]; }
		uint32_t GetBinding() const { return m_Binding; }
		uint32_t GetSize()    const { return m_Size; }

	private:
		std::vector<VkBuffer>       m_Buffers;
		std::vector<VkDeviceMemory> m_BufferMemories;
		std::vector<void*>          m_MappedMemories;
		uint32_t m_Size    = 0;
		uint32_t m_Binding = 0;
	};

}
