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
