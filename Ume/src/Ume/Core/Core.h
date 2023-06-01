#pragma once

#include <memory>

// Platform support
#ifdef _WIN32
	#ifdef _WIN64
		#define UME_PLATFORM_WINDOWS
	#else
		#error "x86 Builds are not supported!"
	#endif
#elif defined(__APPLE__) || defined(__MACH__)
	#include <TargetConditional.h>

	#if TARGET_IPHONE_SIMULATOR == 1
		#error "IOS simulator is not supported!"
	#elif TARGET_OS_IPHONE == 1
		#define UME_PLATFORM_IOS
		#error "IOS is not supported!"
	#elif TARGET_OS_MAC == 1
		#define UME_PLATFORM_MACOS
		#error "MacOS is not supported!"
	#else
		#error "Unknown Apple platform!"
	#endif
#elif defined(__ANDROID__)
	#define UME_PLATFORM_ANDROID
	#error "Android is not suppoerted!"
#elif defined(__linux__)
	#define UME_PLATFORM_LINUX
	#error "Linux is not supported!"
#else
	#error "Unknown platform!"
#endif

// DLL support
#ifdef UME_PLATFORM_WINDOWS
	#if UME_DYNAMIC_LINK
		#ifdef UME_BUILD_DLL
			#define UME_API __declspec(dllexport)
		#else
			#define UME_API __declspec(dllimport)
		#endif
	#else
		#define UME_API
	#endif
#else
	#error Ume Engine only support Windows!
#endif

#ifdef UME_DEBUG
	#define UME_ENABLE_ASSERTS
#endif

#ifdef UME_ENABLE_ASSERTS
#define UME_ASSERT(x, ...) \
	{\
		if (!(x))\
		{\
			UME_ERROR("Assertion Failed: {0}", __VA_ARGS__);\
			__debugbreak();\
		}\
	}
#define UME_CORE_ASSERT(x, ...)\
	{\
		if (!(x))\
		{\
			UME_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__);\
			__debugbreak();\
		}\
	}
#else
#define UME_ASSERT(x, ...)
#define UME_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

//#define UME_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)
#define UME_BIND_EVENT_FN(fn) [this](auto&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Ume
{
	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref   = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}
}