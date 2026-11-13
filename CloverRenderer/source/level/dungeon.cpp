#include "level/dungeon.hpp"
#include <queue>

namespace clvr
{
	Dungeon::Dungeon()
	{
		m_roomGrid = Grid<RoomData>(CLVR_DUNGEON_WIDTH, CLVR_DUNGEON_HEIGHT);
	}

	CellCoord Dungeon::PickSpawnCell(std::mt19937& rng, int edgeMargin) const
	{
		// Clamp so the valid range can never be empty (e.g. margin 6 on a 10-wide grid)
		const int maxMarginX = (CLVR_DUNGEON_WIDTH - 1) / 2;
		const int maxMarginY = (CLVR_DUNGEON_HEIGHT - 1) / 2;
		const int marginX = (std::min)(edgeMargin, maxMarginX);
		const int marginY = (std::min)(edgeMargin, maxMarginY);

		std::uniform_int_distribution<int> distX(marginX, CLVR_DUNGEON_WIDTH - 1 - marginX);
		std::uniform_int_distribution<int> distY(marginY, CLVR_DUNGEON_HEIGHT - 1 - marginY);

		return { distX(rng), distY(rng) };
	}

	std::vector<CellCoord> Dungeon::GetNeighboringRooms(const CellCoord cell)
	{
		std::vector<CellCoord> neighbors;

		std::vector<CellCoord> cardinalDirs = { {0, -1}, {1, 0}, {0, 1}, {-1, 0} }; // N, E, S, W
		for (const auto& dir : cardinalDirs)
		{
			CellCoord next{ cell.x + dir.x, cell.y + dir.y };
			if (next.x < 0 || next.x >= CLVR_DUNGEON_WIDTH || next.y < 0 || next.y >= CLVR_DUNGEON_HEIGHT)
				continue; // out of bounds
			neighbors.push_back(next);
		}
		return neighbors;
	}

	DungeonGenerator::DungeonGenerator() : EditorWindow("Dungeon Generator", ImGuiWindowFlags_MenuBar)
	{
		m_dungeon = nullptr;
		m_roomLoader = std::make_unique<RoomLoader>();

		title = Title();
		priority = 25;
		Open();
	}
	DungeonGenerator::~DungeonGenerator()
	{
		m_dungeon.reset();
		m_roomLoader.reset();
	}

	void DungeonGenerator::Inspect(float dt)
	{
		DrawWindow();
	}

	void DungeonGenerator::OnPlayStart() {}
	void DungeonGenerator::OnPlayStop() {}

	void DungeonGenerator::DrawContents() 
	{
		ImGui::SliderInt("Edge Margin", &m_edgeMargin, 0, ((std::max)(CLVR_DUNGEON_WIDTH, CLVR_DUNGEON_HEIGHT) - 1) / 2);

		const ImGuiStyle& style = ImGui::GetStyle();
		const float halfWidth = (ImGui::GetContentRegionAvail().x - style.ItemSpacing.x) * 0.5f;

		// Sizes the next widget so widget + its label together fit in one half of the row
		auto SetHalfWidth = [&](const char* label)
		{
			const float labelWidth = ImGui::CalcTextSize(label, nullptr, true).x + style.ItemInnerSpacing.x;
			ImGui::SetNextItemWidth((std::max)(halfWidth - labelWidth, 50.f));
		};

		SetHalfWidth("Min Number Rooms");
		ImGui::DragInt("Min Number Rooms", &m_minNumOfRooms, 0.1f, 0, CLVR_DUNGEON_WIDTH * CLVR_DUNGEON_HEIGHT);
		ImGui::SameLine();
		SetHalfWidth("Max Rand. Add. Rooms");
		ImGui::InputInt("Max Rand. Add. Rooms", &m_maxRandAdditionalRooms);

		SetHalfWidth("Min Dead Ends");
		ImGui::InputInt("Min Dead Ends", &m_minDeadEnds);
		ImGui::SameLine();
		SetHalfWidth("Max Filled Neighbors");
		ImGui::SliderInt("Max Filled Neighbors", &m_maxFilledNeighbors, 1, 4);

		ImGui::InputInt("Seed", &m_seed);
		ImGui::SameLine();
		if (ImGui::Button("Randomize"))
		{
			m_seed = std::random_device{}();
		}
		if (ImGui::Button("Generate"))
		{
			Generate();
		}
		ImGui::SameLine();
		if (ImGui::Button("Generate & Randomize"))
		{
			m_seed = std::random_device{}();
			Generate();
		}

		if (!m_dungeon)
		{
			ImGui::TextDisabled("No dungeon generated yet");
			return;
		}

		constexpr float cellSize = 32.f;
		constexpr float cellGap = 2.f;

		const ImVec2 origin = ImGui::GetCursorScreenPos();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const CellCoord spawn = m_dungeon->GetSpawnCell();

		for (int y = 0; y < CLVR_DUNGEON_HEIGHT; ++y)
		{
			for (int x = 0; x < CLVR_DUNGEON_WIDTH; ++x)
			{
				const ImVec2 minSize(origin.x + x * cellSize, origin.y + y * cellSize);
				const ImVec2 maxSize(minSize.x + cellSize - cellGap, minSize.y + cellSize - cellGap);

				const bool isSpawn = (x == spawn.x && y == spawn.y);
				ImU32 color = IM_COL32(60, 60, 70, 255);

				const RoomData room = m_dungeon->GetRoomGrid().At(x, y);
				if (room.id != -1) color = IM_COL32(120, 120, 130, 255);
				if (isSpawn) color = IM_COL32(80, 200, 120, 255);

				drawList->AddRectFilled(minSize, maxSize, color);
			}
		}

		// Reserve the layout space we just drew into, so ImGui knows the widget's size
		ImGui::Dummy(ImVec2(CLVR_DUNGEON_WIDTH * cellSize, CLVR_DUNGEON_HEIGHT * cellSize));

	}
	void DungeonGenerator::OnOpen() {}
	void DungeonGenerator::OnClose() {}

