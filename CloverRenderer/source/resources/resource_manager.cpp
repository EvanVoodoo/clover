#include "resources/resource_manager.hpp"
#include <core/engine.hpp>

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
}