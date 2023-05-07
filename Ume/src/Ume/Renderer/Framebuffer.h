#pragma once

#include "Ume/Core/Core.h"

namespace Ume
{
	struct FramebufferSpecs
	{
		uint32_t width, height;
		uint32_t samples = 1;

		bool swapChainTarget = false;
	};

	class Framebuffer
	{
	public:
		virtual ~Framebuffer() = default;
		virtual const FramebufferSpecs& GetSpecs() const = 0;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;
		virtual uint32_t GetColorAttachment() const = 0;

		static Ref<Framebuffer> Create(const FramebufferSpecs& specs);
	};
}