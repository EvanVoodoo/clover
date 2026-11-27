#pragma once
#include <string>
#include <map>
#include <memory>
#include <cassert>

#include "nlohmann/json.hpp"
#include "resources/resource.hpp"
#include "common.hpp"
#include <core/editor_window.hpp>
#include <rendering/texture_atlas.hpp>
#include <rendering/texture.hpp>
#include "imgui/imgui.h"
#include <cstdint>

namespace clvr
{
	constexpr int TILE_SIZE = 16;

	class Tile
	{
	public:
		Tile() = default;

		TileID GetId() const { return m_id; }
		int GetX() const { return m_x; }
		int GetY() const { return m_y; }

		void SetId(TileID id) { m_id = id; }
	private:
		TileID m_id = 0;
		int m_x = 0;
		int m_y = 0;
	};

	using TilesetID = std::uint16_t;

	struct TilesetInfo
	{
		std::string name = "Tileset";
		std::string path = "tiles/tileset.png"; // relative to Assets
		int tileSize = TILE_SIZE;
		int spacing = 0;            // pixels between neighboring tiles
		int margin = 0;             // pixels from the image edge to the first tile
		int CellSize() const { return tileSize + spacing; }
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TilesetInfo, name, path, tileSize, spacing, margin)

	struct TileDef {
		std::string name; 
		TilesetID tileset = 0;
		int spriteCol = -1;   // -1 = no sprite assigned
		int spriteRow = -1;
		bool blocksMovement = false;
		bool blocksProjectiles = false;
		bool destructible = false;
		int contactDamage = 0;
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TileDef, name, tileset, spriteCol, spriteRow, blocksMovement, blocksProjectiles, destructible, contactDamage)

	struct SpriteRef
	{
		ImTextureID texture = 0;
		std::string name;
		ImVec2 uv0{ 0, 0 }, uv1{ 1, 1 };
		bool valid = false;
	};

	class TileSprites
	{
	public:
		// Call once, before the atlas is built. tilesetW/H are the tileset image's size in pixels.
		bool Init(TextureAtlas& atlas, ResourceManager::Directory dir, const std::string& name);

		// Call once per frame (cheap). Re-reads the SRV and region in case the atlas was rebuilt.
		void Refresh();

		bool Ready() const { return m_ready; }

		SpriteRef Get(const TileDef& def, const TilesetInfo& info) const;
		bool GetSubRect(const TileDef& def, const TilesetInfo& info, XMFLOAT4& out) const;
		SpriteRef WholeTileset() const;

		float TilesetW() const { return m_pixelW; }
		float TilesetH() const { return m_pixelH; }

	private:
		TextureAtlas* m_atlas = nullptr; // non-owning
		std::string m_name = "";
		ResourceManager::Directory m_dir = ResourceManager::Directory::SharedAssets;
		float m_pixelW = 0, m_pixelH = 0;

		ImTextureID m_texture = 0;
		float m_u = 0, m_v = 0, m_uw = 0, m_vh = 0; // region in UV space
		bool m_ready = false;
	};

	struct TilesetEntry
	{
		TilesetInfo info;
		TileSprites sprites;
	};

	class TileDatabase : public Resource
	{
	public:	
		TileDatabase(const char* filepath);
		~TileDatabase() = default;

		void InitSprites();
		const TilesetEntry* GetTileset(TilesetID id) const;
		SpriteRef GetSprite(const TileDef& def) const;                 // picks the right tileset internally
		bool GetSubRect(const TileDef& def, XMFLOAT4& out) const;
		const std::string& TilesetPath(const TileDef& def) const;
		void RefreshSprites();                                          // refreshes every tileset

		const std::map<TilesetID, TilesetEntry>& Tilesets() const { return m_tilesets; }
		TilesetID NextFreeTilesetID() const;
		bool TilesetInUse(TilesetID id) const;
		bool RegisterTileset(TilesetID id, TilesetInfo info);   // add or replace
		bool UnregisterTileset(TilesetID id);

		void Register(TileID id, TileDef def) 
		{ 
			assert(id != 0);
			m_tileDefs[id] = def; 
		}
		void Unregister(TileID id) { m_tileDefs.erase(id); }

		const TileDef* Get(TileID id) const { 
			auto it = m_tileDefs.find(id);
			return it != m_tileDefs.end() ? &it->second : nullptr;
		}

		const std::map<TileID, TileDef>& All() const { return m_tileDefs; }

		// Highest ID + 1 (0 is reserved), or 0 if the ID space is full.
		TileID NextFreeID() const
		{
			if (m_tileDefs.empty())
				return 1;
			const TileID last = m_tileDefs.rbegin()->first;
			return last == (std::numeric_limits<TileID>::max)() ? 0 : static_cast<TileID>(last + 1);
		}

		static std::string GetPath(const char* filepath) {
			std::string path = "tile_database:" + std::filesystem::path(filepath).string();
			return path;
		}

		bool LoadFromFile(const std::filesystem::path& path);

		bool SaveToFile() const;

		using json = nlohmann::json;

		json ToJson() const;
		bool FromJson(const json& j);

	private:
		std::map<TileID, TileDef> m_tileDefs;
		std::map<TilesetID, TilesetEntry> m_tilesets;
	};

	// Same formula the room editor uses for visual layers, so a tile has one color in both.
	ImU32 TileColor(unsigned id, int alpha);

	void DrawTileImage(ImDrawList* dl, ImVec2 a, ImVec2 b, const TileDef* def, const TileDatabase& db, TileID id, int alpha);
}