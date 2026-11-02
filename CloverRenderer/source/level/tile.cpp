#include "level/tile.hpp"
#include "nlohmann/json.hpp"
#include <fstream>
#include "resources/json_io.hpp"

using json = nlohmann::json;

namespace clvr {
    TileDatabase::TileDatabase(const char* filepath) : Resource(ResourceType::TileDatabase)
    {
        m_filename = std::filesystem::path(filepath).filename().string();
        m_path = std::filesystem::path(filepath).string();
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
