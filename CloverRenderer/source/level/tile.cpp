#include "level/tile.hpp"
#include "nlohmann/json.hpp"
#include <fstream>

using json = nlohmann::json;

namespace clvr {
    bool TileDatabase::LoadTileDatabase(const std::filesystem::path& path, TileDatabase& db)
    {
        std::ifstream file(path);
        if (!file)
            return false;

        const json j = json::parse(file, nullptr, /*allow_exceptions=*/false);
        if (j.is_discarded())
            return false; // not valid JSON at all

        return db.FromJson(j);
    }
    json TileDatabase::ToJson() const {
		json tileDB;
		tileDB["version"] = 1;
		tileDB["tileDefinitions"] = json::array();
			
		for (const auto& [id, def] : m_tileDefs) {
			tileDB["tileDefinitions"].push_back({
				{ "id",         id },
				{ "definition", def }
			});
		}

		return tileDB;
	}

	bool TileDatabase::FromJson(const json& j)
	{
        if (!j.is_object())
            return false;

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
}
