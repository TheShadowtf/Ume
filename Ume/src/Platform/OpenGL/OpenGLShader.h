#pragma once

#include "Ume/Renderer/Shader.h"
#include <glm/glm.hpp>

typedef unsigned int GLenum;

namespace Ume
{
	class OpenGlShader : public Shader
	{
	public:
		OpenGlShader(const std::string& filepath);
		OpenGlShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
		virtual ~OpenGlShader();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void SetInt(const std::string name, int value) override;
		virtual void SetMat4(const std::string name, const glm::mat4 value) override;
		virtual void SetFloat4(const std::string name, const glm::vec4 value) override;
		virtual void SetFloat3(const std::string name, const glm::vec3 value) override;
		virtual void SetFloat2(const std::string name, const glm::vec2 value) override;
		virtual void SetFloat(const std::string name, float value) override;
		virtual void SetIntArray(const std::string name, int* values, uint32_t count) override;

		virtual const std::string& GetName() const override { return m_Name; }

		void UploadUniformInt(const std::string& name, int values);
		void UploadUniformFloat(const std::string& name, float values);
		void UploadUniformFloat2(const std::string& name, const glm::vec2& values);
		void UploadUniformFloat3(const std::string& name, const glm::vec3& values);
		void UploadUniformFloat4(const std::string& name, const glm::vec4& values);
		void UploadUniformMat4(const std::string& name, const glm::mat4& matrix);
		void UploadUniformMat3(const std::string& name, const glm::mat3& matrix);


		void UploadUniformIntArray(const std::string& name, int* values, uint32_t count);
	private:
		std::string ReadFile(const std::string& filepath);
		std::unordered_map<GLenum, std::string> PreProcess(const std::string& src);
		void Compile(const std::unordered_map<GLenum, std::string>& shaderSrc);
	private:
		uint32_t m_RendererID = 0;
		std::string m_Name;
	};
}