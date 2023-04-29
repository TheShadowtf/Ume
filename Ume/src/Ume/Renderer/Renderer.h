#pragma once

#include "RendererCommand.h"
#include "OrthographicCamera.h"
#include "Shader.h"

namespace Ume
{
	class Renderer
	{
	public:
		static void Init();
		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginScene(OrthographicCamera& camera);
		static void EndScene();

		static void Submit(const Ref<VertexArray>& vertexArray, const Ref<Shader>& shader, const glm::mat4& transform = glm::mat4(1.0f));

		inline static RendererAPI::API GetRendererAPI() { return RendererAPI::GetApi();  } 
	private:
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};

		static SceneData* m_SceneData;
	};

}