#pragma once

#include <filesystem>
#include <nlohmann/json_fwd.hpp>

namespace clvr
{
	bool WriteJsonFile(const std::filesystem::path& path, const nlohmann::json& j);
}