#pragma once

#include "room.hpp"
#include <vector>
#include "tile.hpp"
#include <core/common.hpp>
#include <DirectXMath.h>
#include <memory>
#include <core/ecs.hpp>
#include <string>
#include "common.hpp"
#include "dungeon.hpp"
#include <core/transform.hpp>

namespace clvr {
	class RoomLoader : public System
	{
	public:
		RoomLoader();
		~RoomLoader();

		void Update(float);
		void Render();
		void Inspect(float);

		RunMode GetRunMode() const { return RunMode::Always; }

		void OnPlayStart();

		std::vector<RoomData> GetRooms() const { return m_rooms; }
		std::vector<RoomData> ReloadAllRooms();
		bool AssignRandomRoom(DungeonCell& room);

		void SpawnDoorTrigger(Entity root, XMFLOAT2 halfExtents, DoorType door);
		bool SpawnTile(const TileDef& tileDef, std::shared_ptr<TileDatabase> tileDb, Entity roomTransform, XMFLOAT2 localPos, float tileSize, unsigned int spriteLayerId);

		Entity SpawnRoom(const std::filesystem::path& path, const DoorFlags doors, std::shared_ptr<TileDatabase> tileDb, std::shared_ptr<BackdropDatabase> backdropDb, XMFLOAT2 worldPos, float tileSize, unsigned int baseLayerId);
		Entity SpawnRoom(const RoomData& room, const DoorFlags doors, std::shared_ptr<TileDatabase> tileDb, std::shared_ptr<BackdropDatabase> backdropDb, XMFLOAT2 worldPos, float tileSize, unsigned int baseLayerId);
		Entity SpawnWall(Entity root, XMFLOAT2 center, XMFLOAT2 halfExtents);
		void SpawnWalls(Entity root, float left, float right, float top, float bottom, float thickness, DoorFlags doors, float doorWidth);
		void SpawnWallSide(Entity root, bool horizontal, float fixed, float from, float to, float halfThickness, bool hasDoor, float doorWidth);
		bool SpawnBackdrop(const RoomData& room, std::shared_ptr<BackdropDatabase> backdropDb, Entity roomRoot, float tileSize, unsigned int layerId);
	
	private:
		std::vector<std::string> m_roomFileNames;
		bool m_roomListDirty = true;
		int m_selectedRoom = -1;
		float m_testTileSize = CLVR_TILE_SIZE;   // set to your real tile size
		float m_testPos[2] = { 0.f, 0.f };
		int m_testBaseLayer = 0;
		std::vector<RoomData> m_rooms;

	};
}