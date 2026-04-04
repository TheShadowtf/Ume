#include "ConsolePanel.h"
#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <sstream>

namespace Ume {

	namespace
	{
		bool IsErrorMessage(const std::string& message)
		{
			return message.find("error") != std::string::npos
				|| message.find("Error") != std::string::npos
				|| message.find("failed") != std::string::npos
				|| message.find("exception") != std::string::npos
				|| message.find("Assertion") != std::string::npos;
		}

		bool LooksLikeStackTraceLine(const std::string& message)
		{
			return message.find(" at ") != std::string::npos
				|| message.find("::") != std::string::npos
				|| message.find("Stack trace") != std::string::npos;
		}
	}

	void ConsolePanel::OnImGuiRender()
	{
		static int topSection = 1;

		ImGui::SetNextWindowSize(ImVec2(760.0f, 280.0f), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Debugger"))
		{
			if (ImGui::Selectable("Stack Trace", topSection == 0, 0, ImVec2(90.0f, 20.0f)))
				topSection = 0;
			ImGui::SameLine(0.0f, 6.0f);
			if (ImGui::Selectable("Errors", topSection == 1, 0, ImVec2(64.0f, 20.0f)))
				topSection = 1;

			ImGui::Separator();
			if (ImGui::Button("Clear"))
				ClearMessages();
			ImGui::SameLine();
			ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
			ImGui::Separator();

			ImGui::BeginChild("##ConsoleOutputRegion", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NoScrollbar);
			bool anyVisibleMessage = false;
			for (const auto& msg : m_Messages)
			{
				const bool showMessage = topSection == 0 ? LooksLikeStackTraceLine(msg.Text) : IsErrorMessage(msg.Text);
				if (!showMessage)
					continue;

				anyVisibleMessage = true;
				ImGui::TextColored(ImVec4(msg.Color.r, msg.Color.g, msg.Color.b, msg.Color.a), "%s", msg.Text.c_str());
			}

			if (!anyVisibleMessage)
				ImGui::TextDisabled(topSection == 0 ? "No stack trace available." : "No errors logged.");

			if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				ImGui::SetScrollHereY(1.0f);
			ImGui::EndChild();
		}
		ImGui::End();
	}

	void ConsolePanel::AddMessage(const std::string& message, const glm::vec4& color)
	{
		m_Messages.push_back({ message, color });
	}

	void ConsolePanel::ClearMessages()
	{
		m_Messages.clear();
	}

}
