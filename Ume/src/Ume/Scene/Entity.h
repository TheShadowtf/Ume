#pragma once

#include "Scene.h"
#include "entt.hpp"

namespace Ume
{
	class Entity
	{
	public:
		Entity();

		template<typename T>
		bool HasComponent()
		{

		}
	private:
		entt::entity m_EntityHandle;
		Scene* m_Scene;
	};
}