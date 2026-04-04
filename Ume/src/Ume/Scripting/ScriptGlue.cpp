#include "umepch.h"
#include "ScriptGlue.h"
#include "ScriptEngine.h"

#include "Ume/Core/UUID.h"
#include "Ume/Core/KeyCodes.h"
#include "Ume/Core/Input.h"
#include "Ume/Core/Application.h"

#include "Ume/Scene/Scene.h"
#include "Ume/Scene/Entity.h"

#include "Ume/Physics/Physics2D.h"

#include "mono/metadata/object.h"
#include "mono/metadata/reflection.h"

#include "box2d/b2_body.h"
#include "box2d/b2_contact.h"

namespace Ume {

	namespace Utils {

		std::string MonoStringToString(MonoString* string)
		{
			char* cStr = mono_string_to_utf8(string);
			std::string str(cStr);
			mono_free(cStr);
			return str;
		}

	}

	static std::unordered_map<MonoType*, std::function<bool(Entity)>> s_EntityHasComponentFuncs;

#define UME_ADD_INTERNAL_CALL(Name) mono_add_internal_call("Ume.InternalCalls::" #Name, Name)

	static void NativeLog(MonoString* string, int parameter)
	{
		std::string str = Utils::MonoStringToString(string);
		const bool isError = parameter != 0;
		if (isError)
			UME_CORE_ERROR("[C#] {}", str);
		else
			UME_CORE_INFO("[C#] {}", str);

		ScriptEngine::SubmitManagedLog(str, isError);
	}

	static void NativeLog_Vector(glm::vec3* parameter, glm::vec3* outResult)
	{
		UME_CORE_WARN("Value: {0}", *parameter);
		*outResult = glm::normalize(*parameter);
	}

	static float NativeLog_VectorDot(glm::vec3* parameter)
	{
		UME_CORE_WARN("Value: {0}", *parameter);
		return glm::dot(*parameter, *parameter);
	}

	static MonoObject* GetScriptInstance(UUID entityID)
	{
		return ScriptEngine::GetManagedInstance(entityID);
	}

