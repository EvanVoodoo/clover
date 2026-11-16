#pragma once

#include "room.hpp"
#include <vector>
#include "tile.hpp"
#include <core/common.hpp>
#include <DirectXMath.h>
#include <memory>
#include <core/ecs.hpp>
#include <string>

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

		std::vector<RoomData> GetAllRooms();

		bool SpawnTile(const TileDef& tileDef, std::shared_ptr<TileDatabase> tileDb, Entity roomTransform, XMFLOAT2 localPos, float tileSize, unsigned int spriteLayerId);

		bool SpawnRoom(const std::filesystem::path& path, std::shared_ptr<TileDatabase> tileDb, std::shared_ptr<BackdropDatabase> backdropDb, XMFLOAT2 worldPos, float tileSize, unsigned int baseLayerId, Entity& outRoot);
		bool SpawnRoom(const RoomData& room, std::shared_ptr<TileDatabase> tileDb, std::shared_ptr<BackdropDatabase> backdropDb, XMFLOAT2 worldPos, float tileSize, unsigned int baseLayerId, Entity& outRoot);
		bool SpawnBackdrop(const RoomData& room, std::shared_ptr<BackdropDatabase> backdropDb, Entity roomRoot, float tileSize, unsigned int layerId);
	
	private:
		std::vector<std::string> m_roomFileNames;
		bool m_roomListDirty = true;
		int m_selectedRoom = -1;
		float m_testTileSize = 16.f;   // set to your real tile size
		float m_testPos[2] = { 0.f, 0.f };
		int m_testBaseLayer = 0;
	};
}