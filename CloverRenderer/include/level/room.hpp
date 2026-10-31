#pragma once
#include "level/common.hpp"
#include <vector>
#include "tile.hpp"

namespace clvr
{
	using Layer = std::vector<std::vector<TileID>>;

	enum class DoorType : std::uint8_t
	{
		None = 0,
		North = 1 << 0,
		East = 1 << 1,
		South = 1 << 2,
		West = 1 << 3,
	};

	using DoorFlags = std::uint8_t; // bitmask of DoorType

	class RoomMetadata
	{
	public:
		RoomMetadata() = default;

		int GetId() const { return m_id; }
		int GetWidth() const { return m_width; }
		int GetHeight() const { return m_height; }
		RoomType GetRoomType() const { return m_roomType; }
		DoorFlags GetDoorFlags() const { return m_doorFlags; }
		int GetDifficulty() const { return m_difficulty; }
		int GetWeight() const { return m_weight; }

		void SetId(int id) { m_id = id; }
		void SetWidth(int width) { m_width = width; }
		void SetHeight(int height) { m_height = height; }
		void SetRoomType(RoomType type) { m_roomType = type; }
		void SetDoorFlags(DoorFlags flags) { m_doorFlags = flags; }
		void SetDoorFlag(DoorType door, bool enabled) {
			if (enabled)
				m_doorFlags |= static_cast<DoorFlags>(door);
			else
				m_doorFlags &= ~static_cast<DoorFlags>(door);
		}
		void SetDifficulty(int difficulty) { m_difficulty = difficulty; }
		void SetWeight(int weight) { m_weight = weight; }

		Layer& GetVisualLayer(int index) { return m_visualLayers[index]; }
		Layer& GetGameplayLayer() { return m_gameplayLayer; }
		Layer& GetSpawnLayer() { return m_spawnLayer; }

		void SetVisualLayer(int index, const Layer& layer) { m_visualLayers[index] = layer; }
		void SetGameplayLayer(const Layer& layer) { m_gameplayLayer = layer; }
		void SetSpawnLayer(const Layer& layer) { m_spawnLayer = layer; }
	private:
		int m_id = -1;
		int m_width = CLVR_ROOM_WIDTH;
		int m_height = CLVR_ROOM_HEIGHT;

		RoomType m_roomType = RoomType::Normal;
		DoorFlags m_doorFlags : 4 = 0; // 4 bits for door flags

		int m_difficulty = 1;
		int m_weight = 1;

		Layer m_visualLayers[CLVR_MAX_VISUAL_LAYERS];
		Layer m_gameplayLayer;
		Layer m_spawnLayer;
	};
}