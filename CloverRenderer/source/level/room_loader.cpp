#include "level/room_loader.hpp"
#include "level/room_serialization.hpp"
#include <core/common.hpp>
#include <core/engine.hpp>
#include <core/transform.hpp>
#include <DirectXMath.h>
#include <filesystem>
#include <imgui.h>
#include <iostream>
#include <level/backdrop.hpp>
#include <level/common.hpp>
#include <level/room.hpp>
#include <level/tile.hpp>
#include <memory>
#include <physics/collision_filters.hpp>
#include <physics/physics_components.hpp>
#include <rendering/render_components.hpp>
#include <rendering/renderer.hpp>
#include <resources/resource_manager.hpp>
#include <string>
#include <types.h>
#include <utility>
#include <vector>
#include <random>
#include <physics/sensor_component.hpp>
#include <physics/world.hpp>
#include <physics/world_helpers.hpp>
#include <level/door_sensor_command.hpp>

namespace clvr {
	RoomLoader::RoomLoader() 
	{
		priority = 20; // has to run before the Dungeon system, which is priority 25
	}
	RoomLoader::~RoomLoader() {}

	void RoomLoader::Update(float)
	{
	}

	void RoomLoader::Render()
	{}

	void RoomLoader::Inspect(float)
	{
		/*auto rm = Engine.GetResourceManager();

		if (m_roomListDirty)
		{
			m_roomFileNames.clear();
			for (const auto& file : rm->GetFilesInDirectory(ResourceManager::Directory::Assets, "rooms", ".json"))
				m_roomFileNames.push_back(file.name);
			m_roomListDirty = false;
			if (m_selectedRoom >= static_cast<int>(m_roomFileNames.size()))
				m_selectedRoom = -1;
		}

		const char* preview = m_selectedRoom >= 0 ? m_roomFileNames[m_selectedRoom].c_str() : "Select a room...";
		if (ImGui::BeginCombo("Room", preview))
		{
			for (int i = 0; i < static_cast<int>(m_roomFileNames.size()); ++i)
			{
				const bool selected = (i == m_selectedRoom);
				if (ImGui::Selectable(m_roomFileNames[i].c_str(), selected))
					m_selectedRoom = i;
				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		if (ImGui::Button("Refresh"))
			m_roomListDirty = true;

		ImGui::DragFloat2("Position", m_testPos);
		ImGui::InputFloat("Tile size", &m_testTileSize);
		ImGui::InputInt("Base sprite layer", &m_testBaseLayer);

		ImGui::BeginDisabled(m_selectedRoom < 0);
		if (ImGui::Button("Spawn room"))
		{
			const auto tilesPath = rm->GetPath(ResourceManager::Directory::Assets, "tiles/tiles.json");
			auto tileDb = rm->Load<TileDatabase>(tilesPath.c_str());
			const auto backdropPath = rm->GetPath(ResourceManager::Directory::Assets, "backdrops/backdrops.json");
			auto backdropDb = rm->Load<BackdropDatabase>(backdropPath.c_str());

			Entity root{};
			if (!SpawnRoom(rm->GetPath(ResourceManager::Directory::Assets, std::string("rooms/" + m_roomFileNames[m_selectedRoom])),
						   tileDb, backdropDb, { m_testPos[0], m_testPos[1] }, m_testTileSize,
						   static_cast<unsigned int>(m_testBaseLayer), root))
			{
				std::cerr << "Test spawn failed for " << m_roomFileNames[m_selectedRoom] << '\n';
			}
		}
		ImGui::EndDisabled();*/
	}

	void RoomLoader::OnPlayStart()
	{
		auto rm = Engine.GetResourceManager();
		const auto tilesPath = rm->GetPath(ResourceManager::Directory::Assets, "tiles/tiles.json");
		auto tileDb = rm->Load<TileDatabase>(tilesPath.c_str());
		tileDb->RefreshSprites();
		
		const auto backdropPath = rm->GetPath(ResourceManager::Directory::Assets, "backdrops/backdrops.json");
		auto backdropDb = rm->Load<BackdropDatabase>(backdropPath.c_str());
		backdropDb->Sprites().Refresh();

		ReloadAllRooms();
	}

