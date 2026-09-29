#include "rendering/render_components.hpp"
#include <core/engine.hpp>
#include <rendering/renderer.hpp>

using namespace clvr;

using Dir = ResourceManager::Directory;

bool Sprite::LoadSpriteTexture(const std::string& filename) {
	std::shared_ptr<Texture> tex = Engine.GetECS()->GetSystem<Renderer>().LoadTexture(filename);
	return LoadSpriteTexture(tex);
}

bool Sprite::LoadSpriteTexture(const std::shared_ptr<Texture> texture)
{
	if (texture) {
		this->texture = texture;
		useLinkedTexture = true;
	}
	return useLinkedTexture;
}

void SpriteComponent::Inspect()
{
	if (ImGui::TreeNode("Sprite"))
	{
		ImGui::DragFloat2("Size", &sprite.size.x, 0.5f);
		ImGui::ColorEdit4("Color", &sprite.color.x);
		ImGui::Checkbox("Is Occluder", &sprite.isOccluder);
		// Combo for selecting a sprite layer from the available layers in the renderer
		auto& renderer = Engine.GetECS()->GetSystem<Renderer>();
		const char* layerNames[100];
		int layerCount = 0;
		for (const auto layer : renderer.GetSpriteLayers()) {
			layerNames[layerCount++] = layer->layerName.c_str();
		}
		if (layerCount == 0) {
			ImGui::Text("No layers available");
		} 
		else {
			// Create a combo box with the available layer names
			// the currentLayer variable is used to keep track of the selected layer index
			int currentLayer = static_cast<int>(std::find(renderer.GetSpriteLayers().begin(), renderer.GetSpriteLayers().end(), sprite.layer) - renderer.GetSpriteLayers().begin());
			if (ImGui::Combo("Layer", &currentLayer, layerNames, static_cast<int>(renderer.GetSpriteLayers().size()))) {
				sprite.layer = renderer.GetSpriteLayers()[currentLayer];
			}
		}

		if (sprite.layer)
			ImGui::Text("Layer: %s", sprite.layer->layerName.c_str());
		else
			ImGui::Text("Layer: None");

		if (sprite.texture)
		{
			ImGui::Text("Texture: %s", std::string(sprite.textureName).c_str());

			// Fixed-size thumbnail, aspect-corrected so square textures don't get stretched into a square box
			// when they aren't actually square.
			const float thumbnailWidth = 128.0f;
			float aspect = static_cast<float>(sprite.texture->GetHeight()) / static_cast<float>(sprite.texture->GetWidth());
			ImVec2 thumbnailSize(thumbnailWidth, thumbnailWidth * aspect);

			ImGui::Image((ImTextureID) sprite.texture->GetSRV(), thumbnailSize);
		}
		else
		{
			ImGui::Text("Texture: %s (Atlas)", std::string(sprite.textureName).c_str());

			// uvRect stores (u0, v0, u1, v1) — ask ImGui to only draw that slice of the atlas.
			ImVec2 uv0(sprite.uvRect.x, sprite.uvRect.y);
			ImVec2 uv1(uv0.x + sprite.uvRect.z, uv0.y + sprite.uvRect.w);

			const float thumbnailWidth = 128.0f;
			float cellAspect = (uv1.y - uv0.y) / (uv1.x - uv0.x); // aspect of just this cell, not the whole atlas
			ImVec2 thumbnailSize(thumbnailWidth, thumbnailWidth * cellAspect);

			auto& renderer = Engine.GetECS()->GetSystem<Renderer>();

			ImGui::Image((ImTextureID) renderer.GetTextureAtlas()->GetSRV(), thumbnailSize, uv0, uv1);
		}
		
		if (ImGui::TreeNode("More Details"))
		{
			ImGui::Text("Pivot: (%.2f, %.2f)", sprite.pivot.x, sprite.pivot.y);
			ImGui::Text("Rotation: %.2f", sprite.rotation);
			ImGui::Text("UV Rect: (%.2f, %.2f, %.2f, %.2f)", sprite.uvRect.x, sprite.uvRect.y, sprite.uvRect.z, sprite.uvRect.w);
			ImGui::TreePop();
		}
		ImGui::TreePop();
	}
}

void Light::Inspect()
{
	if (ImGui::TreeNode("Light"))
	{
		if (type == 0.0f) {
			ImGui::Text("Type: Directional");
			ImGui::DragFloat3("Direction", &direction.x, 0.1f);
		}
		else if (type == 1.0f) {
			ImGui::Text("Type: Point");
		}
		else if (type == 2.0f) {
			ImGui::Text("Type: Spotlight");
		}
		else {
			ImGui::Text("Type: Unknown");
		}
		ImGui::ColorEdit3("Color", &color.x);
		ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f);
		static const char* lightTypeNames[] = { "Directional", "Point", "Spot" };
		int typeIndex = static_cast<int>(type);
		if (ImGui::Combo("Type", &typeIndex, lightTypeNames, IM_ARRAYSIZE(lightTypeNames)))
			type = static_cast<float>(typeIndex);

		ImGui::TreePop();
	}
}