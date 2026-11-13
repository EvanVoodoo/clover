#pragma once

#include "room.hpp"
#include <vector>

namespace clvr {
	class RoomLoader
	{
	public:
		RoomLoader();
		~RoomLoader();

		std::vector<RoomData> GetAllRooms();
		
		bool CreateRoom(const RoomData& room);
	};
}