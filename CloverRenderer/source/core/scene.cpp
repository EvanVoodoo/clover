#include "core/scene.hpp"
#include <imgui.h>
#include <core/engine.hpp>
#include <core/transform.hpp>
#include <rendering/renderer.hpp>
#include "entt/meta/meta.hpp"
#include "entt/core/hashed_string.hpp"

using namespace clvr;

SceneManager::SceneManager()
{
    m_translateGizmo = std::make_unique<TranslateGizmo>();
}

void SceneManager::Update(float dt)
{
	if (m_translateGizmo)
		m_translateGizmo->Update(Engine.GetECS()->GetSystem<Renderer>().GetActiveCamera());
}

void SceneManager::Render()
{
	/*if (m_translateGizmo)
		m_translateGizmo->Draw(Engine.GetECS()->GetSystem<Renderer>().GetActiveCamera());*/
}

void SceneManager::Draw()
{
	if (m_translateGizmo)
		m_translateGizmo->Draw(Engine.GetECS()->GetSystem<Renderer>().GetActiveCamera());
}

void SceneManager::Inspect(float dt)
{
	auto& renderer = Engine.GetECS()->GetSystem<Renderer>();

    ImGui::Begin("Entities");

	auto& registry = Engine.GetECS()->GetRegistry();
    auto view = Engine.GetECS()->GetRegistry().view<Transform>();
    for (auto entity : view)
    {
        auto& transform = view.get<Transform>(entity);
        std::string label = transform.name.empty()
            ? "Entity " + std::to_string(entt::to_integral(entity))
            : transform.name;

        if (ImGui::Selectable(label.c_str(), m_selectedEntity == entity)) {
            UpdateSelectedEntity(entity);
        }
    }
    ImGui::End();

    if (registry.valid(m_selectedEntity))
    {
        ImGui::Begin("Inspector");
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
        }
        ImGui::End();
    }
}

void SceneManager::UpdateSelectedEntity(entt::entity entity)
{
    m_selectedEntity = entity;
    if (m_translateGizmo)
		m_translateGizmo->SetSelectedEntity(entity);
}