#pragma once

#include "Ume/Renderer/VertexArray.h"

namespace Ume
{
	class OpenGLVertexArray : public VertexArray
	{
	public:
		OpenGLVertexArray();
		virtual ~OpenGLVertexArray();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override;
		virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) override;

		virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const { return m_VertexBuffer; }
		virtual const Ref<IndexBuffer>& GetIndexBuffer() const { return m_IndexBuffer; }
	private:
		std::vector<Ref<VertexBuffer>> m_VertexBuffer;
		Ref<IndexBuffer>			   m_IndexBuffer;
		uint32_t m_RendererID;
	};
}