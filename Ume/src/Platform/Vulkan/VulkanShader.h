#pragma once

#include "Ume/Renderer/Shader.h"
#include "Ume/Renderer/Buffer.h"
#include "Platform/Vulkan/VulkanContext.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <string>

namespace Ume {

	// Uniform data is pushed via push constants for small data or descriptor sets.
	// We track uniform values in a CPU-side map and write them to a push-constant
	// buffer or a uniform buffer per draw call.
	class VulkanShader : public Shader
	{
	public:
		VulkanShader(const std::string& filepath);
		VulkanShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
		virtual ~VulkanShader();

		virtual void Bind() const override;
		virtual void Unbind() const override {}

		virtual void SetInt(const std::string& name, int value) override;
		virtual void SetIntArray(const std::string& name, int* values, uint32_t count) override;
		virtual void SetFloat(const std::string& name, float value) override;
		virtual void SetFloat2(const std::string& name, const glm::vec2& value) override;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) override;
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) override;
		virtual void SetMat4(const std::string& name, const glm::mat4& value) override;

		virtual const std::string& GetName() const override { return m_Name; }

		VkPipeline       GetPipeline()       const { return m_Pipeline; }
		VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
		VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

		// Returns the pipeline for a specific blend/topology configuration.
		// Currently returns the single default pipeline.
		VkPipeline GetOrCreatePipeline(bool enableBlend, VkPrimitiveTopology topology);

		// Descriptor set management
		VkDescriptorSet AllocateDescriptorSet();

	private:
		std::string ReadFile(const std::string& filepath);
		std::unordered_map<uint32_t, std::string> PreProcess(const std::string& source);
		void CompileToSpirv(const std::unordered_map<uint32_t, std::string>& sources);
		void CreateDescriptorSetLayout();
		void CreatePipelineLayout();
		void ReflectShader();
		void CreateGraphicsPipeline(VkRenderPass renderPass, bool enableBlend, VkPrimitiveTopology topology);
		VkShaderModule CreateShaderModule(const std::vector<uint32_t>& code);

		// Helper — convert ShaderDataType to Vulkan format
		static VkFormat ShaderDataTypeToVkFormat(ShaderDataType type);

	private:
		std::string m_FilePath;
		std::string m_Name;

		std::unordered_map<uint32_t, std::vector<uint32_t>> m_SpirV;

		// Reflected vertex input layout
		std::vector<VkVertexInputAttributeDescription> m_VertexAttributes;
		VkVertexInputBindingDescription                m_VertexBindingDesc{};
		bool                                           m_HasVertexInput  = false;
		// Reflected fragment output count (determines compatible render pass)
		uint32_t                                       m_NumColorOutputs = 1;
		VkRenderPass                                   m_PipelineRenderPass = VK_NULL_HANDLE;

		VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
		VkPipelineLayout      m_PipelineLayout      = VK_NULL_HANDLE;
		VkPipeline            m_Pipeline            = VK_NULL_HANDLE;

		// Cache for variant pipelines (blend + topology)
		struct PipelineKey { bool blend; VkPrimitiveTopology topology; };
		struct PipelineKeyHash
		{
			size_t operator()(const PipelineKey& k) const
			{
				return std::hash<uint32_t>()((uint32_t)k.topology << 1 | (uint32_t)k.blend);
			}
		};
		struct PipelineKeyEq
		{
			bool operator()(const PipelineKey& a, const PipelineKey& b) const
			{
				return a.blend == b.blend && a.topology == b.topology;
			}
		};
		std::unordered_map<PipelineKey, VkPipeline, PipelineKeyHash, PipelineKeyEq> m_PipelineCache;
	};

}
