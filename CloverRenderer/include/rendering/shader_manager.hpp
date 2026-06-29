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
        Shader* GetActiveShader();
        Shader* GetShader(const std::wstring& name);
        bool ReloadAll();

        void SetPostProcessShader(const std::wstring& name);
        Shader* GetPostProcessShader();

    private:
        ID3D11Device* m_device;
        HWND m_hwnd;
        std::map<std::wstring, Shader*> m_shaders;
        Shader* m_activeShader;
        Shader* m_postProcessShader;
	};
}