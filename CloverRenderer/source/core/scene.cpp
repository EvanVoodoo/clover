#include "core/scene.hpp"
#include <imgui.h>
#include <core/engine.hpp>
#include <core/transform.hpp>
#include <rendering/render_components.hpp>
#include <rendering/renderer.hpp>
#include "entt/meta/meta.hpp"
#include "core/transform.hpp"
#include "entt/core/hashed_string.hpp"

using namespace clvr;

SceneManager::SceneManager()
{
}

void SceneManager::Update(float dt)
{
}

void SceneManager::Render()
{
}

void SceneManager::Inspect(float dt)
{
	auto& renderer = Engine.GetECS()->GetSystem<Renderer>();

    static entt::entity selected = entt::null;

    ImGui::Begin("Entities");

	auto& registry = Engine.GetECS()->GetRegistry();
    auto view = Engine.GetECS()->GetRegistry().view<Transform>();
    for (auto entity : view)
    {
        auto& transform = view.get<Transform>(entity);
        std::string label = transform.name.empty()
            ? "Entity " + std::to_string(entt::to_integral(entity))
            : transform.name;

        if (ImGui::Selectable(label.c_str(), selected == entity))
            selected = entity;
    }
    ImGui::End();

    if (registry.valid(selected))
    {
        ImGui::Begin("Inspector");
        for (auto&& [id, storage] : registry.storage())
        {
            if (!storage.contains(selected))
                continue; // this pool doesn't have a component for the selected entity — skip it

            entt::meta_type meta_type = entt::resolve(id);
            if (!meta_type)
                continue; // component type isn't registered as inspectable

            using namespace entt::literals;
            entt::meta_any comp = meta_type.invoke("get"_hs, {}, entt::forward_as_meta(registry), selected);
            meta_type.invoke("inspect"_hs, comp);
        }
        ImGui::End();
    }
}