#include "level/room_serialization.hpp"
#include "level/room.hpp"
#include "level/tile.hpp"
#include <limits>
#include <nlohmann/json.hpp>
#include <fstream>
#include <resources/json_io.hpp>

namespace clvr
{
	using json = nlohmann::json;

	namespace
	{
		constexpr int kRoomFormatVersion = 2;
		constexpr int kMaxRoomDimension = 64; // sanity limit against corrupt files

		template <typename T>
		json GridToJson(const Grid<T>& g)
		{
			json rows = json::array();
			for (int y = 0; y < g.height; ++y)
			{
				json row = json::array();
				for (int x = 0; x < g.width; ++x)
					row.push_back(g.At(x, y));
				rows.push_back(std::move(row));
			}
			return rows;
		}

		template <typename T>
		bool GridFromJson(const json& j, int w, int h, const char* name,
						  Grid<T>& out, std::string& error)
		{
			if (!j.is_array() || static_cast<int>(j.size()) != h)
			{
				error = std::string(name) + ": expected " + std::to_string(h) + " rows";
				return false;
			}

			Grid<T> g(w, h);
			for (int y = 0; y < h; ++y)
			{
				const json& row = j[y];
				if (!row.is_array() || static_cast<int>(row.size()) != w)
				{
					error = std::string(name) + ": row " + std::to_string(y) +
						" should have " + std::to_string(w) + " cells";
					return false;
				}

				for (int x = 0; x < w; ++x)
				{
					if (!row[x].is_number_integer())
					{
						error = std::string(name) + ": non-integer at (" +
							std::to_string(x) + "," + std::to_string(y) + ")";
						return false;
					}

					const std::int64_t v = row[x].get<std::int64_t>();
					if (v < 0 || v > static_cast<std::int64_t>((std::numeric_limits<T>::max)()))
					{
						error = std::string(name) + ": value out of range at (" +
							std::to_string(x) + "," + std::to_string(y) + ")";
						return false;
					}
					g.At(x, y) = static_cast<T>(v);
				}
			}

			out = std::move(g);
			return true;
		}
	}

	json RoomToJson(const RoomData& room)
	{
		json j;
		j["version"] = kRoomFormatVersion;
		j["id"] = room.id;
		j["backdropId"] = room.backdropId;
		j["width"] = room.width;
		j["height"] = room.height;
		j["type"] = static_cast<int>(room.type);
		j["doors"] = static_cast<int>(room.doors);
		j["difficulty"] = room.difficulty;
		j["weight"] = room.weight;

		j["visualLayers"] = json::array();
		for (const auto& layer : room.visualLayers)
			j["visualLayers"].push_back(GridToJson(layer));

		j["gameplayLayer"] = GridToJson(room.gameplayLayer);
		j["spawnLayer"] = GridToJson(room.spawnLayer);
		return j;
	}

	bool RoomFromJson(const json& j, const TileDatabase& tiles,
					  RoomData& out, std::string& error)
	{
		if (!j.is_object())
		{
			error = "room file is not a JSON object";
			return false;
		}

		try
		{
			if (j.value("version", 1) > kRoomFormatVersion)
			{
				error = "room file is from a newer format version";
				return false;
			}

			RoomData room; // parse into a temporary, commit at the end

			room.width = j.at("width").get<int>();
			room.height = j.at("height").get<int>();
			if (room.width < 1 || room.height < 1 ||
				room.width > kMaxRoomDimension || room.height > kMaxRoomDimension)
			{
				error = "invalid room dimensions";
				return false;
			}

			room.id = j.value("id", -1);
			const int backdrop = j.value("backdropId", 0); // version 1 rooms don't have it
			if (backdrop < 0 || backdrop >(std::numeric_limits<BackdropID>::max)())
			{
				error = "backdropId out of range";
				return false;
			}
			room.backdropId = static_cast<BackdropID>(backdrop);
			room.type = static_cast<RoomType>(j.at("type").get<int>());
			room.difficulty = j.value("difficulty", 1);
			room.weight = j.value("weight", 1);

			const int doors = j.value("doors", 0);
			if (doors < 0 || doors > 0b1111)
			{
				error = "doors bitmask out of range";
				return false;
			}
			room.doors = static_cast<DoorFlags>(doors);

			// Visual layers
			const json& visual = j.at("visualLayers");
			if (!visual.is_array() || visual.empty() ||
				static_cast<int>(visual.size()) > CLVR_MAX_VISUAL_LAYERS)
			{
				error = "visualLayers must be an array of 1.." +
					std::to_string(CLVR_MAX_VISUAL_LAYERS) + " layers";
				return false;
			}

			room.visualLayers.clear();
			for (size_t i = 0; i < visual.size(); ++i)
			{
				Grid<TileID> layer;
				const std::string name = "visualLayers[" + std::to_string(i) + "]";
				if (!GridFromJson(visual[i], room.width, room.height, name.c_str(), layer, error))
					return false;
				room.visualLayers.push_back(std::move(layer));
			}

			if (!GridFromJson(j.at("gameplayLayer"), room.width, room.height,
							  "gameplayLayer", room.gameplayLayer, error))
				return false;

			if (!GridFromJson(j.at("spawnLayer"), room.width, room.height,
							  "spawnLayer", room.spawnLayer, error))
				return false;

			// Every tile ID must be 0 (empty) or exist in the database.
			auto checkTiles = [&](const Grid<TileID>& g, const std::string& name) -> bool
			{
				for (int y = 0; y < g.height; ++y)
					for (int x = 0; x < g.width; ++x)
					{
						const TileID id = g.At(x, y);
						if (id != 0 && !tiles.Get(id))
						{
							error = name + ": unknown tile ID " + std::to_string(id) +
								" at (" + std::to_string(x) + "," + std::to_string(y) + ")";
							return false;
						}
					}
				return true;
			};

			for (size_t i = 0; i < room.visualLayers.size(); ++i)
				if (!checkTiles(room.visualLayers[i], "visualLayers[" + std::to_string(i) + "]"))
					return false;

			if (!checkTiles(room.gameplayLayer, "gameplayLayer"))
				return false;

			out = std::move(room);
			return true;
		}
		catch (const json::exception& e)
		{
			error = std::string("malformed room file: ") + e.what();
			return false;
		}
	}

	bool LoadRoomFromFile(const std::filesystem::path& path, const TileDatabase& tiles,
						  RoomData& out, std::string& error)
	{
		std::ifstream file(path);
		if (!file)
		{
			error = "could not open " + path.string();
			return false;
		}

		const nlohmann::json j = nlohmann::json::parse(file, nullptr, /*allow_exceptions=*/false);
		if (j.is_discarded())
		{
			error = path.string() + " is not valid JSON";
			return false;
		}

		return RoomFromJson(j, tiles, out, error);
	}

	bool SaveRoomToFile(const std::filesystem::path& path, const RoomData& room)
	{
		return WriteJsonFile(path, RoomToJson(room));
	}
}