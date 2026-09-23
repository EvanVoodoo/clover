#pragma once

#include <map>
#include <string>
#include "shader.hpp"

namespace clvr
{
	class ShaderManager
	{
    public:
        ShaderManager();
        ~ShaderManager();

        bool Initialize(ID3D11Device* device, HWND hwnd);
        void Shutdown();

        bool LoadShader(const std::wstring& name, const wchar_t* vsFilename, const wchar_t* psFilename);
        bool SetActiveShader(const std::wstring& name);
        std::shared_ptr<Shader> GetActiveShader();
        std::shared_ptr<Shader> GetShader(const std::wstring& name);
        bool ReloadAll();

        void SetPostProcessShader(const std::wstring& name);
        std::shared_ptr<Shader> GetPostProcessShader();

		int* GetTest() { return test; } // Example of a getter for the pointer member variable

    private:
        ID3D11Device* m_device;
        HWND m_hwnd;
        std::map<std::wstring, std::shared_ptr<Shader>> m_shaders;
        std::shared_ptr<Shader> m_activeShader;
        std::shared_ptr<Shader> m_postProcessShader;
		int* test = nullptr; // Example of a pointer member variable

        bool m_isDestroyed = false;
	};
}