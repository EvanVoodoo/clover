#pragma once
#include <functional>
#include <map>
#include <string>
#include <imgui.h>
#include "level/common.hpp"            // BackdropID
#include "level/tile.hpp"      // SpriteRef
#include "rendering/texture_atlas.hpp"

namespace clvr
{
	class BackdropDatabase;

	class BackdropSprites
	{
	public:
		using PathResolver = std::function<std::string(const std::string&)>; // relative -> absolute

		// Once at startup, before the atlas is built.
		void Init(TextureAtlas& atlas, const BackdropDatabase& db, const PathResolver& resolve);

		// Once per frame. Cheap: one region lookup per backdrop.
		void Refresh();

		SpriteRef Get(BackdropID id) const; // valid == false if missing or not built yet

	private:
		struct Entry
		{
			std::string path;
			ImVec2 uv0{ 0, 0 }, uv1{ 1, 1 };
			bool ready = false;
		};

		TextureAtlas* m_atlas = nullptr; // non-owning
		ImTextureID m_texture = 0;
		std::map<BackdropID, Entry> m_entries;
	};
}