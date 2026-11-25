#pragma once
#include "level/common.hpp"
#include <vector>
#include <cstdint>
#include "grid.hpp"
#include "backdrop.hpp"
#include <string>

namespace clvr
{
	using Layer = std::vector<std::vector<TileID>>;

	struct RoomTag {}; // for ecs

	struct RoomData
	{
		int id = -1;
		BackdropID backdropId = 0;
		int width = CLVR_ROOM_WIDTH;
		int height = CLVR_ROOM_HEIGHT;

		RoomType type = RoomType::Normal;
		DoorFlags doors = static_cast<std::uint8_t>(DoorType::ALL);

		int difficulty = 1;
		int weight = 1;

		std::vector<Grid<TileID>> visualLayers;
		Grid<TileID> gameplayLayer;
		Grid<SpawnID> spawnLayer;

		RoomData() { Init(CLVR_ROOM_WIDTH, CLVR_ROOM_HEIGHT, 1); }

		// Sets up empty layers of the given size (ID 0 = empty).
		void Init(int w, int h, int visualLayerCount);

		// For the editor: resizes every layer, keeping existing content.
		void Resize(int w, int h);

		bool AddVisualLayer();

		int VisualLayerCount() const;

		bool HasDoor(DoorType d) const;

		void SetDoor(DoorType d, bool enabled);

		bool RemoveVisualLayer(int index);
	};
}