#include "level/tile.hpp"
#include "nlohmann/json.hpp"
#include <fstream>
#include "resources/json_io.hpp"
#include <core/engine.hpp>
#include <rendering/renderer.hpp>
#include <iostream>

using json = nlohmann::json;

namespace clvr {
	bool TileSprites::Init(TextureAtlas& atlas, ResourceManager::Directory dir, const std::string& name)
	{
		m_atlas = &atlas;
		m_dir = dir;
		m_name = name;
		return !name.empty() && atlas.AddTexture(dir, name) >= 0;
	}

	void TileSprites::Refresh()
	{
		m_ready = false;
		if (!m_atlas) return;

		ID3D11ShaderResourceView* srv = m_atlas->GetSRV();
		if (!srv) return;                       // atlas not built yet

		AtlasRegion r;
		if (!m_atlas->TryGetRegion(m_dir, m_name, r) || r.pixelW <= 0 || r.pixelH <= 0)
			return;

		m_texture = reinterpret_cast<ImTextureID>(srv);
		m_u = r.uvRect.x;  m_v = r.uvRect.y;
		m_uw = r.uvRect.z; m_vh = r.uvRect.w;
		m_pixelW = static_cast<float>(r.pixelW);
		m_pixelH = static_cast<float>(r.pixelH);
		m_ready = true;
	}

	SpriteRef TileSprites::Get(const TileDef& def, const TilesetInfo& info) const
	{
		SpriteRef ref;

		XMFLOAT4 sub;
		if (!GetSubRect(def, info, sub))   // handles m_ready, bad indices, bad cell size, out-of-bounds
			return ref;

		// sub-rect inside the tileset image -> UV inside the atlas region
		ref.texture = m_texture;
		ref.name = m_name;
		ref.uv0 = ImVec2(m_u + m_uw * sub.x, m_v + m_vh * sub.y);
		ref.uv1 = ImVec2(m_u + m_uw * (sub.x + sub.z), m_v + m_vh * (sub.y + sub.w));
		ref.valid = true;
		return ref;
	}

	bool TileSprites::GetSubRect(const TileDef& def, const TilesetInfo& info, XMFLOAT4& out) const
	{
		if (!m_ready || def.spriteCol < 0 || def.spriteRow < 0 ||
			info.tileSize <= 0 || info.spacing < 0 || info.margin < 0)
			return false;

		const float art = float(info.tileSize);
		const float px = float(info.margin + def.spriteCol * info.CellSize());
		const float py = float(info.margin + def.spriteRow * info.CellSize());
		if (px + art > m_pixelW || py + art > m_pixelH)
			return false;

		out = { px / m_pixelW, py / m_pixelH, art / m_pixelW, art / m_pixelH };
		return true;
	}

	SpriteRef TileSprites::WholeTileset() const
	{
		SpriteRef ref;
		if (!m_ready)
			return ref;

		ref.texture = m_texture;
		ref.name = m_name;
		ref.uv0 = ImVec2(m_u, m_v);
		ref.uv1 = ImVec2(m_u + m_uw, m_v + m_vh);
		ref.valid = true;
		return ref;
	}

	namespace
	{
		constexpr int kTileDbVersion = 3;
	}

    TileDatabase::TileDatabase(const char* filepath) : Resource(ResourceType::TileDatabase)
    {
        m_filename = std::filesystem::path(filepath).filename().string();
        m_path = std::filesystem::path(filepath).string();
		LoadFromFile(filepath);

		if (!LoadFromFile(filepath))
			std::cerr << "TileDatabase: could not load " << filepath << ", starting empty\n";

		if (m_tilesets.empty())
		{
			// Missing or unreadable file: start with one default tileset so the editors have something to show
			m_tilesets[0].info = TilesetInfo{};
			InitSprites();
		}
    }

	void TileDatabase::InitSprites()
	{
		auto& renderer = Engine.GetECS()->GetSystem<Renderer>();
		for (auto& [id, entry] : m_tilesets)
		{
			if (!entry.sprites.Init(*renderer.GetTextureAtlas(), ResourceManager::Directory::Assets, entry.info.path))
				std::cerr << "TileDatabase: tileset " << id << " image failed to load: " << entry.info.path << '\n';
		}
	}

	const TilesetEntry* TileDatabase::GetTileset(TilesetID id) const
	{
		auto it = m_tilesets.find(id);
		if (it == m_tilesets.end())
			return nullptr;
		return &it->second;
	}

	SpriteRef TileDatabase::GetSprite(const TileDef& def) const
	{
		// pick the right tileset internally
		const TilesetEntry* entry = GetTileset(def.tileset);
		return entry ? entry->sprites.Get(def, entry->info) : SpriteRef{};
	}

	bool TileDatabase::GetSubRect(const TileDef& def, XMFLOAT4& out) const
	{
		const TilesetEntry* entry = GetTileset(def.tileset);
		return entry ? entry->sprites.GetSubRect(def, entry->info, out) : false;
	}

	const std::string& TileDatabase::TilesetPath(const TileDef& def) const
	{
		static const std::string kEmpty;
		const TilesetEntry* entry = GetTileset(def.tileset);
		return entry ? entry->info.path : kEmpty;
	}

