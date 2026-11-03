#include "level/backdrop.hpp"
#include "resources/json_io.hpp"
#include <cassert>
#include <fstream>
#include <nlohmann/json.hpp>
#include <core/engine.hpp>
#include <rendering/renderer.hpp>

namespace clvr
{
	using json = nlohmann::json;

	// In the .cpp so the full json header stays out of every file that includes backdrop.hpp.
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BackdropDef,
													name, image, widthTiles, heightTiles, wallThickness)

		namespace { constexpr int kBackdropFormatVersion = 1; }

	BackdropDatabase::BackdropDatabase(const char* filepath)
		: m_path(filepath), Resource(ResourceType::BackdropDatabase)
	{
		m_filename = std::filesystem::path(filepath).filename().string();
		m_path = std::filesystem::path(filepath).string();
		LoadFromFile(filepath);

		//auto& renderer = Engine.GetECS()->GetSystem<Renderer>();

		//// for every backdrop, load the texture and add it to the atlas
		//for (auto& file : Engine.GetResourceManager()->GetFilesInDirectory(ResourceManager::Directory::Assets, "backdrops", ".png"))
		//{
		//	auto texture = Engine.GetResourceManager()->Load<Texture>(static_cast<ID3D11Device*>(renderer.GetNativeDeviceHandle()), file.path);
		//	renderer.AddTexture(file.path);
		//}
	}

	void BackdropDatabase::Register(BackdropID id, BackdropDef def)
	{
		assert(id != 0 && "ID 0 is reserved for 'no backdrop'");
		m_defs.insert_or_assign(id, std::move(def));
	}

	bool BackdropDatabase::LoadFromFile(const std::filesystem::path& path)
	{
		std::ifstream file(path);
		if (!file)
			return false;

		const json j = json::parse(file, nullptr, /*allow_exceptions=*/false);
		if (j.is_discarded())
			return false;

		return FromJson(j);
	}

	bool BackdropDatabase::SaveToFile() const
	{
		return WriteJsonFile(m_path, ToJson());
	}

	json BackdropDatabase::ToJson() const
	{
		json db;
		db["version"] = kBackdropFormatVersion;
		db["backdrops"] = json::array();

		for (const auto& [id, def] : m_defs) // std::map: already sorted by ID
			db["backdrops"].push_back({ { "id", id }, { "definition", def } });

		return db;
	}

	bool BackdropDatabase::FromJson(const json& j)
	{
		if (!j.is_object())
			return false;

		const auto listIt = j.find("backdrops");
		if (listIt == j.end() || !listIt->is_array())
			return false;

		if (j.value("version", 1) > kBackdropFormatVersion)
			return false;

		std::map<BackdropID, BackdropDef> parsed; // commit only if the whole file is valid

		try
		{
			for (const auto& item : *listIt)
			{
				const BackdropID id = item.at("id").get<BackdropID>();
				if (id == 0 || parsed.count(id))
					return false; // reserved or duplicate

				BackdropDef def = item.at("definition").get<BackdropDef>();

				if (def.image.empty() ||
					def.widthTiles < 1 || def.heightTiles < 1 ||
					def.wallThickness < 0 ||
					def.PlayableWidth() < 1 || def.PlayableHeight() < 1)
					return false;

				parsed.emplace(id, std::move(def));
			}
		}
		catch (const json::exception&)
		{
			return false;
		}

		m_defs = std::move(parsed);
		return true;
	}
}