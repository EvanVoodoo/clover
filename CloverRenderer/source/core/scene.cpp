#include "core/scene.hpp"
#include <imgui.h>
#include <core/engine.hpp>
#include <core/transform.hpp>
#include <rendering/render_components.hpp>
#include <rendering/renderer.hpp>

using namespace clvr;

void SceneManager::Update(float dt)
{
}

void SceneManager::Render()
{
}

void SceneManager::Inspect(float dt)
{
	auto& renderer = Engine.GetECS()->GetSystem<Renderer>();

	ImGui::Begin("Inspector");

	ImGui::Text("Frame time: %.3f s", dt);

	// example: iterate lights and show their properties
	auto view = Engine.GetECS()->GetRegistry().view<Transform, Light>();
	int i = 0;
	for (auto [entity, transform, light] : view.each())
	{
		ImGui::PushID(i++);
		std::string label = "";
		if (light.type == 0.f) {
			label = "Directional Light";
			if (ImGui::TreeNode(label.c_str()))
			{
				float angle = atan2f(light.direction.y, light.direction.x);

				if (ImGui::SliderAngle("Direction", &angle, -180.0f, 180.0f))
				{
					light.direction.x = cosf(angle);
					light.direction.y = sinf(angle);
				}
				ImGui::ColorEdit3("Color", &light.color.x);
				ImGui::DragFloat("Intensity", &light.intensity, 0.01f, 0.0f, 10.0f);
				ImGui::TreePop();
			}
		}
		else if (light.type == 1.f) {
			label = "Point Light";
			if (ImGui::TreeNode(label.c_str()))
			{
				if (ImGui::DragFloat2("Position", &transform.position.x)) {
					light.direction.x = transform.position.x;
					light.direction.y = transform.position.y;
				}
				ImGui::ColorEdit3("Color", &light.color.x);
				ImGui::DragFloat("Intensity", &light.intensity);
				ImGui::TreePop();
			}
		}

		ImGui::PopID();
	}

	auto spriteView = Engine.GetECS()->GetRegistry().view<Transform, SpriteComponent>();
	for (auto [entity, transform, sprite] : spriteView.each())
	{
		ImGui::PushID(i++);
		std::string label = "Sprite " + std::to_string(i);
		if (ImGui::TreeNode(label.c_str()))
		{
			if (ImGui::DragFloat2("Position", &transform.position.x)) {
				sprite.sprite.position.x = transform.position.x;
				sprite.sprite.position.y = transform.position.y;
			}
			ImGui::DragFloat2("Size", &sprite.sprite.size.x, 1.0f, 0.0f);
			ImGui::DragFloat2("Scale", &transform.scale.x);
			if (ImGui::SliderAngle("Rotation", &transform.rotation, -180.0f, 180.0f)) {
				// rotation is in radians
			}
			ImGui::ColorEdit4("Color", &sprite.sprite.color.x);

			const auto& layers = renderer.GetSpriteLayers();
			SpriteLayer* currentLayer = renderer.FindSpriteLayer(sprite.sprite.layer->id);
			std::string currentLayerName = currentLayer ? currentLayer->layerName : "None";

			if (ImGui::BeginCombo("Layer", currentLayerName.c_str()))
			{
				for (SpriteLayer* layer : layers)
				{
					bool isSelected = (layer == currentLayer);
					if (ImGui::Selectable(layer->layerName.c_str(), isSelected))
						sprite.sprite.layer = layer;

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	ImGui::End();
}