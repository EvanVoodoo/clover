#include "core/scene.hpp"
#include <imgui.h>
#include <core/engine.hpp>
#include <core/transform.hpp>
#include <rendering/render_components.hpp>

using namespace clvr;

void SceneManager::Update(float dt)
{
}

void SceneManager::Render()
{
}

void SceneManager::Inspect(float dt)
{
	ImGui::Begin("Scene Manager");

	// list every entity with a transform component, essentially listing our actors in the scene
	auto ecs = Engine.GetECS();

	auto sceneView = ecs->GetRegistry().view<Transform>();

	for (auto [entity, transform] : sceneView.each())
	{
		ImGui::PushID(static_cast<int>(entity));
		if (transform.name.empty())
		{
			ImGui::Text("unnamed");
		}
		else
		{
			ImGui::Text(transform.name.c_str());
		}
		ImGui::Text("Entity: %d", static_cast<int>(entity));
		ImGui::Separator();
		ImGui::PopID();
	}

	ImGui::End();
}