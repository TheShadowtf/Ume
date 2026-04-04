#pragma once

#include "Ume/Core/Base.h"
#include "Ume/Core/Log.h"
#include <filesystem>

#ifdef UME_ENABLE_ASSERTS

	// Alteratively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
	// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
	#define UME_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { UME##type##ERROR(msg, __VA_ARGS__); UME_DEBUGBREAK(); } }
	#define UME_INTERNAL_ASSERT_WITH_MSG(type, check, ...) UME_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
	#define UME_INTERNAL_ASSERT_NO_MSG(type, check) UME_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", UME_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

	#define UME_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define UME_INTERNAL_ASSERT_GET_MACRO(...) UME_EXPAND_MACRO( UME_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, UME_INTERNAL_ASSERT_WITH_MSG, UME_INTERNAL_ASSERT_NO_MSG) )

	// Currently accepts at least the condition and one additional parameter (the message) being optional
	#define UME_ASSERT(...) UME_EXPAND_MACRO( UME_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
	#define UME_CORE_ASSERT(...) UME_EXPAND_MACRO( UME_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
	#define UME_ASSERT(...)
	#define UME_CORE_ASSERT(...)
#endif
