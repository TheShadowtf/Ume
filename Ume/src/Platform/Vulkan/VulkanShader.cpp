#include "umepch.h"
#include "Platform/Vulkan/VulkanShader.h"
#include "Platform/Vulkan/VulkanUtils.h"

#include "Ume/Renderer/Buffer.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <filesystem>
#include <fstream>

// Shader stage constants — local to this TU to avoid collision with OpenGL headers
namespace {
	constexpr uint32_t SHADER_STAGE_VERTEX   = 0x8B31u;
	constexpr uint32_t SHADER_STAGE_FRAGMENT = 0x8B30u;
}

namespace Ume {

	namespace Utils {

		static uint32_t ShaderTypeFromString(const std::string& type)
		{
			if (type == "vertex")            return SHADER_STAGE_VERTEX;
			if (type == "fragment" || type == "pixel") return SHADER_STAGE_FRAGMENT;
			UME_CORE_ASSERT(false, "Unknown shader type!");
			return 0;
		}

		static shaderc_shader_kind ShaderStageToShadercKind(uint32_t stage)
		{
			switch (stage)
			{
				case SHADER_STAGE_VERTEX:   return shaderc_glsl_vertex_shader;
				case SHADER_STAGE_FRAGMENT: return shaderc_glsl_fragment_shader;
			}
			UME_CORE_ASSERT(false, "Unknown shader stage!");
			return (shaderc_shader_kind)0;
		}

		static VkShaderStageFlagBits ShaderStageToVkStage(uint32_t stage)
		{
			switch (stage)
			{
				case SHADER_STAGE_VERTEX:   return VK_SHADER_STAGE_VERTEX_BIT;
				case SHADER_STAGE_FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
			}
			UME_CORE_ASSERT(false, "Unknown shader stage!");
			return (VkShaderStageFlagBits)0;
		}

		static const char* GetCacheDir() { return "assets/cache/shader/vulkan"; }

		static void EnsureCacheDir()
		{
			if (!std::filesystem::exists(GetCacheDir()))
				std::filesystem::create_directories(GetCacheDir());
		}

		static std::string CachePath(const std::string& shaderPath, uint32_t stage)
		{
			std::string ext = (stage == SHADER_STAGE_VERTEX) ? ".cached_vulkan.vert" : ".cached_vulkan.frag";
			return std::string(GetCacheDir()) + "/" + std::filesystem::path(shaderPath).filename().string() + ext;
		}

		// ---- SPIRV-Cross reflection helpers ----

		static VkFormat VkFormatFromSpirvType(const spirv_cross::SPIRType& type)
		{
			using BT = spirv_cross::SPIRType::BaseType;
			if (type.basetype == BT::Float)
			{
				switch (type.vecsize) {
					case 1: return VK_FORMAT_R32_SFLOAT;
					case 2: return VK_FORMAT_R32G32_SFLOAT;
					case 3: return VK_FORMAT_R32G32B32_SFLOAT;
					case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
				}
			}
			else if (type.basetype == BT::Int)
			{
				switch (type.vecsize) {
					case 1: return VK_FORMAT_R32_SINT;
					case 2: return VK_FORMAT_R32G32_SINT;
					case 3: return VK_FORMAT_R32G32B32_SINT;
					case 4: return VK_FORMAT_R32G32B32A32_SINT;
				}
			}
			else if (type.basetype == BT::UInt)
			{
				switch (type.vecsize) {
					case 1: return VK_FORMAT_R32_UINT;
					case 2: return VK_FORMAT_R32G32_UINT;
					case 3: return VK_FORMAT_R32G32B32_UINT;
					case 4: return VK_FORMAT_R32G32B32A32_UINT;
				}
			}
			return VK_FORMAT_UNDEFINED;
		}