	std::vector<RoomData> RoomLoader::ReloadAllRooms()
	{
		m_rooms.clear();

		auto rm = Engine.GetResourceManager();
		auto roomFiles = rm->GetFilesInDirectory(ResourceManager::Directory::Assets, "rooms", ".json");
		
		// Same order every run, so a seed always maps to the same room
		std::sort(roomFiles.begin(), roomFiles.end(),
				  [](const FileInfo& a, const FileInfo& b) { return a.name < b.name; });

		auto tileDb = rm->Load<TileDatabase>(
			rm->GetPath(ResourceManager::Directory::Assets, "tiles/tiles.json").c_str());

		for (const auto& file : roomFiles) {
			RoomData room;
			std::string error;
			if (!LoadRoomFromFile(file.path, *tileDb, room, error))
			{
				std::cerr << "Skipping " << file.name << ": " << error << '\n';
				continue;
			}

			room.id = static_cast<int>(m_rooms.size());   // index into the returned vector
			m_rooms.push_back(std::move(room));
		}

		return m_rooms;
	}

	bool RoomLoader::AssignRandomRoom(DungeonCell& room)
	{
		std::vector<int> validRoomIds;

		// First, filter the rooms based on the criteria
		for (const auto& r : m_rooms) {
			if ((r.doors & room.connections) != room.connections) continue; // skip if the room doesn't have all required doors
			// ... add any other criteria checks here if needed, like difficulty level, etc.

			validRoomIds.push_back(r.id);
		}

		// Then, if there are valid rooms, select one at random using a seeded random number generator
		if (validRoomIds.empty()) {
			return false; // no valid rooms found
		}
		std::uniform_int_distribution<int> dist(0, static_cast<int>(validRoomIds.size()) - 1);
		room.roomDataIndex = validRoomIds[dist(LayoutRNG::m_rng)]; // assign a random index from the valid rooms

		return true;
	}

	void RoomLoader::SpawnDoorTrigger(Entity root, XMFLOAT2 halfExtents, DoorType door)
	{
		auto ecs = Engine.GetECS();
		Entity doorEntity = ecs->CreateEntity();

		Transform& doorTransform = ecs->CreateComponent<Transform>(doorEntity);
		doorTransform.SetParent(root, doorEntity);
		
		SensorComponent& doorSensor = CreateBoxSensor(ecs->GetSystem<World>(), doorEntity, halfExtents.x, halfExtents.y, CollisionFilters::CC::Door);
		DoorSensorCommand& dSC = doorSensor.AddCommand<DoorSensorCommand>();

		DoorComponent& doorComp = ecs->CreateComponent<DoorComponent>(doorEntity);

		switch (door)
		{
			case DoorType::North:
				doorTransform.position = { 0.0f, (-RoomUtils::PitchY() - CLVR_TILE_SIZE * 0.5f) * 0.5f };
				doorComp.type = DoorType::North;
				break;
			case DoorType::East:
				doorTransform.position = { (RoomUtils::PitchX() + CLVR_TILE_SIZE * 0.5f) * 0.5f, 0.0f };
				doorComp.type = DoorType::East;
				break;
			case DoorType::South:
				doorTransform.position = { 0.0f, (RoomUtils::PitchY() + CLVR_TILE_SIZE * 0.5f) * 0.5f };
				doorComp.type = DoorType::South;
				break;
			case DoorType::West:
				doorTransform.position = { (-RoomUtils::PitchX() - CLVR_TILE_SIZE * 0.5f) * 0.5f, 0.0f };
				doorComp.type = DoorType::West;
				break;
			default:
				break;
		}
	}

	bool RoomLoader::SpawnTile(const TileDef& tileDef, std::shared_ptr<TileDatabase> tileDb, Entity roomTransform,
							   XMFLOAT2 localPos, float tileSize, unsigned int spriteLayerId)
	{
		// Resolve the sprite first, so a bad tile doesn't leave an orphan entity behind
		const SpriteRef ref = tileDb->GetSprite(tileDef);
		if (!ref.valid)
			return false;

		XMFLOAT4 sub;
		if (!tileDb->GetSubRect(tileDef, sub))
			return false;

		auto& ecs = Engine.GetECSRef();
		Entity tileEntity = ecs.CreateEntity();

		auto& transform = ecs.CreateComponent<Transform>(tileEntity);
		transform.SetParent(roomTransform, tileEntity);
		transform.position = localPos;

		auto& spriteComp = ecs.CreateComponent<SpriteComponent>(tileEntity);
		Sprite& s = spriteComp.sprite;
		s.position = { 0.0f, 0.0f };
		s.size = { tileSize, tileSize };
		s.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		s.SetTextureParams(ResourceManager::Directory::Assets, tileDb->TilesetPath(tileDef));
		s.subRect = sub;
		s.uvRect = { ref.uv0.x, ref.uv0.y, ref.uv1.x - ref.uv0.x, ref.uv1.y - ref.uv0.y };
		s.pivot = { 0.5f, 0.5f };
		s.rotation = 0.0f;
		s.isOccluder = false;

		// Assign the render layer here (see below)
		auto& renderer = Engine.GetECS()->GetSystem<Renderer>();
		s.layer = renderer.FindOrCreateSpriteLayer(spriteLayerId);

		if (tileDef.blocksMovement || tileDef.blocksProjectiles || tileDef.contactDamage > 0)
		{
			auto& rb = ecs.CreateComponent<RigidBody>(tileEntity);
			rb.bodyType = b2_staticBody;

			auto& collider = ecs.CreateComponent<BoxCollider>(tileEntity);
			collider.halfExtents = { tileSize * 0.5f, tileSize * 0.5f };
			collider.category = CollisionFilters::CollisionCategory::Obstacle;
		}

		return true;
	}

