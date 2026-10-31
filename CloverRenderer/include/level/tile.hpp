#pragma once
#include <cstdint>
#include <string>
#include "nlohmann/json.hpp"
#include <map>

namespace clvr
{
	using TileID = std::uint16_t;

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

	struct TileDef {
		std::string name;
		std::string spriteName;
		bool blocksMovement = false;
		bool blocksProjectiles = false;
		bool destructible = false;
		int contactDamage = 0;
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TileDef, name, spriteName, blocksMovement, blocksProjectiles, destructible, contactDamage)

	class TileDatabase
	{
	public:	
		TileDatabase() = default;

		void Register(TileID id, TileDef def) { m_tileDefs[id] = def; }

		const TileDef* Get(TileID id) const { 
			auto it = m_tileDefs.find(id);
			return it != m_tileDefs.end() ? &it->second : nullptr;
		}

		bool LoadTileDatabase(const std::filesystem::path& path, TileDatabase& db);

		using json = nlohmann::json;

		json ToJson() const;
		bool FromJson(const json& j);

	private:
		std::map<TileID, TileDef> m_tileDefs;
	};
}