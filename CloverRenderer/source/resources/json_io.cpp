#include "resources/json_io.hpp"
#include <fstream>
#include <system_error>
#include <nlohmann/json.hpp>

namespace clvr
{
	bool WriteJsonFile(const std::filesystem::path& path, const nlohmann::json& j)
	{
		std::error_code ec;

		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path(), ec);
			if (ec)
				return false;
		}

		std::filesystem::path tmp = path;
		tmp += ".tmp";

		{
			std::ofstream file(tmp, std::ios::trunc);
			if (!file)
				return false;

			file << j.dump(2) << '\n';
			file.flush();

			if (!file)
			{
				file.close();
				std::filesystem::remove(tmp, ec);
				return false;
			}
		} // file closes here, before the rename

		std::filesystem::rename(tmp, path, ec); // replaces an existing file
		if (ec)
		{
			std::filesystem::remove(tmp, ec);
			return false;
		}

		return true;
	}
}