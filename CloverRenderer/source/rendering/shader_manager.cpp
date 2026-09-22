#include "rendering/shader_manager.hpp"
#include <core/engine.hpp>

using namespace clvr;

ShaderManager::ShaderManager()
	: m_device(nullptr), m_hwnd(nullptr), m_activeShader(nullptr), m_postProcessShader(nullptr)
{
}

ShaderManager::~ShaderManager()
{
	// Signal that destruction happened — helps catch dangling-pointer misuse
	m_isDestroyed = true;
	OutputDebugStringA("ShaderManager::~ShaderManager destructor called\n");

#ifdef _DEBUG
	// Optional: hard stop in debug builds if something calls back in after destruction
	assert(m_isDestroyed);
#endif
}

bool ShaderManager::Initialize(ID3D11Device* device, HWND hwnd)
{
	m_device = device;
	m_hwnd = hwnd;
	test = new int(42); // Example of allocating a pointer member variable
	return true;
}

void ShaderManager::Shutdown()
{
	for (auto& pair : m_shaders)
	{
		if (pair.second)
		{
			pair.second->Shutdown();
			pair.second.reset();
			pair.second = nullptr;
		}
	}
	m_shaders.clear();
	m_activeShader = nullptr;
	m_postProcessShader = nullptr;
}

bool ShaderManager::LoadShader(const std::wstring& name, const wchar_t* vsFilename, const wchar_t* psFilename)
{
	// Defensive checks to avoid crashes when ShaderManager is in an invalid state.
	// If 'this' is null or internal state appears invalid, fail gracefully and log.
	if (m_isDestroyed) {
		OutputDebugStringA("ShaderManager::LoadShader called after destruction\n");
		return false;
	}

	// Basic validation of required members
	if (vsFilename == nullptr || psFilename == nullptr) {
		OutputDebugStringA("ShaderManager::LoadShader: null shader filename parameter\n");
		return false;
	}
	if (m_device == nullptr || m_hwnd == nullptr) {
		OutputDebugStringA("ShaderManager::LoadShader: manager not initialized (null device or hwnd)\n");
		return false;
	}
	// Validate that the map object seems usable by catching exceptions from STL operations.
	try {
		if (m_shaders.find(name) != m_shaders.end())
		{
			return false; // Shader with this name already exists
		}
	}
	catch (const std::exception& e)
	{
		OutputDebugStringA("ShaderManager::LoadShader: exception accessing m_shaders: ");
		OutputDebugStringA(e.what());
		OutputDebugStringA("\n");
		return false;
	}


	std::shared_ptr<Shader> shader = nullptr;
	try {
		shader = std::make_shared<Shader>(vsFilename, psFilename);
	}
	catch (const std::bad_alloc&)
	{
		OutputDebugStringA("ShaderManager::LoadShader: allocation failed\n");
		return false;
	}
	catch (...)
	{
		OutputDebugStringA("ShaderManager::LoadShader: unknown exception during allocation\n");
		return false;
	}
	Engine.GetResourceManager()->Load<Shader>(); // Load the shader resource
	if (!shader->Initialize(m_device, m_hwnd))
	{
		shader.reset();
		return false;
	}

	try {
		m_shaders[name] = shader;
	}
	catch (const std::exception& e)
	{
		OutputDebugStringA("ShaderManager::LoadShader: exception inserting into m_shaders: ");
		OutputDebugStringA(e.what());
		OutputDebugStringA("\n");
		shader.reset();
		return false;
	}
	catch (...)
	{
		OutputDebugStringA("ShaderManager::LoadShader: unknown exception inserting into m_shaders\n");
		shader.reset();
		return false;
	}

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

std::shared_ptr<Shader> ShaderManager::GetActiveShader()
{
	return m_activeShader;
}

std::shared_ptr<Shader> ShaderManager::GetShader(const std::wstring& name)
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

std::shared_ptr<Shader> ShaderManager::GetPostProcessShader()
{
	return m_postProcessShader;
}