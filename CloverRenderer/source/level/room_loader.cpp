#include "level/room_loader.hpp"
#include <core/engine.hpp>
#include "level/room_serialization.hpp"
#include <utility>

namespace clvr {
	RoomLoader::RoomLoader() {}
	RoomLoader::~RoomLoader() {}

	std::vector<RoomData> RoomLoader::GetAllRooms()
	{
		std::vector<RoomData> rooms;

		auto rm = Engine.GetResourceManager();
		auto roomFiles = rm->GetFilesInDirectory(ResourceManager::Directory::Assets, "rooms", ".json");
		
		for (const auto& file : roomFiles) {
			RoomData room;
			std::string error = "";
			LoadRoomFromFile(rm->GetPath(ResourceManager::Directory::Assets, file.name), *rm->Load<TileDatabase>(rm->GetPath(ResourceManager::Directory::Assets, "tiles/tiles.json").c_str()), room, error);
			if (!error.empty()) {
				// Handle the error, e.g., log it or display a message
			}
			else {
				rooms.push_back(std::move(room));
			}
		}

		return rooms;
	}

	bool RoomLoader::CreateRoom(const RoomData& room)
	{
		if (room.id < 0) {
			return false;
		}

		

		return true;
	}


}