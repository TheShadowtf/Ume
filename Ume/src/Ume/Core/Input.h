#pragma once

#include "Ume/Core/KeyCodes.h"
#include "Ume/Core/MouseCodes.h"

#include <glm/glm.hpp>

namespace Ume {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);

		static bool IsMouseButtonPressed(MouseCode button);
		static glm::vec2 GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	};
}