		static uint32_t VkFormatElementSize(VkFormat fmt)
		{
			switch (fmt)
			{
				case VK_FORMAT_R32_SFLOAT:          case VK_FORMAT_R32_SINT:          case VK_FORMAT_R32_UINT:          return 4;
				case VK_FORMAT_R32G32_SFLOAT:       case VK_FORMAT_R32G32_SINT:       case VK_FORMAT_R32G32_UINT:       return 8;
				case VK_FORMAT_R32G32B32_SFLOAT:    case VK_FORMAT_R32G32B32_SINT:    case VK_FORMAT_R32G32B32_UINT:    return 12;
				case VK_FORMAT_R32G32B32A32_SFLOAT: case VK_FORMAT_R32G32B32A32_SINT: case VK_FORMAT_R32G32B32A32_UINT: return 16;
				default: return 4;
			}
		}

	}

	// -----------------------------------------------------------------------

	VulkanShader::VulkanShader(const std::string& filepath)
		: m_FilePath(filepath)
	{
		Utils::EnsureCacheDir();
		auto source  = ReadFile(filepath);
		auto sources = PreProcess(source);
		CompileToSpirv(sources);
		ReflectShader();
		CreateDescriptorSetLayout();
		CreatePipelineLayout();
		auto* ctx = VulkanContext::Get();
		m_PipelineRenderPass = (m_NumColorOutputs <= 1)
		    ? ctx->GetRenderPass()
		    : ctx->GetOrCreatePipelineRenderPass(m_NumColorOutputs);
		CreateGraphicsPipeline(m_PipelineRenderPass, true, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		auto lastSlash = filepath.find_last_of("/\\");
		lastSlash = (lastSlash == std::string::npos) ? 0 : lastSlash + 1;
		auto lastDot = filepath.rfind('.');
		auto count   = (lastDot == std::string::npos) ? filepath.size() - lastSlash : lastDot - lastSlash;
		m_Name = filepath.substr(lastSlash, count);
	}

	VulkanShader::VulkanShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
		: m_Name(name)
	{
		Utils::EnsureCacheDir();
		std::unordered_map<uint32_t, std::string> sources;
		sources[SHADER_STAGE_VERTEX]   = vertexSrc;
		sources[SHADER_STAGE_FRAGMENT] = fragmentSrc;
		CompileToSpirv(sources);
		ReflectShader();
		CreateDescriptorSetLayout();
		CreatePipelineLayout();
		auto* ctx = VulkanContext::Get();
		m_PipelineRenderPass = (m_NumColorOutputs <= 1)
		    ? ctx->GetRenderPass()
		    : ctx->GetOrCreatePipelineRenderPass(m_NumColorOutputs);
		CreateGraphicsPipeline(m_PipelineRenderPass, true, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	}

	VulkanShader::~VulkanShader()
	{
		auto* ctx    = VulkanContext::Get();
		VkDevice dev = ctx->GetDevice();
		vkDeviceWaitIdle(dev);

		for (auto& [key, pipeline] : m_PipelineCache)
			vkDestroyPipeline(dev, pipeline, nullptr);

		if (m_Pipeline != VK_NULL_HANDLE)
		{
			// Already in the cache if it was inserted; avoid double-destroy
			bool found = false;
			for (auto& [key, p] : m_PipelineCache)
				if (p == m_Pipeline) { found = true; break; }
			if (!found)
				vkDestroyPipeline(dev, m_Pipeline, nullptr);
		}

		vkDestroyPipelineLayout(dev, m_PipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(dev, m_DescriptorSetLayout, nullptr);
	}

	// -----------------------------------------------------------------------
	// Source preprocessing
	// -----------------------------------------------------------------------

	void VulkanShader::ReflectShader()
	{
		// -- Vertex inputs: build attribute descriptions and compute stride --
		auto vertIt = m_SpirV.find(SHADER_STAGE_VERTEX);
		if (vertIt != m_SpirV.end() && !vertIt->second.empty())
		{
			spirv_cross::Compiler comp(vertIt->second);
			auto resources = comp.get_shader_resources();

			struct InputVar { uint32_t location; VkFormat format; };
			std::vector<InputVar> inputs;

			for (auto& inp : resources.stage_inputs)
			{
				uint32_t loc   = comp.get_decoration(inp.id, spv::DecorationLocation);
				auto&    type  = comp.get_type(inp.base_type_id);
				inputs.push_back({ loc, Utils::VkFormatFromSpirvType(type) });
			}

			std::sort(inputs.begin(), inputs.end(), [](const InputVar& a, const InputVar& b)
			    { return a.location < b.location; });

			uint32_t offset = 0;
			for (auto& in : inputs)
			{
				VkVertexInputAttributeDescription attr{};
				attr.binding  = 0;
				attr.location = in.location;
				attr.format   = in.format;
				attr.offset   = offset;
				m_VertexAttributes.push_back(attr);
				offset += Utils::VkFormatElementSize(in.format);
			}
			if (offset > 0)
			{
				m_VertexBindingDesc.binding   = 0;
				m_VertexBindingDesc.stride    = offset;
				m_VertexBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
				m_HasVertexInput = true;
			}
		}

		// -- Fragment color outputs: count to determine compatible render pass --
		auto fragIt = m_SpirV.find(SHADER_STAGE_FRAGMENT);
		if (fragIt != m_SpirV.end() && !fragIt->second.empty())
		{
			spirv_cross::Compiler comp(fragIt->second);
			auto resources = comp.get_shader_resources();

			uint32_t maxLoc = 0;
			for (auto& out : resources.stage_outputs)
			{
				uint32_t loc = comp.get_decoration(out.id, spv::DecorationLocation);
				maxLoc = std::max(maxLoc, loc + 1);
			}
			m_NumColorOutputs = std::max(maxLoc, 1u);
		}
	}

	std::string VulkanShader::ReadFile(const std::string& filepath)
	{
		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			size_t size = in.tellg();
			if (size != (size_t)-1)
			{
				result.resize(size);
				in.seekg(0, std::ios::beg);
				in.read(&result[0], size);
			}
		}
		else
		{
			UME_CORE_ERROR("VulkanShader: could not open '{0}'", filepath);
		}
		return result;
	}

	std::unordered_map<uint32_t, std::string> VulkanShader::PreProcess(const std::string& source)
	{
		std::unordered_map<uint32_t, std::string> shaderSources;
		const char* typeToken     = "#type";
		size_t typeTokenLength    = strlen(typeToken);
		size_t pos                = source.find(typeToken, 0);

		while (pos != std::string::npos)
		{
			size_t eol   = source.find_first_of("\r\n", pos);
			size_t begin = pos + typeTokenLength + 1;
			std::string type = source.substr(begin, eol - begin);

			size_t nextLinePos = source.find_first_not_of("\r\n", eol);
			pos = source.find(typeToken, nextLinePos);

			shaderSources[Utils::ShaderTypeFromString(type)] =
			    (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
		}
		return shaderSources;
	}

	// -----------------------------------------------------------------------
	// SPIR-V compilation via shaderc
	// -----------------------------------------------------------------------

	void VulkanShader::CompileToSpirv(const std::unordered_map<uint32_t, std::string>& sources)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		options.SetOptimizationLevel(shaderc_optimization_level_performance);

		for (auto& [stage, source] : sources)
		{
			std::string cachePath = Utils::CachePath(m_FilePath, stage);

			// Try to load from cache (keyed by source hash embedded in filename)
			// For simplicity, always recompile when file changes by checking mod time.
			// Here we recompile every load for correctness.
			shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(
			    source, Utils::ShaderStageToShadercKind(stage), m_FilePath.c_str(), options);

			if (result.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				UME_CORE_ERROR("VulkanShader: SPIR-V compilation failed for '{0}':\n{1}", m_FilePath, result.GetErrorMessage());
				UME_CORE_ASSERT(false, "Shader compilation failed!");
				continue;
			}

			m_SpirV[stage] = std::vector<uint32_t>(result.cbegin(), result.cend());

			// Write cache
			std::ofstream out(cachePath, std::ios::binary | std::ios::trunc);
			if (out.is_open())
				out.write((const char*)m_SpirV[stage].data(), m_SpirV[stage].size() * sizeof(uint32_t));
		}
	}

	// -----------------------------------------------------------------------
	// Descriptor set layout
	//   binding 0: Camera UBO          (vertex + fragment stages)
	//   binding 1: texture sampler[32] (fragment stage)
	// -----------------------------------------------------------------------

	void VulkanShader::CreateDescriptorSetLayout()
	{
		auto* ctx    = VulkanContext::Get();
		VkDevice dev = ctx->GetDevice();

		// Binding 0: Camera uniform buffer — visible to both graphics stages
		VkDescriptorSetLayoutBinding cameraBinding{};
		cameraBinding.binding         = 0;
		cameraBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		cameraBinding.descriptorCount = 1;
		cameraBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		// Binding 1: Combined image samplers — fragment stage
		VkDescriptorSetLayoutBinding samplerBinding{};
		samplerBinding.binding            = 1;
		samplerBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerBinding.descriptorCount    = 32;
		samplerBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerBinding.pImmutableSamplers = nullptr;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { cameraBinding, samplerBinding };

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = (uint32_t)bindings.size();
		layoutInfo.pBindings    = bindings.data();

		VK_CHECK(vkCreateDescriptorSetLayout(dev, &layoutInfo, nullptr, &m_DescriptorSetLayout));
	}

	void VulkanShader::CreatePipelineLayout()
	{
		auto* ctx    = VulkanContext::Get();
		VkDevice dev = ctx->GetDevice();

		// Push constant range for small per-draw data
		VkPushConstantRange pushConstant{};
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstant.offset     = 0;
		pushConstant.size       = 128; // 128 bytes max (spec guaranteed minimum)

		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount         = 1;
		layoutInfo.pSetLayouts            = &m_DescriptorSetLayout;
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges    = &pushConstant;

		VK_CHECK(vkCreatePipelineLayout(dev, &layoutInfo, nullptr, &m_PipelineLayout));
	}

	// -----------------------------------------------------------------------
	// Graphics pipeline
	// -----------------------------------------------------------------------

	VkShaderModule VulkanShader::CreateShaderModule(const std::vector<uint32_t>& code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size() * sizeof(uint32_t);
		createInfo.pCode    = code.data();

		VkShaderModule module;
		VK_CHECK(vkCreateShaderModule(VulkanContext::Get()->GetDevice(), &createInfo, nullptr, &module));
		return module;
	}

	VkFormat VulkanShader::ShaderDataTypeToVkFormat(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:  return VK_FORMAT_R32_SFLOAT;
			case ShaderDataType::Float2: return VK_FORMAT_R32G32_SFLOAT;
			case ShaderDataType::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
			case ShaderDataType::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
			case ShaderDataType::Int:    return VK_FORMAT_R32_SINT;
			case ShaderDataType::Int2:   return VK_FORMAT_R32G32_SINT;
			case ShaderDataType::Int3:   return VK_FORMAT_R32G32B32_SINT;
			case ShaderDataType::Int4:   return VK_FORMAT_R32G32B32A32_SINT;
			default: break;
		}
		UME_CORE_ASSERT(false, "Unsupported ShaderDataType for Vulkan vertex input!");
		return VK_FORMAT_UNDEFINED;
	}

	void VulkanShader::CreateGraphicsPipeline(VkRenderPass renderPass, bool enableBlend, VkPrimitiveTopology topology)
	{
		auto* ctx    = VulkanContext::Get();
		VkDevice dev = ctx->GetDevice();

		// Shader stages
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		std::vector<VkShaderModule> modules;

		for (auto& [stage, spirv] : m_SpirV)
		{
			if (spirv.empty()) continue;
			VkShaderModule mod = CreateShaderModule(spirv);
			modules.push_back(mod);

			VkPipelineShaderStageCreateInfo stageInfo{};
			stageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageInfo.stage  = Utils::ShaderStageToVkStage(stage);
			stageInfo.module = mod;
			stageInfo.pName  = "main";
			shaderStages.push_back(stageInfo);
		}

		// Dynamic state
		std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
		dynamicState.pDynamicStates    = dynamicStates.data();

		// Vertex input — populated from SPIRV-Cross reflection in ReflectShader()
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		if (m_HasVertexInput)
		{
			vertexInputInfo.vertexBindingDescriptionCount   = 1;
			vertexInputInfo.pVertexBindingDescriptions      = &m_VertexBindingDesc;
			vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)m_VertexAttributes.size();
			vertexInputInfo.pVertexAttributeDescriptions    = m_VertexAttributes.data();
		}

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology               = topology;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount  = 1;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable        = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth               = 1.0f;
		rasterizer.cullMode                = VK_CULL_MODE_NONE;
		rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable  = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;

		// Build one blend attachment entry per fragment color output
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		if (enableBlend)
		{
			colorBlendAttachment.blendEnable         = VK_TRUE;
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
		}
		// Integer attachment (entity ID) must have blending disabled
		VkPipelineColorBlendAttachmentState intAttachment{};
		intAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachments(m_NumColorOutputs, colorBlendAttachment);
		if (m_NumColorOutputs >= 2)
			blendAttachments[1] = intAttachment; // entity ID — no blending

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable   = VK_FALSE;
		colorBlending.attachmentCount = (uint32_t)blendAttachments.size();
		colorBlending.pAttachments    = blendAttachments.data();

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount          = (uint32_t)shaderStages.size();
		pipelineInfo.pStages             = shaderStages.data();
		pipelineInfo.pVertexInputState   = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState      = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState   = &multisampling;
		pipelineInfo.pDepthStencilState  = &depthStencil;
		pipelineInfo.pColorBlendState    = &colorBlending;
		pipelineInfo.pDynamicState       = &dynamicState;
		pipelineInfo.layout              = m_PipelineLayout;
		pipelineInfo.renderPass          = renderPass;
		pipelineInfo.subpass             = 0;

		VK_CHECK(vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline));