	bool DungeonGenerator::Generate()
	{
		constexpr int kMaxAttempts = 100;

		for (int attempt = 0; attempt < kMaxAttempts; ++attempt)
		{
			m_rng.seed(static_cast<std::mt19937::result_type>(m_seed + attempt));
			m_dungeon = std::make_unique<Dungeon>();

			m_dungeon->SetSpawnCell(m_dungeon->PickSpawnCell(m_rng, m_edgeMargin));

			std::uniform_int_distribution<int> roomDist(0, m_maxRandAdditionalRooms);
			int numberOfRooms = roomDist(m_rng) + m_minNumOfRooms;

			if (GenerateRooms(numberOfRooms) && CountDeadEnds() >= m_minDeadEnds)
			{
				return true;
			}
		}
		m_dungeon.reset();   // or keep the last attempt for debugging
		return false;
	}

	bool DungeonGenerator::GenerateRooms(int numberOfRooms)
	{
		if (!m_dungeon)
			return false;

		// Start from the spawn cell and randomly walk to place rooms until we reach the desired count.
		int roomsPlaced = 0;

		std::vector<CellCoord> cardinalDirs = { {0, -1}, {1, 0}, {0, 1}, {-1, 0} }; // N, E, S, W
		std::queue<CellCoord> roomQueue;
		CellCoord spawn = m_dungeon->GetSpawnCell();
		roomQueue.push(spawn);

		m_dungeon->GetRoomGrid().At(spawn.x, spawn.y).id = roomsPlaced;
		++roomsPlaced;

		while (roomsPlaced < numberOfRooms && !roomQueue.empty())
			RoomGenCrawl(roomsPlaced, numberOfRooms, cardinalDirs, roomQueue);

		return roomsPlaced == numberOfRooms; // false = caller regenerates with the next seed
	}

	void DungeonGenerator::RoomGenCrawl(int& roomsPlaced, int numberOfRooms, std::vector<clvr::CellCoord>& cardinalDirs, std::queue<clvr::CellCoord>& roomQueue)
	{
		std::shuffle(cardinalDirs.begin(), cardinalDirs.end(), m_rng);

		CellCoord current = roomQueue.front();
		roomQueue.pop();

		for (const auto& dir : cardinalDirs)
		{
			// If we already have enough rooms, give up
			if (roomsPlaced >= numberOfRooms)
				break;

			CellCoord next{ current.x + dir.x, current.y + dir.y };
			if (next.x < 0 || next.x >= CLVR_DUNGEON_WIDTH || next.y < 0 || next.y >= CLVR_DUNGEON_HEIGHT)
				continue; // out of bounds

			RoomData& nextRoom = m_dungeon->GetRoomGrid().At(next.x, next.y);

			// If the next cell is already occupied, we cannot place a room there. 
			if (nextRoom.id != -1)
				continue; // already occupied, give up

			// If the neighbour cell itself has more than one filled neighbour, give up.
			auto neighbors = m_dungeon->GetNeighboringRooms(next);
			int filledNeighbors = 0;
			for (const auto& neighbor : neighbors)
			{
				const auto nRoom = m_dungeon->GetRoomGrid().At(neighbor.x, neighbor.y);
				if (nRoom.id != -1)
					++filledNeighbors;
			}
			if (filledNeighbors > m_maxFilledNeighbors)
				continue;

			// Random 50% chance, give up
			auto randomChance = std::uniform_int_distribution<int>(0, 1)(m_rng);
			if (randomChance == 0)
				continue;

			// Otherwise, mark the neighbour cell as having a room in it, and add it to the queue.
			m_dungeon->GetRoomGrid().At(next.x, next.y).id = roomsPlaced; // assign a room ID
			roomQueue.push(next);
			++roomsPlaced;
		}
	}

	int DungeonGenerator::CountDeadEnds() const
	{
		if (!m_dungeon)
			return 0;

		const auto& grid = m_dungeon->GetRoomGrid();
		const CellCoord spawn = m_dungeon->GetSpawnCell();
		int deadEnds = 0;

		for (int y = 0; y < CLVR_DUNGEON_HEIGHT; ++y)
		{
			for (int x = 0; x < CLVR_DUNGEON_WIDTH; ++x)
			{
				if (grid.At(x, y).id == -1)
					continue; // empty cell

				if (x == spawn.x && y == spawn.y)
					continue; // the spawn room never counts as a dead end

				int filledNeighbors = 0;
				for (const auto& n : m_dungeon->GetNeighboringRooms({ x, y }))
				{
					if (grid.At(n.x, n.y).id != -1)
						++filledNeighbors;
				}

				if (filledNeighbors == 1)
					++deadEnds;
			}
		}

		return deadEnds;
	}
}
