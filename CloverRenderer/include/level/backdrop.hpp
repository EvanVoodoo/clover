#pragma once
#include <cstdint>
#include <filesystem>
#include <limits>
#include <map>
#include <string>
#include <nlohmann/json_fwd.hpp>
#include "resources/resource.hpp"
#include "level/common.hpp"
#include "backdrop_sprites.hpp"

namespace clvr
{
	struct BackdropDef
	{
		std::string name;
		std::string image;          // relative to the Assets folder, e.g. "backdrops/basement.png"
		int wallThickness = 1;      // frame width in tiles, on every side
		int widthTiles = CLVR_ROOM_WIDTH + 2 * wallThickness;        // whole image, in tiles, wall frame included
		int heightTiles = CLVR_ROOM_HEIGHT + 2 * wallThickness;

		// The playable area inside the frame, in tiles.
		int PlayableWidth() const { return widthTiles - 2 * wallThickness; }
		int PlayableHeight() const { return heightTiles - 2 * wallThickness; }
	};

	class BackdropDatabase : public Resource
	{
	public:
		using json = nlohmann::json;

		explicit BackdropDatabase(const char* filepath);
		~BackdropDatabase() = default;

		void Register(BackdropID id, BackdropDef def);
		void Unregister(BackdropID id) { m_defs.erase(id); }

		const BackdropDef* Get(BackdropID id) const
		{
			auto it = m_defs.find(id);
			return it != m_defs.end() ? &it->second : nullptr;
		}

		const std::map<BackdropID, BackdropDef>& All() const { return m_defs; }

		// Highest ID + 1, or 0 if the ID space is full.
		BackdropID NextFreeID() const
		{
			if (m_defs.empty())
				return 1;
			const BackdropID last = m_defs.rbegin()->first;
			return last == (std::numeric_limits<BackdropID>::max)()
				? BackdropID(0) : static_cast<BackdropID>(last + 1);
		}

		// Same signature as TileDatabase::GetPath, so ResourceManager::Load<> works the same way.
		static std::string GetPath(const char* filepath)
		{
			return "backdrop_database:" + std::filesystem::path(filepath).string();
		}

		bool LoadFromFile(const std::filesystem::path& path);
		bool LoadFromFile() { return LoadFromFile(m_path); }
		bool SaveToFile() const;

		json ToJson() const;
		bool FromJson(const json& j);

		BackdropSprites& Sprites() { return m_sprites; }
		const BackdropSprites& Sprites() const { return m_sprites; }

	private:
		std::filesystem::path m_path;
		std::map<BackdropID, BackdropDef> m_defs;
		BackdropSprites m_sprites;
	};
}