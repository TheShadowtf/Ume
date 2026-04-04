#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>

namespace Ume {

	class ConsolePanel
	{
	public:
		ConsolePanel() = default;

		void OnImGuiRender();
		void AddMessage(const std::string& message, const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f });
		void ClearMessages();
		bool HasMessages() const { return !m_Messages.empty(); }
	private:
		struct Message
		{
			std::string Text;
			glm::vec4 Color;
		};

		std::vector<Message> m_Messages;
		bool m_AutoScroll = true;
			std::string m_DisplayBuffer;
	};

}
