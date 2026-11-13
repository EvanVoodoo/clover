#pragma once

#include "room.hpp"
#include "room_loader.hpp"
#include <core/ecs.hpp>
#include <core/editor_window.hpp>
#include <memory>
#include <random>
#include <queue>

namespace clvr
{
	constexpr int CLVR_DUNGEON_WIDTH = 13;
	constexpr int CLVR_DUNGEON_HEIGHT = 13;

	struct CellCoord
	{
		int x = -1;
		int y = -1;
	};

	class Dungeon
	{
	public:
		Dungeon();
		~Dungeon() = default;
		// Picks a random cell at least `edgeMargin` cells away from every grid edge.
		CellCoord PickSpawnCell(std::mt19937& rng, int edgeMargin) const;

		CellCoord GetSpawnCell() const { return m_spawnCell; }
		void SetSpawnCell(CellCoord coord) { m_spawnCell = coord; }

		Grid<RoomData>& GetRoomGrid() { return m_roomGrid; }
		std::vector<CellCoord> GetNeighboringRooms(const CellCoord cell);
	protected:
		Grid<RoomData> m_roomGrid;
		CellCoord m_spawnCell;
	};
	
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
		void RoomGenCrawl(int& roomsPlaced, int numberOfRooms, std::vector<clvr::CellCoord>& cardinalDirs, std::queue<clvr::CellCoord>& roomQueue);
		int CountDeadEnds() const;
	private:
		std::unique_ptr<Dungeon> m_dungeon;
		std::unique_ptr<RoomLoader> m_roomLoader;

		std::mt19937 m_rng;
		int m_seed = 12345;
		int m_edgeMargin = 2;
		int m_minNumOfRooms = 8;
		int m_maxRandAdditionalRooms = 2;
		int m_minDeadEnds = 5;
		int m_maxFilledNeighbors = 1;
	};
}