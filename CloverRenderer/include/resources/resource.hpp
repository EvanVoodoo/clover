#pragma once

#include <string>

namespace clvr {
	enum class ResourceType {
		Texture,
		Shader
	};

	class Resource {
	public:
		Resource(const Resource&) = delete;
		virtual ~Resource() = default;

		ResourceType GetType() const { return m_type; }
		const std::string& GetPath() const { return m_path; }
		const std::string& GetFilename() const { return m_filename; }

	protected:
		Resource(ResourceType type) : m_type(type) {}
		ResourceType m_type;

		std::string m_path;
		std::string m_filename;
	};
}