#pragma once

#include "Ume/Renderer/VertexArray.h"

namespace Ume {

	// Vulkan has no VAO concept — this class simply holds vertex/index buffer refs
	// so the rest of Renderer2D code works unchanged.
	class VulkanVertexArray : public VertexArray
	{
	public:
		VulkanVertexArray() = default;
		virtual ~VulkanVertexArray() = default;

		virtual void Bind() const override {}
		virtual void Unbind() const override {}

		virtual void AddVertexBuffer(const Ref<VertexBuffer>& vb) override
		{
			m_VertexBuffers.push_back(vb);
		}

		virtual void SetIndexBuffer(const Ref<IndexBuffer>& ib) override
		{
			m_IndexBuffer = ib;
		}

		virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const override { return m_VertexBuffers; }
		virtual const Ref<IndexBuffer>& GetIndexBuffer() const override { return m_IndexBuffer; }

	private:
		std::vector<Ref<VertexBuffer>> m_VertexBuffers;
		Ref<IndexBuffer>               m_IndexBuffer;
	};

}
