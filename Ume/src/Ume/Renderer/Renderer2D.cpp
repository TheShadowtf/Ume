#include <umepch.h>
#include "Renderer2D.h"

#include "VertexArray.h"
#include "Shader.h"
#include "RendererCommand.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Ume
{
	struct QuadVertex
	{
		glm::vec2 TexCoord;
		glm::vec3 Pos;
		glm::vec4 Color;
		float TexIndex;
		float TexScale;
	};

	struct Renderer2Data
	{
		static const uint32_t maxQuads	       = 200000;
		static const uint32_t maxVertices      = maxQuads * 4;
		static const uint32_t maxIndices	   = maxQuads * 6;
		static const uint32_t maxTextureSlots  = 32;

		uint32_t quadIndexCount = 0;

		Ref<VertexArray> vertexArray;
		Ref<VertexBuffer> vertexBuffer;
		Ref<Shader> textureShader;
		Ref<Texture2D> whiteTexture;

		QuadVertex* quadVertexBufferBase    = nullptr;
		QuadVertex* quadVertexBufferPointer = nullptr;

		std::array<Ref<Texture2D>, maxTextureSlots> textureSlots;
		uint32_t textureSlotIndex = 1;

		glm::vec4 quadVertexPos[4];

		Renderer2D::Statistics stats;
	};

	static Renderer2Data s_Data;

	void Renderer2D::Init()
	{
		UME_PROFILE_FUNC();

		s_Data.vertexArray = VertexArray::Create();
		s_Data.vertexBuffer = VertexBuffer::Create(s_Data.maxVertices * sizeof(QuadVertex));

		s_Data.vertexBuffer->SetLayout({
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Float, "a_TexIndex" },
			{ ShaderDataType::Float, "a_TexScale" }
			});
		s_Data.vertexArray->AddVertexBuffer(s_Data.vertexBuffer);

		s_Data.quadVertexBufferBase = new QuadVertex[s_Data.maxVertices];

		uint32_t* quadIndices = new uint32_t[s_Data.maxIndices];

		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_Data.maxIndices; i += 6)
		{
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;
		}

		Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_Data.maxIndices);
		s_Data.vertexArray->SetIndexBuffer(quadIB);
		delete[] quadIndices;

		s_Data.whiteTexture = Texture2D::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.whiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		int32_t samplers[s_Data.maxTextureSlots];
		for (uint32_t i = 0; i < s_Data.maxTextureSlots; i++)
			samplers[i] = i;

		s_Data.textureShader = Shader::Create("assets/shaders/Texture.glsl");
		s_Data.textureShader->Bind();
		s_Data.textureShader->SetIntArray("u_Textures", samplers, s_Data.maxTextureSlots);

		s_Data.textureSlots[0] = s_Data.whiteTexture;

		s_Data.quadVertexPos[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.quadVertexPos[1] = {  0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.quadVertexPos[2] = {  0.5f,  0.5f, 0.0f, 1.0f };
		s_Data.quadVertexPos[3] = { -0.5f,  0.5f, 0.0f, 1.0f };
	}

	void Renderer2D::Shutdown()
	{
		UME_PROFILE_FUNC();
	}

	void Renderer2D::BeginScene(const OrthographicCamera& camera)
	{
		UME_PROFILE_FUNC();

		s_Data.textureShader->Bind();
		s_Data.textureShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());

		s_Data.quadIndexCount = 0;
		s_Data.quadVertexBufferPointer = s_Data.quadVertexBufferBase;

		s_Data.textureSlotIndex = 1;
	}

	void Renderer2D::BeginScene(const Camera& camera, const glm::mat4 transform)
	{
		UME_PROFILE_FUNC();

		glm::mat4 viewProj = camera.GetProjection() * glm::inverse(transform);

		s_Data.textureShader->Bind();
		s_Data.textureShader->SetMat4("u_ViewProjection", viewProj);

		s_Data.quadIndexCount = 0;
		s_Data.quadVertexBufferPointer = s_Data.quadVertexBufferBase;

		s_Data.textureSlotIndex = 1;
	}

	void Renderer2D::EndScene()
	{
		UME_PROFILE_FUNC();

		Flush();
	}

	void Renderer2D::Flush()
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.quadVertexBufferPointer - (uint8_t*)s_Data.quadVertexBufferBase);
		s_Data.vertexBuffer->SetData(s_Data.quadVertexBufferBase, dataSize);

		for (uint32_t i = 0; i < s_Data.textureSlotIndex; i++)
			s_Data.textureSlots[i]->Bind(i);

		RendererCommand::DrawIndexed(s_Data.vertexArray, s_Data.quadIndexCount);
		s_Data.stats.drawCalls++;
	}

	void Renderer2D::StartNewBatch()
	{
		EndScene();
		
		s_Data.quadIndexCount = 0;
		s_Data.quadVertexBufferPointer = s_Data.quadVertexBufferBase;

		s_Data.textureSlotIndex = 1;
	}

	// Quads
	// Textures
	void Renderer2D::DrawQuad(const glm::vec2& pos, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor /*= 1.0f*/)
	{
		DrawQuad({ pos.x, pos.y, 0.0f }, size, texture, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec1(0.0f), tilingFactor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& pos, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor /*= 1.0f*/)
	{
		DrawQuad(pos, size, texture, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec1(0.0f), tilingFactor);
	}

	void Renderer2D::DrawQuad(const glm::vec2& pos, const glm::vec2& size, const Ref<Texture2D>& texture, const glm::vec4& color, float tilingFactor /*= 1.0f*/)
	{
		DrawQuad({ pos.x, pos.y, 0.0f }, size, texture, color, glm::vec1(0.0f), tilingFactor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& pos, const glm::vec2& size, const Ref<Texture2D>& texture, const glm::vec4& color, float tilingFactor /*= 1.0f*/)
	{
		DrawQuad(pos, size, texture, color, glm::vec1(0.0f), tilingFactor);
	}

	void Renderer2D::DrawQuad(const glm::vec2& pos, const glm::vec2& size, const Ref<Texture2D>& texture, glm::vec1 rotation, float tilingFactor /*= 1.0f*/)
	{
		DrawQuad({ pos.x, pos.y, 0.0f }, size, texture, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), rotation, tilingFactor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& pos, const glm::vec2& size, const Ref<Texture2D>& texture, glm::vec1 rotation, float tilingFactor /*= 1.0f*/)
	{
		DrawQuad(pos, size, texture, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), rotation, tilingFactor);
	}

	void Renderer2D::DrawQuad(const glm::vec2& pos, const glm::vec2& size, const Ref<Texture2D>& texture, const glm::vec4& color, glm::vec1 rotation, float tilingFactor /*= 1.0f*/)
	{
		DrawQuad({ pos.x, pos.y, 0.0f }, size, texture, color, rotation, tilingFactor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& pos, const glm::vec2& size, const Ref<Texture2D>& texture, const glm::vec4& color, glm::vec1 rotation, float tilingFactor)
	{
		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 texCoord[] =
		{
			{ 0.0f, 0.0f },
			{ 1.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 0.0f, 1.0f }
		};

		if (s_Data.quadIndexCount >= Renderer2Data::maxIndices)
			StartNewBatch();

		float texIndex = 0.0f;

		for (uint32_t i = 1; i < s_Data.textureSlotIndex; i++)
		{
			if (*s_Data.textureSlots[i].get() == *texture.get())
			{
				texIndex = (float)i;
				break;
			}
		}
		if (texIndex == 0.0f)
		{
			texIndex = (float)s_Data.textureSlotIndex;
			s_Data.textureSlots[s_Data.textureSlotIndex] = texture;
			s_Data.textureSlotIndex++;
		}

		glm::mat4 transform;

		if (rotation.x == 0.0f)
			transform = glm::translate(glm::mat4(1.0f), pos) *
				glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
		else if (rotation.x > 0.0f)
			transform = glm::translate(glm::mat4(1.0f), pos) *
				glm::rotate(glm::mat4(1.0f), rotation.x, { 0.0f, 0.0f, 1.0f }) *
				glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.quadVertexBufferPointer->Pos = transform * s_Data.quadVertexPos[i];
			s_Data.quadVertexBufferPointer->Color = color;
			s_Data.quadVertexBufferPointer->TexCoord = texCoord[i];
			s_Data.quadVertexBufferPointer->TexIndex = texIndex;
			s_Data.quadVertexBufferPointer->TexScale = tilingFactor;
			s_Data.quadVertexBufferPointer++;
		}

		s_Data.quadIndexCount += 6;
		s_Data.stats.quadCount++;
	}

	// Sub Texture
	void Renderer2D::DrawQuad(const glm::vec2& pos, const glm::vec2& size, const Ref<SubTexture2D>& subTexture, float tilingFactor /*= 1.0f*/)
	{
		DrawQuad({ pos.x, pos.y, 0.0f }, size, subTexture, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec1(0.0f), tilingFactor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& pos, const glm::vec2& size, const Ref<SubTexture2D>& subTexture, float tilingFactor /*= 1.0f*/)
	{
		DrawQuad(pos, size, subTexture, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec1(0.0f), tilingFactor);
	}

	void Renderer2D::DrawQuad(const glm::vec2& pos, const glm::vec2& size, const Ref<SubTexture2D>& subTexture, const glm::vec4& color, float tilingFactor /*= 1.0f*/)
	{
		DrawQuad({ pos.x, pos.y, 0.0f }, size, subTexture, color, glm::vec1(0.0f), tilingFactor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& pos, const glm::vec2& size, const Ref<SubTexture2D>& subTexture, const glm::vec4& color, float tilingFactor /*= 1.0f*/)
	{
		DrawQuad(pos, size, subTexture, color, glm::vec1(0.0f), tilingFactor);
	}

	void Renderer2D::DrawQuad(const glm::vec2& pos, const glm::vec2& size, const Ref<SubTexture2D>& subTexture, glm::vec1 rotation, float tilingFactor /*= 1.0f*/)
	{
		DrawQuad({ pos.x, pos.y, 0.0f }, size, subTexture, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), rotation, tilingFactor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& pos, const glm::vec2& size, const Ref<SubTexture2D>& subTexture, glm::vec1 rotation, float tilingFactor /*= 1.0f*/)
	{
		DrawQuad(pos, size, subTexture, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), rotation, tilingFactor);
	}

	void Renderer2D::DrawQuad(const glm::vec2& pos, const glm::vec2& size, const Ref<SubTexture2D>& subTexture, const glm::vec4& color, glm::vec1 rotation, float tilingFactor /*= 1.0f*/)
	{
		DrawQuad({ pos.x, pos.y, 0.0f }, size, subTexture, color, rotation, tilingFactor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& pos, const glm::vec2& size, const Ref<SubTexture2D>& subTexture, const glm::vec4& color, glm::vec1 rotation, float tilingFactor)
	{
		constexpr size_t quadVertexCount = 4;
		const glm::vec2* texCoord = subTexture->GetTexCoords();
		const Ref<Texture2D> texture = subTexture->GetTexture();

		if (s_Data.quadIndexCount >= Renderer2Data::maxIndices)
			StartNewBatch();

		float texIndex = 0.0f;

		for (uint32_t i = 1; i < s_Data.textureSlotIndex; i++)
		{
			if (*s_Data.textureSlots[i].get() == *texture.get())
			{
				texIndex = (float)i;
				break;
			}
		}
		if (texIndex == 0.0f)
		{
			texIndex = (float)s_Data.textureSlotIndex;
			s_Data.textureSlots[s_Data.textureSlotIndex] = texture;
			s_Data.textureSlotIndex++;
		}

		glm::mat4 transform;

		if (rotation.x == 0.0f)
			transform = glm::translate(glm::mat4(1.0f), pos) *
			glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
		else if (rotation.x > 0.0f)
			transform = glm::translate(glm::mat4(1.0f), pos) *
			glm::rotate(glm::mat4(1.0f), rotation.x, { 0.0f, 0.0f, 1.0f }) *
			glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.quadVertexBufferPointer->Pos = transform * s_Data.quadVertexPos[i];
			s_Data.quadVertexBufferPointer->Color = color;
			s_Data.quadVertexBufferPointer->TexCoord = texCoord[i];
			s_Data.quadVertexBufferPointer->TexIndex = texIndex;
			s_Data.quadVertexBufferPointer->TexScale = tilingFactor;
			s_Data.quadVertexBufferPointer++;
		}

		s_Data.quadIndexCount += 6;
		s_Data.stats.quadCount++;
	}

	// Colors
	void Renderer2D::DrawQuad(const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color, float tilingFactor /*= 1.0f*/)
	{
		DrawQuad({ pos.x, pos.y, 0.0f }, size, color, glm::vec1(0.0f), tilingFactor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& pos, const glm::vec2& size, const glm::vec4& color, float tilingFactor /*= 1.0f*/)
	{
		DrawQuad(pos, size, color, glm::vec1(0.0f), tilingFactor);
	}

	void Renderer2D::DrawQuad(const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color, glm::vec1 rotation, float tilingFactor /*= 1.0f*/)
	{
		DrawQuad({ pos.x, pos.y, 0.0f }, size, color, rotation, tilingFactor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& pos, const glm::vec2& size, const glm::vec4& color, glm::vec1 rotation, float tilingFactor /*= 1.0f*/)
	{
		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 texCoord[] =
		{
			{ 0.0f, 0.0f },
			{ 1.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 0.0f, 1.0f }
		};

		if (s_Data.quadIndexCount >= Renderer2Data::maxIndices)
			StartNewBatch();

		const float texIndex = 0.0f; // White texture
		glm::mat4 transform;

		if (rotation.x == 0.0f)
			transform = glm::translate(glm::mat4(1.0f), pos) *
				glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
		else if (rotation.x > 0.0f)
			transform = glm::translate(glm::mat4(1.0f), pos) *
			glm::rotate(glm::mat4(1.0f), rotation.x, { 0.0f, 0.0f, 1.0f }) *
			glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.quadVertexBufferPointer->Pos = transform * s_Data.quadVertexPos[i];
			s_Data.quadVertexBufferPointer->Color = color;
			s_Data.quadVertexBufferPointer->TexCoord = texCoord[i];
			s_Data.quadVertexBufferPointer->TexIndex = texIndex;
			s_Data.quadVertexBufferPointer->TexScale = tilingFactor;
			s_Data.quadVertexBufferPointer++;
		}

		s_Data.quadIndexCount += 6;
		s_Data.stats.quadCount++;
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color)
	{
		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 texCoord[] =
		{
			{ 0.0f, 0.0f },
			{ 1.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 0.0f, 1.0f }
		};

		if (s_Data.quadIndexCount >= Renderer2Data::maxIndices)
			StartNewBatch();

		const float texIndex = 0.0f; // White texture

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.quadVertexBufferPointer->Pos = transform * s_Data.quadVertexPos[i];
			s_Data.quadVertexBufferPointer->Color = color;
			s_Data.quadVertexBufferPointer->TexCoord = texCoord[i];
			s_Data.quadVertexBufferPointer->TexIndex = texIndex;
			s_Data.quadVertexBufferPointer->TexScale = 1.0f;
			s_Data.quadVertexBufferPointer++;
		}

		s_Data.quadIndexCount += 6;
		s_Data.stats.quadCount++;
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture)
	{
		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 texCoord[] =
		{
			{ 0.0f, 0.0f },
			{ 1.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 0.0f, 1.0f }
		};

		if (s_Data.quadIndexCount >= Renderer2Data::maxIndices)
			StartNewBatch();

		float texIndex = 0.0f;

		for (uint32_t i = 1; i < s_Data.textureSlotIndex; i++)
		{
			if (*s_Data.textureSlots[i].get() == *texture.get())
			{
				texIndex = (float)i;
				break;
			}
		}
		if (texIndex == 0.0f)
		{
			texIndex = (float)s_Data.textureSlotIndex;
			s_Data.textureSlots[s_Data.textureSlotIndex] = texture;
			s_Data.textureSlotIndex++;
		}

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.quadVertexBufferPointer->Pos = transform * s_Data.quadVertexPos[i];
			s_Data.quadVertexBufferPointer->Color = { 1.0f, 1.0f, 1.0, 1.0f };
			s_Data.quadVertexBufferPointer->TexCoord = texCoord[i];
			s_Data.quadVertexBufferPointer->TexIndex = texIndex;
			s_Data.quadVertexBufferPointer->TexScale = 1.0f;
			s_Data.quadVertexBufferPointer++;
		}

		s_Data.quadIndexCount += 6;
		s_Data.stats.quadCount++;
	}

	//

	void Renderer2D::ResetStats()
	{
		memset(&s_Data.stats, 0, sizeof(Statistics));
	}

	Renderer2D::Statistics Renderer2D::GetStats()
	{
		return s_Data.stats;
	}
}