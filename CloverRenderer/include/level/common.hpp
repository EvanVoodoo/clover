#pragma once
#include <cstdint>
#include <random>
#include <DirectXMath.h>

#define CLVR_ROOM_WIDTH 13
#define CLVR_ROOM_HEIGHT 7
#define CLVR_TILE_SIZE 128
#define CLVR_MAX_VISUAL_LAYERS 4

#define CLVR_WALL_THICKNESS 1
#define CLVR_WALL_DTHICKNESS CLVR_WALL_THICKNESS*2

namespace clvr
{
	using TileID = std::uint16_t;
	using SpawnID = std::uint16_t;
	using BackdropID = std::uint16_t;

	struct CellCoord
	{
		int x = -1;
		int y = -1;

		bool operator==(const CellCoord& other) const = default;
		CellCoord operator+(const CellCoord& other) const { return { x + other.x, y + other.y }; }
		CellCoord operator+(const int scalar) const { return { x + scalar, y + scalar }; }
		CellCoord operator-(const CellCoord& other) const { return { x - other.x, y - other.y }; }
		CellCoord operator-(const int scalar) const { return { x - scalar, y - scalar }; }
	};

	enum class RoomType
	{
		Normal = 0,
		Spawn = 1,
		Boss = 2,
		Treasure = 3,
		Shop = 4,
		SuperSecret = 5,
		Curse = 6,
		Vault = 7,
	};

	namespace RoomUtils
	{
		inline float PitchX() { return (CLVR_ROOM_WIDTH + CLVR_WALL_DTHICKNESS) * CLVR_TILE_SIZE; }
		inline float PitchY() { return (CLVR_ROOM_HEIGHT + CLVR_WALL_DTHICKNESS) * CLVR_TILE_SIZE; }

		inline DirectX::XMFLOAT2 GetRoomCenter(const CellCoord cell)
		{
			DirectX::XMFLOAT2 center;
			center = DirectX::XMFLOAT2(
				(cell.x * PitchX()) + (PitchX() / 2.f),
				(cell.y * PitchY()) + (PitchY() / 2.f)
			);
			return center;
		}

		inline CellCoord WorldToCell(const DirectX::XMFLOAT2 worldPos)
		{
			CellCoord cell;
			cell.x = static_cast<int>(std::floor(worldPos.x / PitchX()));
			cell.y = static_cast<int>(std::floor(-worldPos.y / PitchY()));
			return cell;
		}
	}

	namespace LayoutRNG
	{
		inline std::mt19937 m_rng;
		inline int m_seed = 12345;

		inline void Reseed()
		{
			m_rng.seed(m_seed);
		}

		inline void SetSeed(int seed)
		{
			m_seed = seed;
			Reseed();
		}
	}

	enum class DoorType : std::uint8_t
	{
		None = 0,
		North = 1 << 0,
		East = 1 << 1,
		South = 1 << 2,
		West = 1 << 3,
		ALL = North | East | South | West
	};

	constexpr DoorType operator-(DoorType d)
	{
		switch (d)
		{
			case DoorType::North: return DoorType::South;
			case DoorType::East:  return DoorType::West;
			case DoorType::South: return DoorType::North;
			case DoorType::West:  return DoorType::East;
			default:              return DoorType::None;   // None, ALL, or combined flags have no single opposite
		}
	}

	inline std::string DoorDirectionToString(DoorType dir)
	{
		switch (dir)
		{
			case DoorType::North: return "North";
			case DoorType::East: return "East";
			case DoorType::South: return "South";
			case DoorType::West: return "West";
			default: return "None";
		}
	}

	inline CellCoord DoorDirectionToCellOffset(DoorType dir)
	{
		switch (dir)
		{
			case DoorType::North: return { 0, -1 };
			case DoorType::East: return { 1, 0 };
			case DoorType::South: return { 0, 1 };
			case DoorType::West: return { -1, 0 };
			default: return { 0, 0 };
		}
	}

	using DoorFlags = std::uint8_t; // bitmask of DoorType

	struct DoorComponent
	{
		DoorType type;
		bool isOpen = false;
	};
}