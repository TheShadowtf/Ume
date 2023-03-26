#pragma once

#include "Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace Ume
{
	class UME_API Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;


	};
}

#define UME_CORE_ERROR(...)  ::Ume::Log::GetCoreLogger()->error(__VA_ARGS__)
#define UME_CORE_WARN(...)   ::Ume::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define UME_CORE_INFO(...)   ::Ume::Log::GetCoreLogger()->info(__VA_ARGS__)
#define UME_CORE_TRACE(...)  ::Ume::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define UME_CORE_FATAL(...)  ::Ume::Log::GetCoreLogger()->fatal(__VA_ARGS__)

#define UME_ERROR(...)       ::Ume::Log::GetClientLogger()->error(__VA_ARGS__)
#define UME_WARN(...)        ::Ume::Log::GetClientLogger()->warn(__VA_ARGS__)
#define UME_INFO(...)        ::Ume::Log::GetClientLogger()->info(__VA_ARGS__)
#define UME_TRACE(...)       ::Ume::Log::GetClientLogger()->trace(__VA_ARGS__)
#define UME_FATAL(...)       ::Ume::Log::GetClientLogger()->fatal(__VA_ARGS__)