#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <windows.h>
#include <filesystem>

namespace clvr {
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

			std::string path = T::GetPath(std::forward<Args>(args)...);
			auto it = m_resources.find(path);
			if (it != m_resources.end()) {
				// If the resource is already loaded, return the existing shared_ptr
				resource = it->second.lock();
			}

			if (!resource) {
				// If the resource is not loaded or has expired, load it
				resource = std::make_shared<T>(std::forward<Args>(args)...);
				m_resources[path] = resource;
			}

			return resource;
		}

		inline std::string GetPath(Directory dir, const std::string& relativePath = "") {
			auto it = m_directoryPaths.find(dir);
			if (it != m_directoryPaths.end()) {
				return (std::filesystem::path(it->second) / relativePath).string();
			}
			return "";
		}

	private:
		// Use weak_ptr to allow resources to be freed when no longer needed through .lock()
		std::unordered_map<std::string, std::weak_ptr<Resource>> m_resources;

		std::unordered_map<Directory, std::string> m_directoryPaths;
	};
}