#include "level/room.hpp"

namespace clvr {
	// Sets up empty layers of the given size (ID 0 = empty).
	void RoomData::Init(int w, int h, int visualLayerCount)
	{
		width = w;
		height = h;
		visualLayers.assign(visualLayerCount, Grid<TileID>(w, h));
		gameplayLayer = Grid<TileID>(w, h);
		spawnLayer = Grid<SpawnID>(w, h);
	}

	// For the editor: resizes every layer, keeping existing content.
	void RoomData::Resize(int w, int h)
	{
		width = w;
		height = h;
		for (auto& layer : visualLayers)
			layer.Resize(w, h);
		gameplayLayer.Resize(w, h);
		spawnLayer.Resize(w, h);
	}

	bool RoomData::AddVisualLayer()
	{
		if (static_cast<int>(visualLayers.size()) >= CLVR_MAX_VISUAL_LAYERS)
			return false;
		visualLayers.emplace_back(width, height);
		return true;
	}
	
	int RoomData::VisualLayerCount() const { return static_cast<int>(visualLayers.size()); }
	bool RoomData::HasDoor(DoorType d) const { return (doors & static_cast<DoorFlags>(d)) != 0; }

	void RoomData::SetDoor(DoorType d, bool enabled)
	{
		if (enabled)
			doors = static_cast<DoorFlags>(doors | static_cast<DoorFlags>(d));
		else
			doors = static_cast<DoorFlags>(doors & ~static_cast<DoorFlags>(d));
	}
}