	Entity RoomLoader::SpawnRoom(const std::filesystem::path& path, const DoorFlags doors, std::shared_ptr<TileDatabase> tileDb, std::shared_ptr<BackdropDatabase> backdropDb,
							   XMFLOAT2 worldPos, float tileSize, unsigned int baseLayerId)
	{
		RoomData room;
		std::string error;
		if (!LoadRoomFromFile(path, *tileDb, room, error))
		{
			std::cerr << "Failed to load room: " << error << '\n';
			return entt::null;
		}
		return SpawnRoom(room, doors, tileDb, backdropDb, worldPos, tileSize, baseLayerId);
	}

	Entity RoomLoader::SpawnRoom(const RoomData& room, const DoorFlags doors, std::shared_ptr<TileDatabase> tileDb, std::shared_ptr<BackdropDatabase> backdropDb,
								   XMFLOAT2 worldPos, float tileSize, unsigned int baseLayerId)
	{
		Entity result = entt::null;
		auto& ecs = Engine.GetECSRef();
		Entity root = ecs.CreateEntity();
		auto& rootTransform = ecs.CreateComponent<Transform>(root);
		rootTransform.position = worldPos;
		ecs.CreateComponent<RoomTag>(root);

		SpawnBackdrop(room, backdropDb, root, tileSize, baseLayerId);

		// Center the room on the root so the dungeon can place it by cell position
		const float halfW = (room.width - 2) * 0.5f;
		const float halfH = (room.height - 2) * 0.5f;

		for (size_t layerIdx = 0; layerIdx < room.visualLayers.size(); ++layerIdx)
		{
			const auto& layer = room.visualLayers[layerIdx];

			for (int y = 0; y < room.height; ++y)
			{
				for (int x = 0; x < room.width; ++x)
				{
					const TileID id = layer.At(x, y);
					if (id == 0)
						continue; // 0 = empty

					const TileDef* def = tileDb->Get(id);   // adapt to your TileDatabase lookup
					if (!def)
						continue;

					// Row 0 is the top of the file, so flip y if your world is y-up
					const XMFLOAT2 localPos{ (x - halfW - 0.5f) * tileSize, (halfH - y + 0.5f) * tileSize };

					if (!SpawnTile(*def, tileDb, root, localPos, tileSize,
								   baseLayerId - 1 - static_cast<unsigned int>(layerIdx)))
					{
						return result;
					}
				}
			}
		}

		// create room colliders
		const float left   = (-halfW - 1.5f) * tileSize;
		const float right = (room.width - 1 - halfW + 0.5f) * tileSize;
		const float top = (halfH + 1.5f) * tileSize;
		const float bottom = (halfH - (room.height - 1) - 0.5f) * tileSize;

		const BackdropDef* bd = backdropDb ? backdropDb->Get(room.backdropId) : nullptr;
		const float thickness = (bd ? bd->wallThickness : 1) * tileSize;

		SpawnWalls(root, left, right, top, bottom, thickness, doors, 2.0f * tileSize);

		// check bit flags in room.doors to determine which doors exist and spawn triggers accordingly
		if (doors & static_cast<std::uint8_t>(DoorType::North))
		{
			// spawn north door trigger
			SpawnDoorTrigger(root, { tileSize * 0.5f, tileSize * 0.5f }, DoorType::North);
		}
		if (doors & static_cast<std::uint8_t>(DoorType::East))
		{
			// spawn east door trigger
			SpawnDoorTrigger(root, { tileSize * 0.5f, tileSize * 0.5f }, DoorType::East);
		}
		if (doors & static_cast<std::uint8_t>(DoorType::South))
		{
			// spawn south door trigger
			SpawnDoorTrigger(root, { tileSize * 0.5f, tileSize * 0.5f }, DoorType::South);
		}
		if (doors & static_cast<std::uint8_t>(DoorType::West))
		{
			// spawn west door trigger
			SpawnDoorTrigger(root, { tileSize * 0.5f, tileSize * 0.5f }, DoorType::West);
		}

		result = root;
		return result;
	}

