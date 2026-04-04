#include "SceneHierarchyPanel.h"
#include "Ume/Scene/Components.h"

#include "Ume/Project/Project.h"

#include "Ume/Scripting/ScriptEngine.h"
#include "Ume/UI/UI.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <glm/gtc/type_ptr.hpp>

#include <cstring>

#include <string>
#include <vector>
#include <algorithm>
#include <regex>

#ifdef _MSVC_LANG
  #define _CRT_SECURE_NO_WARNINGS
#endif

namespace Ume {

	namespace
	{
		constexpr const char* kSceneHierarchyEntityPayload = "SCENE_HIERARCHY_ENTITY";

		struct ScopedInspectorWidgetStyle
		{
			ScopedInspectorWidgetStyle()
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.164f, 0.168f, 0.176f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.196f, 0.202f, 0.212f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.212f, 0.220f, 0.235f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.204f, 0.208f, 0.216f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.230f, 0.236f, 0.246f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.257f, 0.447f, 0.715f, 0.28f));
				ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.204f, 0.208f, 0.216f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.230f, 0.236f, 0.246f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.257f, 0.447f, 0.715f, 0.28f));
				ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.286f, 0.525f, 0.859f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.356f, 0.600f, 0.945f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.286f, 0.525f, 0.859f, 1.0f));
			}

			~ScopedInspectorWidgetStyle()
			{
				ImGui::PopStyleColor(12);
				ImGui::PopStyleVar(2);
			}
		};

		void PushAxisButtonStyle(const ImVec4& axisColor)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.155f, 0.157f, 0.165f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.188f, 0.192f, 0.204f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(axisColor.x, axisColor.y, axisColor.z, 0.28f));
			ImGui::PushStyleColor(ImGuiCol_Text, axisColor);
		}

		void PopAxisButtonStyle()
		{
			ImGui::PopStyleColor(4);
		}

		float ClampScriptFloat(const ScriptField& field, float value)
		{
			if (field.HasRange)
			{
				if (value < field.RangeMin)
					value = field.RangeMin;
				if (value > field.RangeMax)
					value = field.RangeMax;
			}

			if (field.HasMaxValue && value > field.MaxValue)
				value = field.MaxValue;

			return value;
		}

		double ClampScriptDouble(const ScriptField& field, double value)
		{
			if (field.HasRange)
			{
				if (value < field.RangeMin)
					value = field.RangeMin;
				if (value > field.RangeMax)
					value = field.RangeMax;
			}

			if (field.HasMaxValue && value > field.MaxValue)
				value = field.MaxValue;

			return value;
		}

		int ClampScriptInt(const ScriptField& field, int value)
		{
			if (field.HasRange)
			{
				const int minValue = (int)field.RangeMin;
				const int maxValue = (int)field.RangeMax;
				if (value < minValue)
					value = minValue;
				if (value > maxValue)
					value = maxValue;
			}

			if (field.HasMaxValue)
			{
				const int maxValue = (int)field.MaxValue;
				if (value > maxValue)
					value = maxValue;
			}

			return value;
		}

		std::string GetEntityDisplayName(Scene* scene, UUID entityID)
		{
			if (!scene || (uint64_t)entityID == 0)
				return "None";

			Entity target = scene->GetEntityByUUID(entityID);
			if (!target)
				return "<Missing Entity>";

			return target.GetName();
		}

		bool DrawEntityReferenceField(const char* label, Scene* scene, UUID& entityID)
		{
			bool changed = false;
			const std::string currentName = GetEntityDisplayName(scene, entityID);

			if (ImGui::BeginCombo(label, currentName.c_str()))
			{
				const bool noneSelected = ((uint64_t)entityID == 0);
				if (ImGui::Selectable("None", noneSelected))
				{
					entityID = UUID(0);
					changed = true;
				}

				if (scene)
				{
					auto view = scene->GetAllEntitiesWith<IDComponent, TagComponent>();
					for (auto e : view)
					{
						Entity candidate{ e, scene };
						const uint64_t candidateID = candidate.GetUUID();
						const bool isSelected = ((uint64_t)entityID == candidateID);

						if (ImGui::Selectable(candidate.GetName().c_str(), isSelected))
						{
							entityID = UUID(candidateID);
							changed = true;
						}

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
				}

				ImGui::EndCombo();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kSceneHierarchyEntityPayload))
				{
					if (payload->DataSize == sizeof(uint64_t))
					{
						entityID = UUID(*(const uint64_t*)payload->Data);
						changed = true;
					}
				}
				ImGui::EndDragDropTarget();
			}

			return changed;
		}
	}

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
	{
		SetContext(context);
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& context)
	{
		m_Context = context;
		m_SelectionContext = {};
		m_AutoRequestedScriptBuild = false;
	}

	void SceneHierarchyPanel::SetScriptBuildCallback(std::function<void()> callback)
	{
		m_ScriptBuildCallback = std::move(callback);
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		static char sceneFilter[128] = "";
		static char inspectorFilter[128] = "";
		static int inspectorTab = 0;

		ImGui::Begin("Scene");
		if (ImGui::BeginTabBar("##SceneTabs", ImGuiTabBarFlags_FittingPolicyScroll))
		{
			if (ImGui::BeginTabItem("Scene"))
			{
				if (ImGui::Button("+", ImVec2(24.0f, 0.0f)))
					ImGui::OpenPopup("##SceneCreateEntityPopup");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(-1.0f);
				ImGui::InputTextWithHint("##SceneFilter", "Filter Nodes", sceneFilter, sizeof(sceneFilter));

				if (ImGui::BeginPopup("##SceneCreateEntityPopup"))
				{
					if (ImGui::MenuItem("Create Empty Entity"))
						m_Context->CreateEntity("Empty Entity");
					ImGui::EndPopup();
				}

				ImGui::Separator();
				ImGui::BeginChild("##SceneTree", ImVec2(0.0f, 0.0f), false);
				if (m_Context)
				{
					m_Context->m_Registry.view<IDComponent, RelationshipComponent>().each([&](auto entityID, IDComponent& id, RelationshipComponent& relationship)
					{
						if ((uint64_t)relationship.Parent != 0)
							return;

						Entity entity{ entityID, m_Context.get() };
						const auto& tag = entity.GetComponent<TagComponent>().Tag;
						if (sceneFilter[0] != '\0' && tag.find(sceneFilter) == std::string::npos)
							return;

						DrawEntityNode(entity);
					});

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kSceneHierarchyEntityPayload))
						{
							const uint64_t droppedEntityID = *(const uint64_t*)payload->Data;
							Entity droppedEntity = m_Context->GetEntityByUUID(UUID(droppedEntityID));
							if (droppedEntity)
								m_Context->UnparentEntity(droppedEntity);
						}
						ImGui::EndDragDropTarget();
					}

					if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
						m_SelectionContext = {};

					if (ImGui::BeginPopupContextWindow(0, 1, false))
					{
						if (ImGui::MenuItem("Create Empty Entity"))
							m_Context->CreateEntity("Empty Entity");
						ImGui::EndPopup();
					}
				}
				ImGui::EndChild();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Import"))
			{
				ImGui::TextDisabled("Import tools are not wired into this dock yet.");
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::End();

		ImGui::Begin("Inspector");
		if (ImGui::BeginTabBar("##InspectorTabs", ImGuiTabBarFlags_FittingPolicyScroll))
		{
			if (ImGui::BeginTabItem("Inspector", nullptr, inspectorTab == 0 ? ImGuiTabItemFlags_SetSelected : 0))
			{
				inspectorTab = 0;
				ImGui::SetNextItemWidth(-1.0f);
				ImGui::InputTextWithHint("##InspectorFilter", "Filter Properties", inspectorFilter, sizeof(inspectorFilter));
				ImGui::Separator();
				if (m_SelectionContext)
					DrawComponents(m_SelectionContext);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Signals", nullptr, inspectorTab == 1 ? ImGuiTabItemFlags_SetSelected : 0))
			{
				inspectorTab = 1;
				ImGui::TextDisabled("Signal routing UI is not available yet.");
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Groups", nullptr, inspectorTab == 2 ? ImGuiTabItemFlags_SetSelected : 0))
			{
				inspectorTab = 2;
				ImGui::TextDisabled("Group editing UI is not available yet.");
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::End();
	}

	void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
	{
		m_SelectionContext = entity;
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		auto& tag = entity.GetComponent<TagComponent>().Tag;
		auto& relationship = entity.GetComponent<RelationshipComponent>();
		const bool hasChildren = !relationship.Children.empty();

		ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (hasChildren)
			flags |= ImGuiTreeNodeFlags_OpenOnArrow;
		else
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());
		if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
			m_SelectionContext = entity;

		if (ImGui::BeginDragDropSource())
		{
			const uint64_t entityID = entity.GetUUID();
			ImGui::SetDragDropPayload(kSceneHierarchyEntityPayload, &entityID, sizeof(uint64_t));
			ImGui::TextUnformatted(tag.c_str());
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kSceneHierarchyEntityPayload))
			{
				const uint64_t droppedEntityID = *(const uint64_t*)payload->Data;
				Entity droppedEntity = m_Context->GetEntityByUUID(UUID(droppedEntityID));
				if (droppedEntity && droppedEntity != entity)
					m_Context->ReparentEntity(droppedEntity, entity);
			}
			ImGui::EndDragDropTarget();
		}

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete Entity"))
				entityDeleted = true;
			ImGui::EndPopup();
		}

		if (hasChildren && opened)
		{
			for (UUID childID : relationship.Children)
			{
				Entity childEntity = m_Context->GetEntityByUUID(childID);
				if (childEntity)
					DrawEntityNode(childEntity);
			}
			ImGui::TreePop();
		}

		if (entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if (m_SelectionContext == entity)
				m_SelectionContext = {};
		}
	}

	static void DrawVec2Control(const std::string& label, glm::vec2& values, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(2, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 6, 0 });
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.164f, 0.168f, 0.176f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.196f, 0.202f, 0.212f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.212f, 0.220f, 0.235f, 1.0f));

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight, lineHeight };

		PushAxisButtonStyle(ImVec4{ 0.855f, 0.333f, 0.333f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
			values.x = resetValue;
		ImGui::PopFont();
		PopAxisButtonStyle();

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		PushAxisButtonStyle(ImVec4{ 0.580f, 0.766f, 0.337f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetValue;
		ImGui::PopFont();
		PopAxisButtonStyle();

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 6, 0 });
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.164f, 0.168f, 0.176f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.196f, 0.202f, 0.212f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.212f, 0.220f, 0.235f, 1.0f));

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight, lineHeight };

		PushAxisButtonStyle(ImVec4{ 0.855f, 0.333f, 0.333f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
			values.x = resetValue;
		ImGui::PopFont();
		PopAxisButtonStyle();

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		PushAxisButtonStyle(ImVec4{ 0.580f, 0.766f, 0.337f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetValue;
		ImGui::PopFont();
		PopAxisButtonStyle();

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		PushAxisButtonStyle(ImVec4{ 0.353f, 0.596f, 0.937f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
			values.z = resetValue;
		ImGui::PopFont();
		PopAxisButtonStyle();

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}
	
	template<typename T, typename UIFunction>
	static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
		if (entity.HasComponent<T>())
		{
			auto& component = entity.GetComponent<T>();
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 8, 6 });
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.204f, 0.208f, 0.216f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.230f, 0.236f, 0.246f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.257f, 0.447f, 0.715f, 0.28f));
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar();
			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.180f, 0.184f, 0.192f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.210f, 0.216f, 0.226f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.257f, 0.447f, 0.715f, 0.28f));
			if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
			{
				ImGui::OpenPopup("ComponentSettings");
			}
			ImGui::PopStyleColor(3);

			bool removeComponent = false;
			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Remove component"))
					removeComponent = true;

				ImGui::EndPopup();
			}

			if (open)
			{
				uiFunction(component);
				ImGui::TreePop();
			}

			if (removeComponent)
				entity.RemoveComponent<T>();
		}
	}

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		ScopedInspectorWidgetStyle inspectorWidgetStyle;

		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strncpy_s(buffer, sizeof(buffer), tag.c_str(), sizeof(buffer));
			if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
			{
				tag = std::string(buffer);
			}
		}

		ImGui::SameLine();
		ImGui::PushItemWidth(-1);

		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponent");

		if (ImGui::BeginPopup("AddComponent"))
		{
			DisplayAddComponentEntry<CameraComponent>("Camera");
			DisplayAddComponentEntry<ScriptComponent>("Script");
			DisplayAddComponentEntry<SpriteRendererComponent>("Sprite Renderer");
			DisplayAddComponentEntry<CircleRendererComponent>("Circle Renderer");
			DisplayAddComponentEntry<Rigidbody2D>("Rigidbody 2D");
			DisplayAddComponentEntry<BoxCollider2DComponent>("Box Collider 2D");
			DisplayAddComponentEntry<ContactListenerComponent>("Contact Listener 2D");
			DisplayAddComponentEntry<CircleCollider2DComponent>("Circle Collider 2D");
			DisplayAddComponentEntry<TextComponent>("Text Component");

			ImGui::EndPopup();
		}

		ImGui::PopItemWidth();

		DrawComponent<TransformComponent>("Transform", entity, [](auto& component)
		{
			DrawVec3Control("Translation", component.Translation);
			glm::vec3 rotation = glm::degrees(component.Rotation);
			DrawVec3Control("Rotation", rotation);
			component.Rotation = glm::radians(rotation);
			DrawVec3Control("Scale", component.Scale, 1.0f);
		});

		DrawComponent<CameraComponent>("Camera", entity, [](auto& component)
		{
			auto& camera = component.Camera;

			ImGui::Checkbox("Primary", &component.Primary);

			const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
			const char* currentProjectionTypeString = projectionTypeStrings[(int)camera.GetProjectionType()];
			if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
			{
				for (int i = 0; i < 2; i++)
				{
					bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
					if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
					{
						currentProjectionTypeString = projectionTypeStrings[i];
						camera.SetProjectionType((SceneCamera::ProjectionType)i);
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}

			if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
			{
				float perspectiveVerticalFov = glm::degrees(camera.GetPerspectiveVerticalFOV());
				if (ImGui::DragFloat("Vertical FOV", &perspectiveVerticalFov))
					camera.SetPerspectiveVerticalFOV(glm::radians(perspectiveVerticalFov));

				float perspectiveNear = camera.GetPerspectiveNearClip();
				if (ImGui::DragFloat("Near", &perspectiveNear))
					camera.SetPerspectiveNearClip(perspectiveNear);

				float perspectiveFar = camera.GetPerspectiveFarClip();
				if (ImGui::DragFloat("Far", &perspectiveFar))
					camera.SetPerspectiveFarClip(perspectiveFar);
			}

			if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
			{
				float orthoSize = camera.GetOrthographicSize();
				if (ImGui::DragFloat("Size", &orthoSize))
					camera.SetOrthographicSize(orthoSize);

				float orthoNear = camera.GetOrthographicNearClip();
				if (ImGui::DragFloat("Near", &orthoNear))
					camera.SetOrthographicNearClip(orthoNear);

				float orthoFar = camera.GetOrthographicFarClip();
				if (ImGui::DragFloat("Far", &orthoFar))
					camera.SetOrthographicFarClip(orthoFar);

				ImGui::Checkbox("Fixed Aspect Ratio", &component.FixedAspectRatio);
			}
		});

		DrawComponent<ScriptComponent>("Script", entity, [this, entity, scene = m_Context](auto& component) mutable
		{
			auto entityClasses = ScriptEngine::GetEntityClasses();
			const bool hasLoadedScriptClasses = !entityClasses.empty();
			std::vector<std::string> classNames;
			classNames.reserve(entityClasses.size());
			for (const auto& [fullName, scriptClass] : entityClasses)
				classNames.push_back(fullName);

			if (classNames.empty())
			{
				auto discoveredOnDisk = DiscoverScriptClassesFromProjectSource();
				classNames.insert(classNames.end(), discoveredOnDisk.begin(), discoveredOnDisk.end());
			}

			std::sort(classNames.begin(), classNames.end());

			bool scriptClassExists = ScriptEngine::EntityClassExists(component.ClassName);
			const char* selectedClass = component.ClassName.empty() ? "<None>" : component.ClassName.c_str();

			UI::ScopedStyleColor textColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.3f, 1.0f), !component.ClassName.empty() && !scriptClassExists);

			if (ImGui::BeginCombo("Class", selectedClass))
			{
				bool isNoneSelected = component.ClassName.empty();
				if (ImGui::Selectable("<None>", isNoneSelected))
				{
					component.ClassName.clear();
					auto& fields = ScriptEngine::GetScriptFieldMap(entity);
					fields.clear();
				}

				for (const auto& className : classNames)
				{
					const bool isSelected = component.ClassName == className;
					if (ImGui::Selectable(className.c_str(), isSelected))
					{
						component.ClassName = className;
						auto& fields = ScriptEngine::GetScriptFieldMap(entity);
						fields.clear();
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}

			// Re-evaluate after potential selection changes from combo.
			scriptClassExists = ScriptEngine::EntityClassExists(component.ClassName);

			if (!hasLoadedScriptClasses)
			{
				if (!classNames.empty())
					ImGui::TextDisabled("Scripts found on disk. Build scripts to load classes.");
				else
					ImGui::TextDisabled("No script classes found.");

				if (!m_AutoRequestedScriptBuild && m_ScriptBuildCallback)
				{
					m_ScriptBuildCallback();
					m_AutoRequestedScriptBuild = true;
				}

				if (ImGui::Button("Build Scripts"))
				{
					if (m_ScriptBuildCallback)
						m_ScriptBuildCallback();
				}
			}
			else
			{
				m_AutoRequestedScriptBuild = false;
			}

			// Fields
			bool sceneRunning = scene->IsRunning();
			if (sceneRunning)
			{
				Ref<ScriptInstance> scriptInstance = ScriptEngine::GetEntityScriptInstance(entity.GetUUID());
				if (scriptInstance)
				{
					const auto& fields = scriptInstance->GetScriptClass()->GetFields();
					for (const auto& [name, field] : fields)
					{
						if (field.Type == ScriptFieldType::Float)
						{
							float data = scriptInstance->GetFieldValue<float>(name);
							bool changed = field.HasRange
								? ImGui::SliderFloat(name.c_str(), &data, field.RangeMin, field.RangeMax)
								: ImGui::DragFloat(name.c_str(), &data);
							if (changed)
							{
								data = ClampScriptFloat(field, data);
								scriptInstance->SetFieldValue(name, data);
							}
						}
					}
				}
			}
			else
			{
				if (scriptClassExists)
				{
					Ref<ScriptClass> entityClass = ScriptEngine::GetEntityClass(component.ClassName);
					if (!entityClass)
						return;

					const auto& fields = entityClass->GetFields();

					auto& entityFields = ScriptEngine::GetScriptFieldMap(entity);
					for (const auto& [name, field] : fields)
					{
						// Field has been set in editor
						if (entityFields.find(name) != entityFields.end())
						{
							ScriptFieldInstance& scriptField = entityFields.at(name);

							// Display control to set it maybe
							if (field.Type == ScriptFieldType::Float)
							{
								float data = scriptField.GetValue<float>();
								bool changed = field.HasRange
									? ImGui::SliderFloat(name.c_str(), &data, field.RangeMin, field.RangeMax)
									: ImGui::DragFloat(name.c_str(), &data);
								if (changed)
								{
									data = ClampScriptFloat(field, data);
									scriptField.SetValue(data);
								}
							}
							if (field.Type == ScriptFieldType::Double)
							{
								double data = scriptField.GetValue<double>();
								if (ImGui::InputDouble(name.c_str(), &data))
								{
									data = ClampScriptDouble(field, data);
									scriptField.SetValue(data);
								}
							}
							if (field.Type == ScriptFieldType::Bool)
							{
								bool data = scriptField.GetValue<bool>();
								if (ImGui::Checkbox(name.c_str(), &data))
									scriptField.SetValue(data);
							}
							if (field.Type == ScriptFieldType::Int)
							{
								int data = scriptField.GetValue<int>();
								bool changed = field.HasRange
									? ImGui::SliderInt(name.c_str(), &data, (int)field.RangeMin, (int)field.RangeMax)
									: ImGui::InputInt(name.c_str(), &data);
								if (changed)
								{
									data = ClampScriptInt(field, data);
									scriptField.SetValue(data);
								}
							}
							if (field.Type == ScriptFieldType::String)
							{
								std::string data = scriptField.GetValue<std::string>();
								if (ImGui::InputText(name.c_str(), &data))
									scriptField.SetValue(data);
							}
							if (field.Type == ScriptFieldType::Vector2)
							{
								glm::vec2 data = scriptField.GetValue<glm::vec2>();
								DrawVec2Control(name.c_str(), data);
								scriptField.SetValue(data);
							}
							if (field.Type == ScriptFieldType::Vector3)
							{
								glm::vec3 data = scriptField.GetValue<glm::vec3>();
								DrawVec3Control(name.c_str(), data);
								scriptField.SetValue(data);
							}
							if (field.Type == ScriptFieldType::ColorPicker)
							{
								glm::vec4 data = scriptField.GetValue<glm::vec4>();
								ImGui::ColorEdit4(name.c_str(), glm::value_ptr(data));
								scriptField.SetValue(data);
							}
							if (field.Type == ScriptFieldType::Entity)
							{
								UUID data = scriptField.GetValue<UUID>();
								if (DrawEntityReferenceField(name.c_str(), scene.get(), data))
									scriptField.SetValue(data);
							}
						}
						else
						{
							// Display control to set it maybe
							if (field.Type == ScriptFieldType::Float)
							{
								float data = 0.0f;
								ScriptEngine::GetScriptFieldDefaultValue(component.ClassName, name, &data, sizeof(data));
								bool changed = field.HasRange
									? ImGui::SliderFloat(name.c_str(), &data, field.RangeMin, field.RangeMax)
									: ImGui::DragFloat(name.c_str(), &data);
								if (changed)
								{
									data = ClampScriptFloat(field, data);
									ScriptFieldInstance& fieldInstance = entityFields[name];
									fieldInstance.Field = field;
									fieldInstance.SetValue(data);
								}
							}
							if (field.Type == ScriptFieldType::Double)
							{
								double data = 0.0f;
								ScriptEngine::GetScriptFieldDefaultValue(component.ClassName, name, &data, sizeof(data));
								if (ImGui::InputDouble(name.c_str(), &data))
								{
									data = ClampScriptDouble(field, data);
									ScriptFieldInstance& fieldInstance = entityFields[name];
									fieldInstance.Field = field;
									fieldInstance.SetValue(data);
								}
							}
							if (field.Type == ScriptFieldType::Bool)
							{
								bool data = false;
								ScriptEngine::GetScriptFieldDefaultValue(component.ClassName, name, &data, sizeof(data));
								if (ImGui::Checkbox(name.c_str(), &data))
								{
									ScriptFieldInstance& fieldInstance = entityFields[name];
									fieldInstance.Field = field;
									fieldInstance.SetValue(data);
								}
							}
							if (field.Type == ScriptFieldType::Int)
							{
								int data = 0;
								ScriptEngine::GetScriptFieldDefaultValue(component.ClassName, name, &data, sizeof(data));
								bool changed = field.HasRange
									? ImGui::SliderInt(name.c_str(), &data, (int)field.RangeMin, (int)field.RangeMax)
									: ImGui::InputInt(name.c_str(), &data);
								if (changed)
								{
									data = ClampScriptInt(field, data);
									ScriptFieldInstance& fieldInstance = entityFields[name];
									fieldInstance.Field = field;
									fieldInstance.SetValue(data);
								}
							}
							if (field.Type == ScriptFieldType::String)
							{
								std::string data = "";
								if (ImGui::InputText(name.c_str(), &data))
								{
									ScriptFieldInstance& fieldInstance = entityFields[name];
									fieldInstance.Field = field;
									fieldInstance.SetValue(data);
								}
							}
							if (field.Type == ScriptFieldType::Vector3)
							{
								glm::vec3 data = { 0, 0, 0 };
								ScriptEngine::GetScriptFieldDefaultValue(component.ClassName, name, &data, sizeof(data));
								if (ImGui::DragFloat3(name.c_str(), glm::value_ptr(data), 0.1f))
								{
									ScriptFieldInstance& fieldInstance = entityFields[name];
									fieldInstance.Field = field;
									fieldInstance.SetValue(data);
								}
							}
							if (field.Type == ScriptFieldType::Vector2)
							{
								glm::vec2 data = { 0, 0 };
								ScriptEngine::GetScriptFieldDefaultValue(component.ClassName, name, &data, sizeof(data));
								if (ImGui::DragFloat2(name.c_str(), glm::value_ptr(data), 0.1f))
								{
									ScriptFieldInstance& fieldInstance = entityFields[name];
									fieldInstance.Field = field;
									fieldInstance.SetValue(data);
								}
							}
							if (field.Type == ScriptFieldType::ColorPicker)
							{
								glm::vec4 data = { 0, 0, 0, 1 };
								ScriptEngine::GetScriptFieldDefaultValue(component.ClassName, name, &data, sizeof(data));
								if (ImGui::ColorEdit4(name.c_str(), glm::value_ptr(data)))
								{
									ScriptFieldInstance& fieldInstance = entityFields[name];
									fieldInstance.Field = field;
									fieldInstance.SetValue(data);
								}
							}
							if (field.Type == ScriptFieldType::Entity)
							{
								UUID data = UUID(0);
								if (DrawEntityReferenceField(name.c_str(), scene.get(), data))
								{
									ScriptFieldInstance& fieldInstance = entityFields[name];
									fieldInstance.Field = field;
									fieldInstance.SetValue(data);
								}
							}
						}
					}
				}
			}
		});

		DrawComponent<SpriteRendererComponent>("Sprite Renderer", entity, [](auto& component)
		{
			const char* blendModeNames[] = { "Alpha", "Additive", "Multiply" };

			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
			
			ImGui::Button("Texture", ImVec2(100.0f, 0.0f));
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					const wchar_t* path = (const wchar_t*)payload->Data;
					std::filesystem::path texturePath(path);
					Ref<Texture2D> texture = Texture2D::Create(texturePath.string());
					if (texture->IsLoaded())
						component.Texture = texture;
					else
						UME_WARN("Could not load texture {0}", texturePath.filename().string());
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::DragFloat("Tiling Factor", &component.TilingFactor, 0.1f, 0.0f, 100.0f);
			int blendMode = (int)component.RenderBlendMode;
			if (ImGui::Combo("Blend Mode", &blendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames)))
				component.RenderBlendMode = (SpriteRendererComponent::BlendMode)blendMode;

			ImGui::DragInt("Sorting Layer", &component.SortingLayer, 1.0f, -1000, 1000);
			ImGui::Checkbox("Depth Test", &component.DepthTest);
			ImGui::Checkbox("Depth Write", &component.DepthWrite);
			ImGui::Checkbox("Cull", &component.Cull);
		});

		DrawComponent<CircleRendererComponent>("Circle Renderer", entity, [](auto& component)
		{
			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
			ImGui::DragFloat("Thickness", &component.Thickness, 0.025f, 0.0f, 1.0f);
			ImGui::DragFloat("Fade", &component.Fade, 0.00025f, 0.0f, 1.0f);
		});

		DrawComponent<Rigidbody2D>("Rigidbody 2D", entity, [](auto& component)
		{
			const char* bodyTypeStrings[] = { "Static", "Dynamic", "Kinematic"};
			const char* currentBodyTypeString = bodyTypeStrings[(int)component.Type];
			if (ImGui::BeginCombo("Body Type", currentBodyTypeString))
			{
				for (int i = 0; i < 2; i++)
				{
					bool isSelected = currentBodyTypeString == bodyTypeStrings[i];
					if (ImGui::Selectable(bodyTypeStrings[i], isSelected))
					{
						currentBodyTypeString = bodyTypeStrings[i];
						component.Type = (Rigidbody2D::BodyType)i;
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}

			ImGui::Checkbox("Fixed Rotation", &component.FixedRotation);
			ImGui::Checkbox("Is Ground", &component.IsGround);
		});

		DrawComponent<BoxCollider2DComponent>("Box Collider 2D", entity, [](auto& component)
		{
			ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
			ImGui::DragFloat2("Size", glm::value_ptr(component.Size));
			ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Restitution Threshold", &component.RestitutionThreshold, 0.01f, 0.0f);
		});

		DrawComponent<CircleCollider2DComponent>("Circle Collider 2D", entity, [](auto& component)
		{
			ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
			ImGui::DragFloat("Radius", &component.Radius);
			ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Restitution Threshold", &component.RestitutionThreshold, 0.01f, 0.0f);
		});

		DrawComponent<ContactListenerComponent>("Contact Listener 2D", entity, [](auto& component)
			{
				ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
				ImGui::DragFloat2("Size", glm::value_ptr(component.Size));
			});

		DrawComponent<TextComponent>("Text Renderer", entity, [](auto& component)
		{
			ImGui::InputTextMultiline("Text String", &component.TextString);
			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
			ImGui::DragFloat("Kerning", &component.Kerning, 0.025f);
			ImGui::DragFloat("Line Spacing", &component.LineSpacing, 0.025f);
		});

	}
	
	template<typename T>
	void SceneHierarchyPanel::DisplayAddComponentEntry(const std::string& entryName) {
		if (!m_SelectionContext.HasComponent<T>())
		{
			if (ImGui::MenuItem(entryName.c_str()))
			{
				m_SelectionContext.AddComponent<T>();
				ImGui::CloseCurrentPopup();
			}
		}
	}

	std::string ReadFile(const std::filesystem::path& filepath)
	{
		UME_PROFILE_FUNCTION();

		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
		if (in)
		{
			in.seekg(0, std::ios::end);
			size_t size = in.tellg();
			if (size != -1)
			{
				result.resize(size);
				in.seekg(0, std::ios::beg);
				in.read(&result[0], size);
			}
			else
			{
				UME_CORE_ERROR("Could not read from file '{0}'", filepath);
			}
		}
		else
		{
			UME_CORE_ERROR("Could not open file '{0}'", filepath);
		}

		return result;
	}

	std::vector<std::string> DiscoverScriptClassesFromProjectSource()
	{
		std::vector<std::string> discoveredClasses;
		if (!Project::GetActive())
			return discoveredClasses;

		std::filesystem::path scriptsRoot = Project::GetScriptRootDirectory();
		if (scriptsRoot.empty() || !std::filesystem::exists(scriptsRoot))
			return discoveredClasses;

		std::filesystem::path sourceRoot = scriptsRoot / "Source";
		if (!std::filesystem::exists(sourceRoot) || !std::filesystem::is_directory(sourceRoot))
			sourceRoot = scriptsRoot;

		const std::regex namespaceRegex(R"(\bnamespace\s+([A-Za-z_][A-Za-z0-9_.]*))");
		const std::regex classRegex(R"(\bclass\s+([A-Za-z_][A-Za-z0-9_]*)\s*:\s*([A-Za-z_][A-Za-z0-9_.]*))");

		for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceRoot))
		{
			if (!entry.is_regular_file() || entry.path().extension() != ".cs")
				continue;

			const std::string source = ReadFile(entry.path());
			if (source.empty())
				continue;

			std::string namespaceName;
			std::smatch namespaceMatch;
			if (std::regex_search(source, namespaceMatch, namespaceRegex) && namespaceMatch.size() > 1)
				namespaceName = namespaceMatch[1].str();

			auto classBegin = std::sregex_iterator(source.begin(), source.end(), classRegex);
			auto classEnd = std::sregex_iterator();
			for (auto it = classBegin; it != classEnd; ++it)
			{
				const std::smatch match = *it;
				if (match.size() < 3)
					continue;

				const std::string className = match[1].str();
				const std::string baseType = match[2].str();
				if (baseType != "Entity" && baseType != "Ume.Entity")
					continue;

				if (namespaceName.empty())
					discoveredClasses.push_back(className);
				else
					discoveredClasses.push_back(namespaceName + "." + className);
			}
		}

		std::sort(discoveredClasses.begin(), discoveredClasses.end());
		discoveredClasses.erase(std::unique(discoveredClasses.begin(), discoveredClasses.end()), discoveredClasses.end());
		return discoveredClasses;
	}

	char* SetScript(const std::filesystem::path& path)
	{
		std::string source = ReadFile(path);
		const char* typeToken = "namespace";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken, 0);
		size_t eol = source.find_first_of("\r\n", pos);
		UME_CORE_ASSERT(eol != std::string::npos, "Syntax error");
		size_t begin = pos + typeTokenLength + 1;
		std::string type = source.substr(begin, eol - begin);

		std::string apath = path.filename().string();
		char* fileName = strtok(const_cast<char*>(apath.c_str()), ".");

		uint32_t newSize = strlen(const_cast<char*>(type.c_str())) + strlen(fileName) + 2;
		char result[256];
		strcpy(result, const_cast<char*>(type.c_str()));
		strcat(result, ".");
		strcat(result, fileName);

		UME_CORE_INFO("{0}", result);
		return result;
	}

}
