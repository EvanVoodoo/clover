#pragma once

#include "room.hpp"
#include <core/ecs.hpp>
#include <core/editor_window.hpp>
#include <memory>
#include <random>
#include <queue>
#include <cstdint>
#include <utility>
#include <vector>

namespace clvr
{
	constexpr int CLVR_DUNGEON_WIDTH = 13;
	constexpr int CLVR_DUNGEON_HEIGHT = 13;

	struct DungeonCell
	{
		int id = -1;                 // -1 = empty
		RoomType type = RoomType::Normal;
		DoorFlags connections = 0;   // doors actually linked to neighbors
		int roomDataIndex = -1;      // which RoomData in the loader, picked after layout
		int distanceFromSpawn = INT16_MAX; // for pathfinding, etc.
	};

	class Dungeon
	{
	public:
		Dungeon();
		~Dungeon() = default;
		// Picks a random cell at least `edgeMargin` cells away from every grid edge.
		CellCoord PickSpawnCell(std::mt19937& rng, int edgeMargin) const;

		CellCoord GetSpawnCell() const { return m_spawnCell; }
		void SetSpawnCell(CellCoord coord) 
		{ 
			m_spawnCell = coord; 
			DungeonCell& cell = roomGrid.At(coord.x, coord.y);
			cell.type = RoomType::Spawn;
			cell.id = 0;
			cell.distanceFromSpawn = 0;
		}

		Grid<DungeonCell>& GetRoomGrid() { return roomGrid; }
		std::vector<CellCoord> GetNeighboringRooms(const CellCoord cell);
		std::vector<CellCoord> floorCells;
		std::vector<CellCoord> deadEnds;
		Grid<DungeonCell> roomGrid;
	protected:
		CellCoord m_spawnCell;
	};

	enum class CellValueMode { RoomId, AssignedRoomId, DistanceFromSpawn, Doors };
	
	class DungeonGenerator : public System, public EditorWindow
	{
	public:
		DungeonGenerator();
		~DungeonGenerator();

		void Inspect(float dt) override;
		void OnPlayStart() override;
		void OnPlayStop() override;

		void DrawContents() override;
		void OnOpen() override;
		void OnClose() override;

		int priority = 20;
		std::string title = "Dungeon Generator";

		bool Generate();
		bool GenerateRooms(int numberOfRooms);
		void RoomGenCrawl(int& roomsPlaced, int numberOfRooms, std::vector<std::pair<CellCoord, DoorType>>& cardinalDirs, std::queue<clvr::CellCoord>& roomQueue);
		int FindDeadEnds() const;
		void DijkstraMap();
		void SpecialRoomPlacement();

		Dungeon* GetDungeon() const { return m_dungeon.get(); }

	private:
		std::unique_ptr<Dungeon> m_dungeon;

		int m_edgeMargin = 2;
		int m_minNumOfRooms = 8;
		int m_maxRandAdditionalRooms = 2;
		int m_minDeadEnds = 5;
		int m_maxFilledNeighbors = 1;
		bool m_showCellValues = true;
		CellValueMode m_cellValueMode = CellValueMode::RoomId;
		bool m_generateOnPlay = true;
		bool m_generateRandom = true;

		bool m_generated = false;
		std::vector<RoomType> m_guaranteedSpecialRooms;
	};

	enum class TransitionState { Idle, Sliding };

	struct RuntimeState
	{
		bool isVisited = false;
		bool isCleared = false;
	};

	class DungeonRuntime : public System
	{
	public:
		DungeonRuntime();
		~DungeonRuntime() = default;
		void Update(float dt) override;
		void OnPlayStart() override;
		void OnPlayStop() override;
		RunMode GetRunMode() const { return RunMode::Playing; }

		int priority = 19;
		std::string title = "Dungeon Runtime System";

		CellCoord GetCurrentRoom() const { return m_currentRoom; }
		void MoveToRoom(CellCoord newRoom, DoorType doorDir, Entity player = entt::null);
		void UpdateRoomEntity(Entity newRoomEntity);

	private:
		CellCoord m_currentRoom;
		Entity m_currentRoomEntity;

		TransitionState m_transitionState = TransitionState::Idle;
		Grid<RuntimeState> m_runtimeStates;
	};
}