	void TileDatabase::RefreshSprites()
	{
		// call refresh on every tileset, in case the atlas was rebuilt
		for (auto& [id, entry] : m_tilesets)
			entry.sprites.Refresh();
	}

	TilesetID TileDatabase::NextFreeTilesetID() const
{
	return m_tilesets.empty() ? TilesetID(0) : static_cast<TilesetID>(m_tilesets.rbegin()->first + 1);
}

bool TileDatabase::TilesetInUse(TilesetID id) const
{
	return std::any_of(m_tileDefs.begin(), m_tileDefs.end(),
	                   [id](const auto& kv) { return kv.second.tileset == id; });
}

bool TileDatabase::RegisterTileset(TilesetID id, TilesetInfo info)
{
	if (info.path.empty() || info.tileSize <= 0 || info.spacing < 0 || info.margin < 0)
		return false;

	auto& entry = m_tilesets[id];
	entry.info = std::move(info);
	entry.sprites = TileSprites{};

	auto& renderer = Engine.GetECS()->GetSystem<Renderer>();
	entry.sprites.Init(*renderer.GetTextureAtlas(), ResourceManager::Directory::Assets, entry.info.path);
	// The atlas must be rebuilt before Refresh() can find the new region.
	// Call whatever you use for that here (the Build() call from the backdrop setup).
	return true;
}

bool TileDatabase::UnregisterTileset(TilesetID id)
{
	if (m_tilesets.size() <= 1 || TilesetInUse(id))
		return false;
	return m_tilesets.erase(id) > 0;
}

    bool TileDatabase::LoadFromFile(const std::filesystem::path& path)
    {
        std::ifstream file(path);
        if (!file)
            return false;

        const json j = json::parse(file, nullptr, /*allow_exceptions=*/false);
        if (j.is_discarded())
            return false;

        return FromJson(j);
    }

    bool TileDatabase::SaveToFile() const
    {
        return WriteJsonFile(m_path, ToJson());
    }

    json TileDatabase::ToJson() const {
		json db;
		db["version"] = kTileDbVersion;

		db["tilesets"] = json::array();
		for (const auto& [id, entry] : m_tilesets)
			db["tilesets"].push_back({ { "id", id }, { "info", entry.info } });

		db["tileDefinitions"] = json::array();
		for (const auto& [id, def] : m_tileDefs)
			db["tileDefinitions"].push_back({ { "id", id }, { "definition", def } });

		return db;
	}

	bool TileDatabase::FromJson(const json& j)
	{
		if (!j.is_object())
			return false;

		// Don't load a file written by a newer build: saving it again would drop fields
		if (j.value("version", 1) > kTileDbVersion)
			return false;

		// Parse into temporaries so a bad file can't leave us half-loaded
		std::map<TilesetID, TilesetEntry> tilesets;
		std::map<TileID, TileDef> tileDefs;

		try
		{
			if (const auto it = j.find("tilesets"); it != j.end() && it->is_array())
			{
				for (const auto& item : *it)
				{
					const TilesetID id = item.at("id").get<TilesetID>();
					if (tilesets.contains(id))
						return false; // duplicate tileset id
					tilesets[id].info = item.at("info").get<TilesetInfo>();
				}
			}
			else
			{
				// Old format (version 2 and earlier): one tileset, which becomes tileset 0
				tilesets[0].info = j.value("tileset", TilesetInfo{});
			}

			for (const auto& [id, entry] : tilesets)
			{
				const TilesetInfo& info = entry.info;
				if (info.path.empty() || info.tileSize <= 0 || info.spacing < 0 || info.margin < 0)
					return false; // unusable tileset settings
			}

			const auto listIt = j.find("tileDefinitions");
			if (listIt == j.end() || !listIt->is_array())
				return false;

			for (const auto& item : *listIt)
			{
				const TileID id = item.at("id").get<TileID>();
				if (id == 0 || tileDefs.contains(id))
					return false; // 0 is reserved for "empty", and ids must be unique

				TileDef def = item.at("definition").get<TileDef>();
				if (!tilesets.contains(def.tileset))
					return false; // tile points at a tileset that isn't defined

				tileDefs.emplace(id, std::move(def));
			}
		}
		catch (const json::exception&)
		{
			return false; // missing key or wrong type
		}

		m_tilesets = std::move(tilesets);
		m_tileDefs = std::move(tileDefs);
		InitSprites();
		return true;
	}

	// Same formula the room editor uses for visual layers, so a tile has one color in both.
	ImU32 TileColor(unsigned id, int alpha)
	{
		return IM_COL32((id * 37) % 200 + 40, (id * 59) % 200 + 40, (id * 83) % 200 + 40, alpha);
	}

	void DrawTileImage(ImDrawList* dl, ImVec2 a, ImVec2 b, const TileDef* def, const TileDatabase& db, TileID id, int alpha)
	{
		if (def && def->spriteCol >= 0)
		{
			const SpriteRef s = db.GetSprite(*def);
			if (s.valid)
				dl->AddImage(s.texture, a, b, s.uv0, s.uv1, IM_COL32(255, 255, 255, alpha));
			else
				dl->AddRectFilled(a, b, IM_COL32(255, 0, 255, alpha)); // magenta: sprite assigned but unresolvable
		}
		else
			dl->AddRectFilled(a, b, TileColor(id, alpha)); // no sprite yet: the old ID color
	}
}
