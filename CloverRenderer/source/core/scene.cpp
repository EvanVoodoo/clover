#include "core/scene.hpp"
#include <imgui.h>
#include <core/engine.hpp>
#include <core/transform.hpp>
#include <rendering/renderer.hpp>
#include "entt/meta/meta.hpp"
#include "entt/core/hashed_string.hpp"
#include <imgui_internal.h>

using namespace clvr;

SceneManager::SceneManager()
{
    m_translateGizmo = std::make_unique<TranslateGizmo>();
}

void SceneManager::Update(float dt)
{
    return; // gizmos disabled for now 
	if (m_translateGizmo)
		m_translateGizmo->Update(Engine.GetECS()->GetSystem<Renderer>().GetActiveCamera());
}

void SceneManager::Render()
{
    return; // gizmos disabled for now
	if (m_translateGizmo)
		m_translateGizmo->Draw(Engine.GetECS()->GetSystem<Renderer>().GetActiveCamera());
}

void SceneManager::Draw()
{
    return; // gizmos disabled for now
	if (m_translateGizmo)
		m_translateGizmo->Draw(Engine.GetECS()->GetSystem<Renderer>().GetActiveCamera());
}

void SceneManager::Inspect(float dt)
{
    using namespace entt::literals;

    auto* ecs = Engine.GetECS();
	auto& renderer = ecs->GetSystem<Renderer>();

    ImGui::Begin("Outliner");

    if (ImGui::Button("Add Entity")) { ImGui::OpenPopup("CreateEntityPopup"); }

    if (ImGui::BeginPopup("CreateEntityPopup")) {
        if (ImGui::MenuItem("Create New Entity")) {
            Entity entity = ecs->CreateEntity();
            ecs->CreateComponent<Transform>(entity);
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (m_selectedEntity != entt::null && ImGui::Button("Delete Entity"))
    { 
        DeleteSelectedEntity();
    }

    ImGui::SameLine();
    ImGui::SeparatorText("Entities");

    ImGui::BeginChild("EntityList", ImVec2(0, ImGui::GetContentRegionAvail().y), ImGuiChildFlags_Borders);

    auto& registry = ecs->GetRegistry();
    auto view = ecs->GetRegistry().view<Transform>();

    for (auto entity : view)
    {
        auto& transform = view.get<Transform>(entity);
        std::string label = transform.name.empty()
            ? "Entity " + std::to_string(entt::to_integral(entity))
            : transform.name;

        ImGui::PushID(static_cast<int>(entt::to_integral(entity)));

        if (ImGui::Selectable(label.c_str(), m_selectedEntity == entity)) {
            UpdateSelectedEntity(entity);
        }

        if (m_scrollEntity != entt::null && m_scrollEntity == entity) {
			ImGui::SetScrollHereY();
			m_scrollEntity = entt::null; // reset scroll entity after scrolling
        }

        if (ImGui::BeginPopupContextItem("EntityContextMenu")) {
            UpdateSelectedEntity(entity); // right-click selects this row too

			ImGui::Text("Entity: %s", label.c_str());

			ImGui::Separator();

            if (ImGui::BeginMenu("Add Component")) {
				bool noComponentsToAdd = true;
                for (auto&& [id, type] : entt::resolve()) {
                    auto nameFunc = type.func("name"_hs);
                    auto hasFunc = type.func("has"_hs);
                    auto createFunc = type.func("create"_hs);

                    if (!nameFunc || !hasFunc || !createFunc)
                        continue; // skip meta types that aren't components (if you ever reflect anything else)

                    const char* name = nameFunc.invoke(entt::meta_handle{}).cast<const char*>();

                    bool has = hasFunc.invoke(entt::meta_handle{}, entt::forward_as_meta(registry),
                                                entt::forward_as_meta(entity)).cast<bool>();
                    
                    noComponentsToAdd = has && noComponentsToAdd;

                    if (!has && ImGui::MenuItem(name)) {
                        createFunc.invoke(entt::meta_handle{}, entt::forward_as_meta(registry), entt::forward_as_meta(entity));
                    }
                }
				if (noComponentsToAdd) {
					ImGui::Text("No components available to add.");
				}
                ImGui::EndMenu();
            }

			ImGui::Separator();

            if (ImGui::MenuItem("Delete")) {
                DeleteSelectedEntity();
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    ImGui::EndChild();

    ImGui::End();

    ImGui::Begin("Inspector");
    if (registry.valid(m_selectedEntity))
    {
        for (auto&& [id, storage] : registry.storage())
        {
            if (!storage.contains(m_selectedEntity))
                continue; // this pool doesn't have a component for the selected entity — skip it

            entt::meta_type meta_type = entt::resolve(id);
            if (!meta_type)
                continue; // component type isn't registered as inspectable

            using namespace entt::literals;
            entt::meta_any comp = meta_type.invoke("get"_hs, {}, entt::forward_as_meta(registry), m_selectedEntity);
            meta_type.invoke("inspect"_hs, comp);

			ImGui::PushID(static_cast<int>(entt::to_integral(id)));

            // Right-click context menu for deleting the Transform component
            if (ImGui::BeginPopupContextItem("ComponentContextMenu"))
            {
				if (ImGui::MenuItem("Delete Component"))
				{
                    auto removeFunc = meta_type.func("remove"_hs);
                    if (removeFunc)
                        removeFunc.invoke(entt::meta_handle{}, entt::forward_as_meta(registry), entt::forward_as_meta(m_selectedEntity));
				}
                ImGui::EndPopup();
            }

			ImGui::PopID();
        }
    }
    else
    {
		ImGui::Text("No entity selected.");
    }
    ImGui::End();
}

void clvr::SceneManager::DeleteSelectedEntity()
{
    if (m_selectedEntity == entt::null)
        return;

    Engine.GetECS()->DeleteEntity(m_selectedEntity);
    m_selectedEntity = entt::null;
    m_translateGizmo->SetSelectedEntity(entt::null);
}

void SceneManager::UpdateSelectedEntity(entt::entity entity)
{
    m_selectedEntity = entity;

    if (m_translateGizmo)
		m_translateGizmo->SetSelectedEntity(entity);
}