		// Store in cache
		PipelineKey key{ enableBlend, topology };
		m_PipelineCache[key] = m_Pipeline;

		// Destroy shader modules (no longer needed after pipeline creation)
		for (auto mod : modules)
			vkDestroyShaderModule(dev, mod, nullptr);
	}

	VkPipeline VulkanShader::GetOrCreatePipeline(bool enableBlend, VkPrimitiveTopology topology)
	{
		PipelineKey key{ enableBlend, topology };
		auto it = m_PipelineCache.find(key);
		if (it != m_PipelineCache.end())
			return it->second;

		// Create new variant
		CreateGraphicsPipeline(m_PipelineRenderPass, enableBlend, topology);
		return m_PipelineCache[key];
	}

	VkDescriptorSet VulkanShader::AllocateDescriptorSet()
	{
		auto* ctx    = VulkanContext::Get();
		VkDevice dev = ctx->GetDevice();

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool     = ctx->GetCurrentFrameDescriptorPool();
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts        = &m_DescriptorSetLayout;

		VkDescriptorSet set;
		VK_CHECK(vkAllocateDescriptorSets(dev, &allocInfo, &set));
		return set;
	}

	// -----------------------------------------------------------------------
	// Uniform setters — stored in a push-constant-style CPU buffer.
	// The actual Bind/push happens in VulkanRendererAPI::DrawIndexed.
	// For now these are no-ops since Renderer2D uploads data via UBOs.
	// -----------------------------------------------------------------------

	void VulkanShader::Bind() const
	{
		VkCommandBuffer cmd = VulkanContext::Get()->GetCurrentCommandBuffer();
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	}

	void VulkanShader::SetInt(const std::string&, int) {}
	void VulkanShader::SetIntArray(const std::string&, int*, uint32_t) {}
	void VulkanShader::SetFloat(const std::string&, float) {}
	void VulkanShader::SetFloat2(const std::string&, const glm::vec2&) {}
	void VulkanShader::SetFloat3(const std::string&, const glm::vec3&) {}
	void VulkanShader::SetFloat4(const std::string&, const glm::vec4&) {}
	void VulkanShader::SetMat4(const std::string&, const glm::mat4&) {}

} // namespace Ume
