#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <fstream>
#include <map>
#include <filesystem>

using namespace DirectX;
using namespace std;

namespace clvr
{

	class Shader
	{
	private:
		struct MatrixBufferType
		{
			XMMATRIX world;
			XMMATRIX view;
			XMMATRIX projection;
		};

	public:
		Shader();
		Shader(const Shader&);
		~Shader();
		bool Initialize(ID3D11Device* device, HWND hwnd, const wchar_t* vs, const wchar_t* ps);
		void Shutdown();
		bool Bind(ID3D11DeviceContext* deviceContext);
		bool NeedsReload() const;
		bool Reload(ID3D11Device* device, HWND hwnd); 

	private:
		bool InitializeShader(ID3D11Device*, HWND, const wchar_t*, const wchar_t*);
		bool InitializeShaderInto(ID3D11Device* device, HWND hwnd, const wchar_t* vsFilename, const wchar_t* psFilename,
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