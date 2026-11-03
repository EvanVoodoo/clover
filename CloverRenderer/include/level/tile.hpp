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

	struct TilesetInfo
	{
		std::string path = "tiles/tileset.png"; // relative to Assets, not the absolute path from GetPath
		int padding = 1;                        // border on each side of the art
		int cellSize = TILE_SIZE + padding * 2; // full cell, including padding
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TilesetInfo, path, cellSize, padding)

	struct TileDef {
		std::string name; 
		int spriteCol = -1;   // -1 = no sprite assigned
		int spriteRow = -1;
		bool blocksMovement = false;
		bool blocksProjectiles = false;
		bool destructible = false;
		int contactDamage = 0;
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TileDef, name, spriteCol, spriteRow, blocksMovement, blocksProjectiles, destructible, contactDamage)

	struct SpriteRef
	{
		ImTextureID texture = 0;
		ImVec2 uv0{ 0, 0 }, uv1{ 1, 1 };
		bool valid = false;
	};

	class TileSprites
	{
	public:
		// Call once, before the atlas is built. tilesetW/H are the tileset image's size in pixels.
		bool Init(TextureAtlas& atlas, const std::string& absolutePath, int tilesetW, int tilesetH);

		// Call once per frame (cheap). Re-reads the SRV and region in case the atlas was rebuilt.
		void Refresh();

		bool Ready() const { return m_ready; }

		SpriteRef Get(const TileDef& def, const TilesetInfo& info) const;
		SpriteRef WholeTileset() const;

		float TilesetW() const { return m_pixelW; }
		float TilesetH() const { return m_pixelH; }

	private:
		TextureAtlas* m_atlas = nullptr; // non-owning
		std::string m_path;
		float m_pixelW = 0, m_pixelH = 0;

		ImTextureID m_texture = 0;
		float m_u = 0, m_v = 0, m_uw = 0, m_vh = 0; // region in UV space
		bool m_ready = false;
	};

	class TileDatabase : public Resource
	{
	public:	
		TileDatabase(const char* filepath);
		~TileDatabase() = default;

		TilesetInfo& Tileset() { return m_tileset; }
		TileSprites& Sprites() { return m_sprites; }

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

		int GetTileSetWidth() const { return m_tilesetTexture ? m_tilesetTexture->GetWidth() : 0; }
		int GetTileSetHeight() const { return m_tilesetTexture ? m_tilesetTexture->GetHeight() : 0; }

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
		std::shared_ptr<Texture> m_tilesetTexture;
		TilesetInfo m_tileset;
		TileSprites m_sprites;
	};

	// Same formula the room editor uses for visual layers, so a tile has one color in both.
	ImU32 TileColor(unsigned id, int alpha);

	void DrawTileImage(ImDrawList* dl, ImVec2 a, ImVec2 b, const TileDef* def,
					   const TileSprites& sprites, const TilesetInfo& info, TileID id, int alpha);
}