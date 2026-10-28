#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <windows.h>
#include <filesystem>

namespace clvr {
	struct FileInfo {
		std::string name;
		std::string path;
		std::uintmax_t size;
	};

	class Resource;

	class ResourceManager
	{
	public:

		enum class Directory {
			SharedAssets,
			Assets,
			Saves,
		};

		ResourceManager();
		ResourceManager(const ResourceManager&) = delete;
		ResourceManager(ResourceManager&&) = delete;
		ResourceManager& operator=(const ResourceManager&) = delete;
		ResourceManager& operator=(ResourceManager&&) = delete;

		~ResourceManager() {

		}

		template<typename T, typename... Args>
		std::shared_ptr<T> Load(Args&&... args) {
			std::shared_ptr<T> resource;

			std::string path = T::GetPath(args...);
			auto it = m_resources.find(path);
			if (it != m_resources.end()) {
				// Downcast: stored as shared_ptr<Resource>, need shared_ptr<T>
				resource = std::static_pointer_cast<T>(it->second);
			}

			if (!resource) {
				resource = std::make_shared<T>(std::forward<Args>(args)...);
				m_resources[path] = resource;
			}

			return resource;
		}

		void UnloadUnusedResources() {
			for (auto it = m_resources.begin(); it != m_resources.end(); ) {
				if (it->second.use_count() == 1) { // only the ResourceManager holds a reference
					it = m_resources.erase(it);
				}
				else {
					++it;
				}
			}
		}

		inline std::string GetPath(Directory dir, const std::string& relativePath = "") {
			auto it = m_directoryPaths.find(dir);
			if (it != m_directoryPaths.end()) {
				return (std::filesystem::path(it->second) / relativePath).string();
			}
			return "";
		}

		std::vector<FileInfo> GetFilesInDirectory(Directory dir, const std::string& relativePath = "", const std::string& extensionFilter = "");

	private:
		// Use shared_ptr and check if the ref count is 1 to determine if the resource is still in use. (must be 1 because the resource manager itself holds a ref to it)
		std::unordered_map<std::string, std::shared_ptr<Resource>> m_resources;

		std::unordered_map<Directory, std::string> m_directoryPaths;
	};
}