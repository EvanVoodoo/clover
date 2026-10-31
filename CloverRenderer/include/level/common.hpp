#pragma once

#define CLVR_ROOM_WIDTH 13
#define CLVR_ROOM_HEIGHT 7
#define CLVR_MAX_VISUAL_LAYERS 4

namespace clvr
{
	enum class RoomType
	{
		Normal = 0,
		Boss = 1,
		Treasure = 2,
		Shop = 3,
	};
}