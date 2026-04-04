#pragma once

#include "Ume/Core/Base.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

namespace Ume {

	class Log
	{
	public:
		static void Init();

		static Ref<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static Ref<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static Ref<spdlog::logger> s_CoreLogger;
		static Ref<spdlog::logger> s_ClientLogger;
	};

}

template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector)
{
	return os << glm::to_string(vector);
}

template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix)
{
	return os << glm::to_string(matrix);
}

template<typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion)
{
	return os << glm::to_string(quaternion);
}

// Core log macros
#define UME_CORE_TRACE(...)    ::Ume::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define UME_CORE_INFO(...)     ::Ume::Log::GetCoreLogger()->info(__VA_ARGS__)
#define UME_CORE_WARN(...)     ::Ume::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define UME_CORE_ERROR(...)    ::Ume::Log::GetCoreLogger()->error(__VA_ARGS__)
#define UME_CORE_CRITICAL(...) ::Ume::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define UME_TRACE(...)         ::Ume::Log::GetClientLogger()->trace(__VA_ARGS__)
#define UME_INFO(...)          ::Ume::Log::GetClientLogger()->info(__VA_ARGS__)
#define UME_WARN(...)          ::Ume::Log::GetClientLogger()->warn(__VA_ARGS__)
#define UME_ERROR(...)         ::Ume::Log::GetClientLogger()->error(__VA_ARGS__)
#define UME_CRITICAL(...)      ::Ume::Log::GetClientLogger()->critical(__VA_ARGS__)
