#include "rendering/render_components.hpp"
#include <core/engine.hpp>
#include <rendering/texture.hpp>

using namespace clvr;

using Dir = ResourceManager::Directory;

bool Sprite::LoadSpriteTexture(const std::string& filename) {
	ResourceManager* rMngr = Engine.GetResourceManager();

	std::shared_ptr<Texture> tex = rMngr->Load<Texture>(Dir::SharedAssets, filename);
	if (tex) {
		this->texture = tex->GetSRV();
		useLinkedTexture = true;
	}
	return useLinkedTexture;
}
