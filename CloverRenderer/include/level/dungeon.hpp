#pragma once

#include "room.hpp"
#include <core/ecs.hpp>
#include <core/editor_window.hpp>
#include <memory>
#include <random>
#include <queue>
#include <cstdint>
#include <utility>

namespace clvr
{
	constexpr int CLVR_DUNGEON_WIDTH = 13;
	constexpr int CLVR_DUNGEON_HEIGHT = 13;

	struct CellCoord
	{
		int x = -1;
		int y = -1;

		bool operator==(const CellCoord& other) const = default;
	};

	struct DungeonCell
	{
		int id = -1;                 // -1 = empty
		RoomType type = RoomType::Normal;
		DoorFlags connections = 0;   // doors actually linked to neighbors
		int templateIndex = -1;      // which RoomData in the loader, picked after layout
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
			DungeonCell& cell = m_roomGrid.At(coord.x, coord.y);
			cell.type = RoomType::Spawn;
			cell.id = 0;
			cell.distanceFromSpawn = 0;
		}

		Grid<DungeonCell>& GetRoomGrid() { return m_roomGrid; }
		std::vector<CellCoord> GetNeighboringRooms(const CellCoord cell);
		std::vector<CellCoord> floorCells;
		std::vector<CellCoord> deadEnds;

	protected:
		Grid<DungeonCell> m_roomGrid;
		CellCoord m_spawnCell;
	};

	enum class CellValueMode { RoomId, DistanceFromSpawn, Doors };
	
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

		int priority = 25;
		std::string title = "Dungeon Generator";

		bool Generate();
		bool GenerateRooms(int numberOfRooms);
		void RoomGenCrawl(int& roomsPlaced, int numberOfRooms, std::vector<std::pair<CellCoord, DoorType>>& cardinalDirs, std::queue<clvr::CellCoord>& roomQueue);
		int FindDeadEnds() const;
		void DijkstraMap();
		void SpecialRoomSpawn();

	private:
		std::unique_ptr<Dungeon> m_dungeon;

		std::mt19937 m_rng;
		int m_seed = 12345;
		int m_edgeMargin = 2;
		int m_minNumOfRooms = 8;
		int m_maxRandAdditionalRooms = 2;
		int m_minDeadEnds = 5;
		int m_maxFilledNeighbors = 1;
		bool m_showCellValues = true;
		CellValueMode m_cellValueMode = CellValueMode::RoomId;

		std::vector<RoomType> m_guaranteedSpecialRooms;
	};
}