	static bool Entity_HasComponent(UUID entityID, MonoReflectionType* componentType)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);

		MonoType* managedType = mono_reflection_type_get_type(componentType);
		auto it = s_EntityHasComponentFuncs.find(managedType);
		if (it == s_EntityHasComponentFuncs.end())
		{
			UME_CORE_WARN("Entity_HasComponent called for unregistered managed component type");
			return false;
		}

		return it->second(entity);
	}

	static uint64_t Entity_FindEntityByName(MonoString* name)
	{
		char* nameCStr = mono_string_to_utf8(name);

		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->FindEntityByName(nameCStr);
		mono_free(nameCStr);

		if (!entity)
			return 0;
		
		return entity.GetUUID();
	}

	static uint64_t Entity_FindEntityByTag(MonoString* tag)
	{
		char* tagCStr = mono_string_to_utf8(tag);

		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);

		auto view = scene->GetAllEntitiesWith<TagComponent>();
		for (auto e : view)
		{
			Entity entity{ e, scene };
			if (entity.GetComponent<TagComponent>().Tag == tagCStr)
			{
				mono_free(tagCStr);
				return entity.GetUUID();
			}
		}

		mono_free(tagCStr);
		return 0;
	}

	static MonoString* Entity_GetTag(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);

		Entity entity = scene->GetEntityByUUID(entityID);
		if (!entity || !entity.HasComponent<TagComponent>())
			return ScriptEngine::CreateString("");

		const auto& tag = entity.GetComponent<TagComponent>().Tag;
		return ScriptEngine::CreateString(tag.c_str());
	}

	static void Entity_SetTag(UUID entityID, MonoString* tag)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);

		Entity entity = scene->GetEntityByUUID(entityID);
		if (!entity || !entity.HasComponent<TagComponent>())
			return;

		entity.GetComponent<TagComponent>().Tag = Utils::MonoStringToString(tag);
	}

	static bool Entity_IsOnGround(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);

		Entity entity = scene->GetEntityByUUID(entityID);
		if (!entity || !entity.HasComponent<Rigidbody2D>())
			return false;

		auto& rb2d = entity.GetComponent<Rigidbody2D>();
		if (!rb2d.RuntimeBody)
			return false;

		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		for (b2ContactEdge* edge = body->GetContactList(); edge; edge = edge->next)
		{
			b2Contact* contact = edge->contact;
			if (!contact || !contact->IsTouching())
				continue;

			b2Fixture* fixtureA = contact->GetFixtureA();
			b2Fixture* fixtureB = contact->GetFixtureB();
			const bool bodyIsFixtureA = fixtureA->GetBody() == body;
			const bool bodyIsFixtureB = fixtureB->GetBody() == body;
			if (!bodyIsFixtureA && !bodyIsFixtureB)
				continue;

			b2Body* otherBody = bodyIsFixtureA ? fixtureB->GetBody() : fixtureA->GetBody();
			const uint64_t otherEntityID = (uint64_t)otherBody->GetUserData().pointer;
			if (otherEntityID == 0)
				continue;

			Entity otherEntity = scene->GetEntityByUUID(otherEntityID);
			if (!otherEntity || !otherEntity.HasComponent<Rigidbody2D>())
				continue;

			auto& otherRb = otherEntity.GetComponent<Rigidbody2D>();
			if (!otherRb.IsGround)
				continue;

			// Ignore sensor-only contacts for grounded checks.
			if (fixtureA->IsSensor() || fixtureB->IsSensor())
				continue;

			b2WorldManifold worldManifold;
			contact->GetWorldManifold(&worldManifold);

			// Contact normal points from fixtureA to fixtureB.
			// Convert to "up" relative to this body and accept floor-like normals.
			const float upwardNormal = bodyIsFixtureA ? -worldManifold.normal.y : worldManifold.normal.y;
			if (upwardNormal > 0.35f)
				return true;
		}

		return false;
	}

	static void TransformComponent_GetTranslation(UUID entityID, glm::vec3* outTranslation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);

		*outTranslation = entity.GetComponent<TransformComponent>().Translation;
	}

	static void TransformComponent_SetTranslation(UUID entityID, glm::vec3* translation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);

		entity.GetComponent<TransformComponent>().Translation = *translation;
	}

	static void TransformComponent_GetScale(UUID entityID, glm::vec3* outScale)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);

		*outScale = entity.GetComponent<TransformComponent>().Scale;
	}

	static void TransformComponent_SetScale(UUID entityID, glm::vec3* scale)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);

		entity.GetComponent<TransformComponent>().Scale = *scale;
	}

	static void Rigidbody2DComponent_ApplyLinearImpulse(UUID entityID, glm::vec2* impulse, glm::vec2* point, bool wake)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);

		auto& rb2d = entity.GetComponent<Rigidbody2D>();
		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		body->ApplyLinearImpulse(b2Vec2(impulse->x, impulse->y), b2Vec2(point->x, point->y), wake);
	}

	static void Rigidbody2DComponent_ApplyLinearImpulseToCenter(UUID entityID, glm::vec2* impulse, bool wake)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);

		auto& rb2d = entity.GetComponent<Rigidbody2D>();
		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		body->ApplyLinearImpulseToCenter(b2Vec2(impulse->x, impulse->y), wake);
	}
	
	static void Rigidbody2DComponent_GetLinearVelocity(UUID entityID, glm::vec2* outLinearVelocity)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);

		auto& rb2d = entity.GetComponent<Rigidbody2D>();
		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		const b2Vec2& linearVelocity = body->GetLinearVelocity();
		*outLinearVelocity = glm::vec2(linearVelocity.x, linearVelocity.y);
	}

	static void Rigidbody2DComponent_SetLinearVelocity(UUID entityID, glm::vec2* linearVelocity)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);

		auto& rb2d = entity.GetComponent<Rigidbody2D>();
		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		body->SetLinearVelocity(b2Vec2(linearVelocity->x, linearVelocity->y));
	}

	static float Rigidbody2DComponent_GetGravityScale(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);

		auto& rb2d = entity.GetComponent<Rigidbody2D>();
		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		return body->GetGravityScale();
	}

	static void Rigidbody2DComponent_SetGravityScale(UUID entityID, float gravityScale)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);

		auto& rb2d = entity.GetComponent<Rigidbody2D>();
		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		body->SetGravityScale(gravityScale);
	}

	static Rigidbody2D::BodyType Rigidbody2DComponent_GetType(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);

		auto& rb2d = entity.GetComponent<Rigidbody2D>();
		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		return Utils::Rigidbody2DTypeFromBox2DBody(body->GetType());
	}
	
	static void Rigidbody2DComponent_SetType(UUID entityID, Rigidbody2D::BodyType bodyType)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);

		auto& rb2d = entity.GetComponent<Rigidbody2D>();
		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		body->SetType(Utils::Rigidbody2DTypeToBox2DBody(bodyType));
	}

	static MonoString* TextComponent_GetText(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);
		UME_CORE_ASSERT(entity.HasComponent<TextComponent>());

		auto& tc = entity.GetComponent<TextComponent>();
		return ScriptEngine::CreateString(tc.TextString.c_str());
	}

	static void TextComponent_SetText(UUID entityID, MonoString* textString)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);
		UME_CORE_ASSERT(entity.HasComponent<TextComponent>());

		auto& tc = entity.GetComponent<TextComponent>();
		tc.TextString = Utils::MonoStringToString(textString);
	}

	static void TextComponent_GetColor(UUID entityID, glm::vec4* color)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);
		UME_CORE_ASSERT(entity.HasComponent<TextComponent>());

		auto& tc = entity.GetComponent<TextComponent>();
		*color = tc.Color;
	}

	static void TextComponent_SetColor(UUID entityID, glm::vec4* color)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);
		UME_CORE_ASSERT(entity.HasComponent<TextComponent>());

		auto& tc = entity.GetComponent<TextComponent>();
		tc.Color = *color;
	}

	static float TextComponent_GetKerning(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);
		UME_CORE_ASSERT(entity.HasComponent<TextComponent>());

		auto& tc = entity.GetComponent<TextComponent>();
		return tc.Kerning;
	}

	static void TextComponent_SetKerning(UUID entityID, float kerning)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);
		UME_CORE_ASSERT(entity.HasComponent<TextComponent>());

		auto& tc = entity.GetComponent<TextComponent>();
		tc.Kerning = kerning;
	}

	static float TextComponent_GetLineSpacing(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);
		UME_CORE_ASSERT(entity.HasComponent<TextComponent>());

		auto& tc = entity.GetComponent<TextComponent>();
		return tc.LineSpacing;
	}

	static void TextComponent_SetLineSpacing(UUID entityID, float lineSpacing)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		UME_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		UME_CORE_ASSERT(entity);
		UME_CORE_ASSERT(entity.HasComponent<TextComponent>());

		auto& tc = entity.GetComponent<TextComponent>();
		tc.LineSpacing = lineSpacing;
	}

	static bool Input_IsKeyDown(KeyCode keycode)
	{
		return Input::IsKeyPressed(keycode);
	}

	static uint32_t Screen_GetWidth()
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		if (scene)
		{
			const uint32_t viewportWidth = scene->GetViewportWidth();
			if (viewportWidth > 0)
				return viewportWidth;
		}

		return Application::Get().GetWindow().GetWidth();
	}

	static uint32_t Screen_GetHeight()
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		if (scene)
		{
			const uint32_t viewportHeight = scene->GetViewportHeight();
			if (viewportHeight > 0)
				return viewportHeight;
		}

		return Application::Get().GetWindow().GetHeight();
	}

	static bool Screen_GetPrimaryCameraWorldExtents(float* outHalfWidth, float* outHalfHeight, glm::vec3* outCameraTranslation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		if (!scene)
			return false;

		Entity cameraEntity = scene->GetPrimaryCameraEntity();
		if (!cameraEntity || !cameraEntity.HasComponent<CameraComponent>() || !cameraEntity.HasComponent<TransformComponent>())
			return false;

		auto& cameraComponent = cameraEntity.GetComponent<CameraComponent>();
		auto& transform = cameraEntity.GetComponent<TransformComponent>();
		auto& camera = cameraComponent.Camera;

		if (camera.GetProjectionType() != SceneCamera::ProjectionType::Orthographic)
			return false;

		float aspectRatio = camera.GetAspectRatio();
		if (aspectRatio <= 0.0f)
		{
			const uint32_t viewportWidth = scene->GetViewportWidth();
			const uint32_t viewportHeight = scene->GetViewportHeight();
			if (viewportWidth == 0 || viewportHeight == 0)
				return false;

			aspectRatio = (float)viewportWidth / (float)viewportHeight;
		}

		*outHalfHeight = camera.GetOrthographicSize() * 0.5f;
		*outHalfWidth = *outHalfHeight * aspectRatio;
		*outCameraTranslation = transform.Translation;
		return true;
	}

	static float Screen_GetWorldLeft()
	{
		float halfWidth = 0.0f, halfHeight = 0.0f;
		glm::vec3 cameraTranslation{ 0.0f };
		if (!Screen_GetPrimaryCameraWorldExtents(&halfWidth, &halfHeight, &cameraTranslation))
			return 0.0f;

		return cameraTranslation.x - halfWidth;
	}

	static float Screen_GetWorldRight()
	{
		float halfWidth = 0.0f, halfHeight = 0.0f;
		glm::vec3 cameraTranslation{ 0.0f };
		if (!Screen_GetPrimaryCameraWorldExtents(&halfWidth, &halfHeight, &cameraTranslation))
			return 0.0f;

		return cameraTranslation.x + halfWidth;
	}

	static float Screen_GetWorldBottom()
	{
		float halfWidth = 0.0f, halfHeight = 0.0f;
		glm::vec3 cameraTranslation{ 0.0f };
		if (!Screen_GetPrimaryCameraWorldExtents(&halfWidth, &halfHeight, &cameraTranslation))
			return 0.0f;

		return cameraTranslation.y - halfHeight;
	}

	static float Screen_GetWorldTop()
	{
		float halfWidth = 0.0f, halfHeight = 0.0f;
		glm::vec3 cameraTranslation{ 0.0f };
		if (!Screen_GetPrimaryCameraWorldExtents(&halfWidth, &halfHeight, &cameraTranslation))
			return 0.0f;

		return cameraTranslation.y + halfHeight;
	}

	template<typename... Component>
	static void RegisterComponent()
	{
		([]()
		{
			std::string_view typeName = typeid(Component).name();
			size_t pos = typeName.find_last_of(':');
			std::string_view structName = typeName.substr(pos + 1);

			auto registerManagedType = [&](const std::string& managedTypename)
			{
				std::string mutableManagedTypename = managedTypename;
				MonoType* managedType = mono_reflection_type_from_name(mutableManagedTypename.data(), ScriptEngine::GetCoreAssemblyImage());
				if (!managedType)
					return false;

				s_EntityHasComponentFuncs[managedType] = [](Entity entity) { return entity.HasComponent<Component>(); };
				return true;
			};

			const std::string baseManagedTypename = fmt::format("Ume.{}", structName);
			bool registeredAny = registerManagedType(baseManagedTypename);

			const std::string_view suffix = "Component";
			const bool hasComponentSuffix = structName.size() >= suffix.size() &&
				structName.substr(structName.size() - suffix.size()) == suffix;
			if (!hasComponentSuffix)
			{
				const std::string aliasManagedTypename = fmt::format("Ume.{}Component", structName);
				registeredAny |= registerManagedType(aliasManagedTypename);
			}

			if (!registeredAny)
				UME_CORE_WARN("Managed component type not exposed in script core: {}", baseManagedTypename);
		}(), ...);
	}

	template<typename... Component>
	static void RegisterComponent(ComponentGroup<Component...>)
	{
		RegisterComponent<Component...>();
	}

	void ScriptGlue::RegisterComponents()
	{
		s_EntityHasComponentFuncs.clear();
		RegisterComponent(AllComponents{});
	}

	void ScriptGlue::RegisterFunctions()
	{
		UME_ADD_INTERNAL_CALL(NativeLog);
		UME_ADD_INTERNAL_CALL(NativeLog_Vector);
		UME_ADD_INTERNAL_CALL(NativeLog_VectorDot);

		UME_ADD_INTERNAL_CALL(GetScriptInstance);

		UME_ADD_INTERNAL_CALL(Entity_HasComponent);
		UME_ADD_INTERNAL_CALL(Entity_FindEntityByName);
		UME_ADD_INTERNAL_CALL(Entity_FindEntityByTag);
		UME_ADD_INTERNAL_CALL(Entity_GetTag);
		UME_ADD_INTERNAL_CALL(Entity_SetTag);
		UME_ADD_INTERNAL_CALL(Entity_IsOnGround);

		UME_ADD_INTERNAL_CALL(TransformComponent_GetTranslation);
		UME_ADD_INTERNAL_CALL(TransformComponent_SetTranslation);
		UME_ADD_INTERNAL_CALL(TransformComponent_GetScale);
		UME_ADD_INTERNAL_CALL(TransformComponent_SetScale);
		
		UME_ADD_INTERNAL_CALL(Rigidbody2DComponent_ApplyLinearImpulse);
		UME_ADD_INTERNAL_CALL(Rigidbody2DComponent_ApplyLinearImpulseToCenter);
		UME_ADD_INTERNAL_CALL(Rigidbody2DComponent_GetLinearVelocity);
		UME_ADD_INTERNAL_CALL(Rigidbody2DComponent_SetLinearVelocity);
		UME_ADD_INTERNAL_CALL(Rigidbody2DComponent_GetGravityScale);
		UME_ADD_INTERNAL_CALL(Rigidbody2DComponent_SetGravityScale);
		UME_ADD_INTERNAL_CALL(Rigidbody2DComponent_GetType);
		UME_ADD_INTERNAL_CALL(Rigidbody2DComponent_SetType);
		
		UME_ADD_INTERNAL_CALL(TextComponent_GetText);
		UME_ADD_INTERNAL_CALL(TextComponent_SetText);
		UME_ADD_INTERNAL_CALL(TextComponent_GetColor);
		UME_ADD_INTERNAL_CALL(TextComponent_SetColor);
		UME_ADD_INTERNAL_CALL(TextComponent_GetKerning);
		UME_ADD_INTERNAL_CALL(TextComponent_SetKerning);
		UME_ADD_INTERNAL_CALL(TextComponent_GetLineSpacing);
		UME_ADD_INTERNAL_CALL(TextComponent_SetLineSpacing);

		UME_ADD_INTERNAL_CALL(Input_IsKeyDown);
		UME_ADD_INTERNAL_CALL(Screen_GetWidth);
		UME_ADD_INTERNAL_CALL(Screen_GetHeight);
		UME_ADD_INTERNAL_CALL(Screen_GetWorldLeft);
		UME_ADD_INTERNAL_CALL(Screen_GetWorldRight);
		UME_ADD_INTERNAL_CALL(Screen_GetWorldBottom);
		UME_ADD_INTERNAL_CALL(Screen_GetWorldTop);
	}

}
