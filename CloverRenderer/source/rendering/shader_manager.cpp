#include "rendering/shader_manager.hpp"

using namespace clvr;

ShaderManager::ShaderManager()
	: m_device(nullptr), m_hwnd(nullptr), m_activeShader(nullptr), m_postProcessShader(nullptr)
{
}

ShaderManager::~ShaderManager()
{
}

bool ShaderManager::Initialize(ID3D11Device* device, HWND hwnd)
{
	m_device = device;
	m_hwnd = hwnd;
	return true;
}

void ShaderManager::Shutdown()
{
	for (auto& pair : m_shaders)
	{
		if (pair.second)
		{
			pair.second->Shutdown();
			delete pair.second;
			pair.second = nullptr;
		}
	}
	m_shaders.clear();
	m_activeShader = nullptr;
	m_postProcessShader = nullptr;
}

bool ShaderManager::LoadShader(const std::wstring& name, const wchar_t* vsFilename, const wchar_t* psFilename)
{
	if (m_shaders.find(name) != m_shaders.end())
	{
		return false; // Shader with this name already exists
	}

	Shader* shader = new Shader();
	if (!shader->Initialize(m_device, m_hwnd, vsFilename, psFilename))
	{
		delete shader;
		return false;
	}

	m_shaders[name] = shader;

	if (!m_activeShader)
	{
		m_activeShader = shader; // Set the first loaded shader as active by default
	}

	return true;
}

bool ShaderManager::SetActiveShader(const std::wstring& name)
{
	auto it = m_shaders.find(name);
	if (it != m_shaders.end())
	{
		m_activeShader = it->second;
		return true;
	}
	return false; // Shader with this name not found
}

Shader* ShaderManager::GetActiveShader()
{
	return m_activeShader;
}

Shader* ShaderManager::GetShader(const std::wstring& name)
{
	auto it = m_shaders.find(name);
	if (it != m_shaders.end())
	{
		return it->second;
	}
	return nullptr;
}

bool ShaderManager::ReloadAll()
{
	for (auto& pair : m_shaders)
	{
		if (!pair.second->NeedsReload()) continue;
		if (!pair.second->Reload(m_device, m_hwnd))
		{
			return false; // Failed to reload a shaders
		}
	}
	return true;
}

void ShaderManager::SetPostProcessShader(const std::wstring& name)
{
	auto it = m_shaders.find(name);
	if (it != m_shaders.end())
		m_postProcessShader = it->second;
}

Shader* ShaderManager::GetPostProcessShader()
{
	return m_postProcessShader;
}