#include "level/tile.hpp"
#include "nlohmann/json.hpp"
#include <fstream>
#include "resources/json_io.hpp"
#include <core/engine.hpp>
#include <rendering/renderer.hpp>

using json = nlohmann::json;

namespace clvr {
	bool TileSprites::Init(TextureAtlas& atlas, const std::string& absolutePath, int tilesetW, int tilesetH)
	{
		if (tilesetW <= 0 || tilesetH <= 0)
			return false;

		m_atlas = &atlas;
		m_path = absolutePath;
		m_pixelW = static_cast<float>(tilesetW);
		m_pixelH = static_cast<float>(tilesetH);

		return atlas.AddTexture(absolutePath) >= 0; // assumes a negative return means failure
	}

	void TileSprites::Refresh()
	{
		m_ready = false;
		if (!m_atlas)
			return;

		ID3D11ShaderResourceView* srv = m_atlas->GetSRV();
		if (!srv)
			return; // atlas not built yet

		const AtlasRegion r = m_atlas->GetRegion(m_path);
		if (r.uvRect.z <= 0.0f || r.uvRect.w <= 0.0f)
			return; // not found, or not built yet

		m_texture = reinterpret_cast<ImTextureID>(srv);
		m_u = r.uvRect.x;
		m_v = r.uvRect.y;
		m_uw = r.uvRect.z; // width in UV space
		m_vh = r.uvRect.w; // height in UV space
		m_ready = true;
	}

	SpriteRef TileSprites::Get(const TileDef& def, const TilesetInfo& info) const
	{
		SpriteRef ref;
		if (!m_ready || def.spriteCol < 0 || def.spriteRow < 0 ||
			info.cellSize <= 0 || info.padding * 2 >= info.cellSize)
			return ref;

		const float art = static_cast<float>(info.cellSize - 2 * info.padding);
		const float px = static_cast<float>(def.spriteCol * info.cellSize + info.padding);
		const float py = static_cast<float>(def.spriteRow * info.cellSize + info.padding);

		if (px + art > m_pixelW || py + art > m_pixelH)
			return ref; // cell lies outside the image

		// pixel inside the tileset -> fraction of the tileset -> UV inside the atlas
		ref.texture = m_texture;
		ref.uv0 = ImVec2(m_u + m_uw * (px / m_pixelW), m_v + m_vh * (py / m_pixelH));
		ref.uv1 = ImVec2(m_u + m_uw * ((px + art) / m_pixelW), m_v + m_vh * ((py + art) / m_pixelH));
		ref.valid = true;
		return ref;
	}

	SpriteRef TileSprites::WholeTileset() const
	{
		SpriteRef ref;
		if (!m_ready)
			return ref;

		ref.texture = m_texture;
		ref.uv0 = ImVec2(m_u, m_v);
		ref.uv1 = ImVec2(m_u + m_uw, m_v + m_vh);
		ref.valid = true;
		return ref;
	}

    TileDatabase::TileDatabase(const char* filepath) : Resource(ResourceType::TileDatabase)
    {
        m_filename = std::filesystem::path(filepath).filename().string();
        m_path = std::filesystem::path(filepath).string();
		LoadFromFile(filepath);

		auto& renderer = Engine.GetECS()->GetSystem<Renderer>();
		const std::string path = Engine.GetResourceManager()->GetPath(
			ResourceManager::Directory::Assets, Tileset().path);
		auto texture = Engine.GetResourceManager()->Load<Texture>(static_cast<ID3D11Device*>(renderer.GetNativeDeviceHandle()), path);
		m_sprites.Init(*renderer.GetTextureAtlas(), path, texture->GetWidth(), texture->GetHeight()); // your Texture's size getters
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
		json tileDB;
		tileDB["version"] = 2;
		tileDB["tileDefinitions"] = json::array();
			
		for (const auto& [id, def] : m_tileDefs) {
			tileDB["tileDefinitions"].push_back({
				{ "id",         id },
				{ "definition", def }
			});
		}

        tileDB["tileset"] = m_tileset;

		return tileDB;
	}

	bool TileDatabase::FromJson(const json& j)
	{
        if (!j.is_object())
            return false;

        TilesetInfo tileset = j.value("tileset", TilesetInfo{});

        const auto listIt = j.find("tileDefinitions");
        if (listIt == j.end() || !listIt->is_array())
            return false;

        // Parse into a temporary so a bad file can't leave you half-loaded
        std::map<TileID, TileDef> parsed;

        try
        {
            for (const auto& item : *listIt)
            {
                const TileID id = item.at("id").get<TileID>();
                if (id == 0)
                    return false; // 0 is reserved for "empty"

                if (parsed.contains(id))
                    return false; // duplicate ID

                parsed.emplace(id, item.at("definition").get<TileDef>());
            }
        }
        catch (const json::exception&)
        {
            return false; // missing key or wrong type
        }

        m_tileDefs = std::move(parsed);
        return true;
	}

	// Same formula the room editor uses for visual layers, so a tile has one color in both.
	ImU32 TileColor(unsigned id, int alpha)
	{
		return IM_COL32((id * 37) % 200 + 40, (id * 59) % 200 + 40, (id * 83) % 200 + 40, alpha);
	}
	void DrawTileImage(ImDrawList* dl, ImVec2 a, ImVec2 b, const TileDef* def, const TileSprites& sprites, const TilesetInfo& info, TileID id, int alpha)
	{
		if (def && def->spriteCol >= 0)
		{
			const SpriteRef s = sprites.Get(*def, info);
			if (s.valid)
				dl->AddImage(s.texture, a, b, s.uv0, s.uv1, IM_COL32(255, 255, 255, alpha));
			else
				dl->AddRectFilled(a, b, IM_COL32(255, 0, 255, alpha)); // magenta: sprite assigned but unresolvable
		}
		else
			dl->AddRectFilled(a, b, TileColor(id, alpha)); // no sprite yet: the old ID color
	}
}
