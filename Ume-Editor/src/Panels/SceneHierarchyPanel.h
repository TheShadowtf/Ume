#pragma once

#include "Ume/Core/Core.h"
#include "Ume/Core/Debug/Log.h"
#include "Ume/Scene/Scene.h"
#include "Ume/Scene/Entity.h"

namespace Ume
{
	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene> context);

		void SetContext(const Ref<Scene> context);
		void OnImGuiRender();
	private:
		void DrawEntityNode(Entity entity);
		void DrawComponents(Entity entity);
	private:
		Ref<Scene> m_Context;
		Entity m_SelectionContext;
	};
}