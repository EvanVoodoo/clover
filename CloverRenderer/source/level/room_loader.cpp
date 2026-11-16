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

namespace clvr {
	RoomLoader::RoomLoader() {}
	RoomLoader::~RoomLoader() {}

	void RoomLoader::Update(float)
	{}

	void RoomLoader::Render()
	{}

	void RoomLoader::Inspect(float)
	{
		auto rm = Engine.GetResourceManager();

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
		ImGui::EndDisabled();
	}

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

	bool RoomLoader::SpawnTile(const TileDef& tileDef, std::shared_ptr<TileDatabase> tileDb, Entity roomTransform,
							   XMFLOAT2 localPos, float tileSize, unsigned int spriteLayerId)
	{
		// Resolve the sprite first, so a bad tile doesn't leave an orphan entity behind
		const SpriteRef ref = tileDb->Sprites().Get(tileDef, tileDb->Tileset());
		if (!ref.valid)
			return false;

		auto& ecs = Engine.GetECSRef();
		Entity tileEntity = ecs.CreateEntity();

		auto& transform = ecs.CreateComponent<Transform>(tileEntity);
		transform.parent = roomTransform;
		transform.position = localPos;

		auto& spriteComp = ecs.CreateComponent<SpriteComponent>(tileEntity);
		Sprite& s = spriteComp.sprite;
		s.position = { 0.0f, 0.0f };
		s.size = { tileSize, tileSize };
		s.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		s.uvRect = { ref.uv0.x, ref.uv0.y, ref.uv1.x - ref.uv0.x, ref.uv1.y - ref.uv0.y };
		s.textureName = "tileset.png";
		s.texturePath = ref.name;
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

	bool RoomLoader::SpawnRoom(const std::filesystem::path& path, std::shared_ptr<TileDatabase> tileDb, std::shared_ptr<BackdropDatabase> backdropDb,
							   XMFLOAT2 worldPos, float tileSize, unsigned int baseLayerId, Entity& outRoot)
	{
		RoomData room;
		std::string error;
		if (!LoadRoomFromFile(path, *tileDb, room, error))
		{
			std::cerr << "Failed to load room: " << error << '\n';
			return false;
		}
		return SpawnRoom(room, tileDb, backdropDb, worldPos, tileSize, baseLayerId, outRoot);
	}

	bool RoomLoader::SpawnRoom(const RoomData& room, std::shared_ptr<TileDatabase> tileDb, std::shared_ptr<BackdropDatabase> backdropDb,
								   XMFLOAT2 worldPos, float tileSize, unsigned int baseLayerId, Entity& outRoot)
	{
		auto& ecs = Engine.GetECSRef();
		Entity root = ecs.CreateEntity();
		auto& rootTransform = ecs.CreateComponent<Transform>(root);
		rootTransform.position = worldPos;

		SpawnBackdrop(room, backdropDb, root, tileSize, baseLayerId);

		// Center the room on the root so the dungeon can place it by cell position
		const float halfW = (room.width - 1) * 0.5f;
		const float halfH = (room.height - 1) * 0.5f;

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
					const XMFLOAT2 localPos{ (x - halfW) * tileSize, (halfH - y) * tileSize };

					SpawnTile(*def, tileDb, root, localPos, tileSize,
							  baseLayerId - 1 - static_cast<unsigned int>(layerIdx));
				}
			}
		}

		outRoot = root;
		return true;
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
		transform.parent = roomRoot;
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

		auto& renderer = Engine.GetECS()->GetSystem<Renderer>();
		s.layer = renderer.FindOrCreateSpriteLayer(layerId);
		return true;
	}
}