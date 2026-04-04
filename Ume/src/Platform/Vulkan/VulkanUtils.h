#pragma once

#include "Ume/Core/Log.h"
#include <vulkan/vulkan.h>

namespace Ume {

	inline const char* VkResultToString(VkResult result)
	{
		switch (result)
		{
#define VK_RESULT_CASE(x) case x: return #x
			VK_RESULT_CASE(VK_SUCCESS);
			VK_RESULT_CASE(VK_NOT_READY);
			VK_RESULT_CASE(VK_TIMEOUT);
			VK_RESULT_CASE(VK_EVENT_SET);
			VK_RESULT_CASE(VK_EVENT_RESET);
			VK_RESULT_CASE(VK_INCOMPLETE);
			VK_RESULT_CASE(VK_ERROR_OUT_OF_HOST_MEMORY);
			VK_RESULT_CASE(VK_ERROR_OUT_OF_DEVICE_MEMORY);
			VK_RESULT_CASE(VK_ERROR_INITIALIZATION_FAILED);
			VK_RESULT_CASE(VK_ERROR_DEVICE_LOST);
			VK_RESULT_CASE(VK_ERROR_MEMORY_MAP_FAILED);
			VK_RESULT_CASE(VK_ERROR_LAYER_NOT_PRESENT);
			VK_RESULT_CASE(VK_ERROR_EXTENSION_NOT_PRESENT);
			VK_RESULT_CASE(VK_ERROR_FEATURE_NOT_PRESENT);
			VK_RESULT_CASE(VK_ERROR_INCOMPATIBLE_DRIVER);
			VK_RESULT_CASE(VK_ERROR_TOO_MANY_OBJECTS);
			VK_RESULT_CASE(VK_ERROR_FORMAT_NOT_SUPPORTED);
			VK_RESULT_CASE(VK_ERROR_SURFACE_LOST_KHR);
			VK_RESULT_CASE(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
			VK_RESULT_CASE(VK_SUBOPTIMAL_KHR);
			VK_RESULT_CASE(VK_ERROR_OUT_OF_DATE_KHR);
			VK_RESULT_CASE(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
			VK_RESULT_CASE(VK_ERROR_VALIDATION_FAILED_EXT);
#undef VK_RESULT_CASE
			default: return "VK_UNKNOWN";
		}
	}

}

#define VK_CHECK(x)                                                                  \
	do {                                                                             \
		VkResult _vk_result_ = (x);                                                  \
		if (_vk_result_ != VK_SUCCESS)                                               \
		{                                                                             \
			UME_CORE_ERROR("Vulkan error in {0}:{1} — {2} returned {3}",             \
			    __FILE__, __LINE__, #x, ::Ume::VkResultToString(_vk_result_));        \
			UME_CORE_ASSERT(false, "Vulkan call failed!");                            \
		}                                                                             \
	} while(0)
