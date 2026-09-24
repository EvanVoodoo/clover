#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include "resources/resource.hpp"
#include <core/engine.hpp>

namespace clvr
{
    using Microsoft::WRL::ComPtr;

	class Texture : public Resource
    {
    public:
        Texture(const wchar_t* filename);
        Texture(std::string filename);
        Texture(ID3D11Device* device, const wchar_t* filename);

        ~Texture();                                // ComPtr releases automatically

        Texture(const Texture&) = delete;                     // Texture owns a GPU resource — move-only
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&&) noexcept = default;                // ComPtr's move ctor nulls the source
        Texture& operator=(Texture&&) noexcept = default;

		// TODO: Implement GetPath that uses the texture filename to create a unique path string
		static std::string GetPath(const wchar_t* filename);
        static std::string GetPath(std::string filename)
        {
			return GetPath(ToWString(filename).c_str());
        }
        static std::string GetPath(ID3D11Device* device, const wchar_t* filename)
        {
            return GetPath(filename);
        }

        bool Load(ID3D11Device* device, const wchar_t* filename);
        void Shutdown();                                       // optional — ComPtr::Reset() does this too

        ID3D11ShaderResourceView* GetSRV() const { return m_srv.Get(); }
        int GetWidth() const { return m_width; }
        int GetHeight() const { return m_height; }

    private:
        ComPtr<ID3D11Texture2D> m_texture;
        ComPtr<ID3D11ShaderResourceView> m_srv;
        int m_width = 0;
        int m_height = 0;
    };
}