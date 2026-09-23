#pragma once

#include <d3d11.h>
#include <directxmath.h>
#include <filesystem>
#include <string>
#include "resources/resource.hpp"

using namespace DirectX;
using namespace std;

namespace clvr
{
	class Shader : public Resource
	{
	private:
		struct MatrixBufferType
		{
			XMMATRIX world;
			XMMATRIX view;
			XMMATRIX projection;
		};

	public:
		Shader(const wchar_t* vsFilename, const wchar_t* psFilename);
		Shader(ID3D11Device* device, HWND hwnd, const wchar_t* vs, const wchar_t* ps);
		Shader(const Shader&) = delete;
		~Shader();
		bool Initialize(ID3D11Device* device, HWND hwnd);
		void Shutdown();
		bool Bind(ID3D11DeviceContext* deviceContext);
		bool NeedsReload() const;
		bool Reload(ID3D11Device* device, HWND hwnd); 
		
		static std::string GetPath(const wchar_t* vsFilename, const wchar_t* psFilename);
		static std::string GetPath(ID3D11Device*, HWND, const wchar_t* vs, const wchar_t* ps) {
			return GetPath(vs, ps); // delegate to the 2-arg version
		}
	
	private:
		bool InitializeShader(ID3D11Device*, HWND);
		bool InitializeShaderInto(ID3D11Device* device, HWND hwnd,
			ID3D11VertexShader** outVS, ID3D11PixelShader** outPS, ID3D11InputLayout** outLayout);
		void ShutdownShader();
		void OutputShaderErrorMessage(ID3D10Blob*, HWND, const wchar_t*, bool silent = false);

	private:
		std::wstring m_vsFilename;
		std::wstring m_psFilename;

		ID3D11VertexShader* m_vertexShader;
		ID3D11PixelShader* m_pixelShader;
		ID3D11InputLayout* m_layout;

		std::filesystem::file_time_type m_vsLastModifiedTime;
		std::filesystem::file_time_type m_psLastModifiedTime;
	};
}