	Entity RoomLoader::SpawnWall(Entity root, XMFLOAT2 center, XMFLOAT2 halfExtents)
	{
		auto& ecs = Engine.GetECSRef();
		Entity wall = ecs.CreateEntity();

		auto& t = ecs.CreateComponent<Transform>(wall);
		t.SetParent(root, wall);
		t.position = center;   // local to the room root

		CreateBoxCollider(Engine.GetECS()->GetSystem<World>(), wall,
						  halfExtents.x, halfExtents.y, CollisionFilters::CC::Wall);
		return wall;
	}

	void RoomLoader::SpawnWalls(Entity root, float left, float right, float top, float bottom,
								float thickness, DoorFlags doors, float doorWidth)
	{
		const float ht = thickness * 0.5f;
		const auto has = [&](DoorType d) { return (doors & static_cast<std::uint8_t>(d)) != 0; };

		// North and south span the full width including the corners
		SpawnWallSide(root, true, top + ht, left - thickness, right + thickness, ht, has(-DoorType::North), doorWidth);
		SpawnWallSide(root, true, bottom - ht, left - thickness, right + thickness, ht, has(-DoorType::South), doorWidth);
		// East and west only span the height between them
		SpawnWallSide(root, false, right + ht, bottom, top, ht, has(DoorType::East), doorWidth);
		SpawnWallSide(root, false, left - ht, bottom, top, ht, has(DoorType::West), doorWidth);
	}

	void RoomLoader::SpawnWallSide(Entity root, bool horizontal, float fixed, float from, float to,
								   float halfThickness, bool hasDoor, float doorWidth)
	{
		auto addSegment = [&](float a, float b)
		{
			if (b - a <= 0.001f)
				return; // gap as wide as the wall, nothing to spawn

			const float mid = (a + b) * 0.5f;
			const float half = (b - a + 0.85f * CLVR_TILE_SIZE) * 0.5f;
			if (horizontal)
				SpawnWall(root, { mid, fixed }, { half, halfThickness });
			else
				SpawnWall(root, { fixed, mid }, { halfThickness, half });
		};

		if (!hasDoor)
		{
			addSegment(from, to);
			return;
		}

		const float mid = (from + to) * 0.5f;
		addSegment(from, mid - doorWidth * 0.5f);
		addSegment(mid + doorWidth * 0.5f, to);
	}

	bool RoomLoader::SpawnBackdrop(const RoomData& room, std::shared_ptr<BackdropDatabase> backdropDb,
								   Entity roomRoot, float tileSize, unsigned int layerId)
	{
		const BackdropDef* def = backdropDb ? backdropDb->Get(room.backdropId) : nullptr;
		if (!def)
			return false; // no backdrop set, or unknown id

		const SpriteRef ref = backdropDb->Sprites().Get(room.backdropId);
		if (!ref.valid)
			return false;

		auto& ecs = Engine.GetECSRef();
		Entity e = ecs.CreateEntity();

		auto& transform = ecs.CreateComponent<Transform>(e);
		transform.SetParent(roomRoot, e);
		transform.position = { 0.0f, 0.0f };   // image is centered on the room (symmetric wall frame)

		auto& spriteComp = ecs.CreateComponent<SpriteComponent>(e);
		Sprite& s = spriteComp.sprite;
		s.position = { 0.0f, 0.0f };
		s.size = { def->widthTiles * tileSize, def->heightTiles * tileSize };   // whole image, frame included
		s.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		s.uvRect = { ref.uv0.x, ref.uv0.y, ref.uv1.x - ref.uv0.x, ref.uv1.y - ref.uv0.y };
		s.pivot = { 0.5f, 0.5f };
		s.rotation = 0.0f;
		s.isOccluder = false;
		s.SetTextureParams(ResourceManager::Directory::Assets, def->image);

		auto& renderer = Engine.GetECS()->GetSystem<Renderer>();
		s.layer = renderer.FindOrCreateSpriteLayer(layerId);
		return true;
	}
}