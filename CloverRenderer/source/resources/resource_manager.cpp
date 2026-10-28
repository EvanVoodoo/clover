#include "resources/resource_manager.hpp"
#include <core/engine.hpp>

namespace fs = std::filesystem;

namespace clvr {
	ResourceManager::ResourceManager() {
        if (IsDevEnvironment()) {
            m_directoryPaths = {
                { Directory::SharedAssets, "../CloverRenderer/assets/" },
                { Directory::Assets,       "assets/" },
                { Directory::Saves,        "saves/" },
            };
        }
        else {
            m_directoryPaths = {
                { Directory::SharedAssets, "CloverRenderer/assets/" },
                { Directory::Assets,       "assets/" },
                { Directory::Saves,        "saves/" },
            };
        }

	}
    std::vector<FileInfo> ResourceManager::GetFilesInDirectory(Directory dir, const std::string& relativePath, const std::string& extensionFilter)
    {
        std::vector<FileInfo> files = {};
        // Get the full path to the directory
		const fs::path fullPath = GetPath(dir, relativePath);

        std::error_code ec;
        if (!fs::is_directory(fullPath, ec))
            return files;   // missing or not a directory: return empty instead of throwing

        // Normalize the filter once: lowercase, with a leading dot (".png"). Empty means "all files".
        std::string filter = extensionFilter;
        std::transform(filter.begin(), filter.end(), filter.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (!filter.empty() && filter.front() != '.')
            filter.insert(filter.begin(), '.');

        for (const fs::directory_entry& entry : fs::directory_iterator(fullPath, ec))
        {
            std::error_code entryEc;
            if (!entry.is_regular_file(entryEc))
                continue;   // skip subdirectories and special files

            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            if (!filter.empty() && ext != filter)
                continue;

            FileInfo info;
            info.name = entry.path().filename().string();
            info.path = entry.path().string();
            info.size = fs::file_size(entry.path());
            files.push_back(std::move(info));
        }

        // directory_iterator order is unspecified, so sort for a stable listing (the UI would otherwise reshuffle).
        std::sort(files.begin(), files.end(),
                  [](const FileInfo& a, const FileInfo& b) { return a.name < b.name; });

		return files;
        
    }
}