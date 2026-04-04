#pragma once

#include "Ume/Core/Base.h"
#include "Ume/Scene/Scene.h"
#include "Ume/Scene/Entity.h"

#include <functional>

namespace Ume {

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& scene);

		void SetContext(const Ref<Scene>& scene);
		void SetScriptBuildCallback(std::function<void()> callback);

		void OnImGuiRender();

		Entity GetSelectedEntity() const { return m_SelectionContext; }
		void SetSelectedEntity(Entity entity);
	private:
		template<typename T>
		void DisplayAddComponentEntry(const std::string& entryName);
	
		void DrawEntityNode(Entity entity);
		void DrawComponents(Entity entity);
	private:
		Ref<Scene> m_Context;
		Entity m_SelectionContext;
		std::function<void()> m_ScriptBuildCallback;
		bool m_AutoRequestedScriptBuild = false;
	};

}
