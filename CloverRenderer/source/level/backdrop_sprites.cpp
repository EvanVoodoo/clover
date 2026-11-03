#include "level/backdrop_sprites.hpp"
#include "level/backdrop.hpp"

namespace clvr
{
	void BackdropSprites::Init(TextureAtlas& atlas, const BackdropDatabase& db, const PathResolver& resolve)
	{
		m_atlas = &atlas;
		m_entries.clear();

		for (const auto& [id, def] : db.All())
		{
			Entry e;
			e.path = resolve(def.image);
			if (atlas.AddTexture(e.path) >= 0) // assumes a negative return means failure
				m_entries.emplace(id, std::move(e));
		}
	}

	void BackdropSprites::Refresh()
	{
		m_texture = 0;
		if (!m_atlas)
			return;

		ID3D11ShaderResourceView* srv = m_atlas->GetSRV();
		if (!srv)
		{
			for (auto& [id, e] : m_entries)
				e.ready = false;
			return; // atlas not built yet
		}
		m_texture = reinterpret_cast<ImTextureID>(srv);

		for (auto& [id, e] : m_entries)
		{
			const AtlasRegion r = m_atlas->GetRegion(e.path);
			e.ready = r.uvRect.z > 0.0f && r.uvRect.w > 0.0f;
			if (e.ready)
			{
				e.uv0 = ImVec2(r.uvRect.x, r.uvRect.y);
				e.uv1 = ImVec2(r.uvRect.x + r.uvRect.z, r.uvRect.y + r.uvRect.w);
			}
		}
	}

	SpriteRef BackdropSprites::Get(BackdropID id) const
	{
		SpriteRef ref;
		const auto it = m_entries.find(id);
		if (it == m_entries.end() || !it->second.ready || !m_texture)
			return ref;

		ref.texture = m_texture;
		ref.uv0 = it->second.uv0;
		ref.uv1 = it->second.uv1;
		ref.valid = true;
		return ref;
	}
}