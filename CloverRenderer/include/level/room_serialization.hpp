#pragma once

#include <string>
#include <nlohmann/json_fwd.hpp>
#include <filesystem>

namespace clvr
{
	struct RoomData;
	class TileDatabase;

	nlohmann::json RoomToJson(const RoomData& room);

	// On failure returns false, fills `error`, and leaves `out` untouched.
	bool RoomFromJson(const nlohmann::json& j, const TileDatabase& tiles,
					  RoomData& out, std::string& error);

	bool LoadRoomFromFile(const std::filesystem::path& path, const TileDatabase& tiles,
						  RoomData& out, std::string& error);

	bool SaveRoomToFile(const std::filesystem::path& path, const RoomData& room);
}